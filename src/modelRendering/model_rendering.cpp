#include "model_rendering.h"

#include <map>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

extern std::map<std::string, ModelAsset> g_ModelRegistry;
extern std::map<std::string, int>        g_MaterialTextureIndex;
extern std::map<std::string, ModelPaths> g_PathsRegistry;
extern GLint g_model_uniform;
extern GLint g_bbox_min_uniform;
extern GLint g_bbox_max_uniform;
extern GLint g_texture_index_uniform;
extern GLint g_has_texture_uniform;

ModelAsset BuildModelAsset(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    ModelAsset asset;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        const tinyobj::mesh_t& mesh = model->shapes[shape].mesh;
        size_t num_triangles = mesh.num_face_vertices.size();

        std::map<int, std::vector<size_t>> mat_triangles;
        for (size_t tri = 0; tri < num_triangles; ++tri)
        {
            int mat_id = mesh.material_ids.empty() ? -1 : mesh.material_ids[tri];
            mat_triangles[mat_id].push_back(tri);
        }

        for (auto& mat_entry : mat_triangles)
        {
            int mat_id = mat_entry.first;
            const std::vector<size_t>& tris = mat_entry.second;
            size_t first_index = indices.size();

            const float minval = std::numeric_limits<float>::lowest();
            const float maxval = std::numeric_limits<float>::max();
            glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
            glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

            SceneObject theobject;
            theobject.name                   = model->shapes[shape].name + "#" + std::to_string(mat_id);
            theobject.rendering_mode         = GL_TRIANGLES;
            theobject.vertex_array_object_id = vertex_array_object_id;
            theobject.material_id            = -1;

            for (size_t tri : tris)
            {
                // -------------------------------------------------------
                // NOVO: captura os 3 vértices do triângulo para colisão
                // -------------------------------------------------------
                RawTriangle rawTri;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = mesh.indices[3*tri + vertex];
                    size_t current_vertex = model_coefficients.size() / 4;
                    indices.push_back((GLuint)current_vertex);

                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];

                    model_coefficients.push_back(vx);
                    model_coefficients.push_back(vy);
                    model_coefficients.push_back(vz);
                    model_coefficients.push_back(1.0f);

                    bbox_min.x = std::min(bbox_min.x, vx);
                    bbox_min.y = std::min(bbox_min.y, vy);
                    bbox_min.z = std::min(bbox_min.z, vz);
                    bbox_max.x = std::max(bbox_max.x, vx);
                    bbox_max.y = std::max(bbox_max.y, vy);
                    bbox_max.z = std::max(bbox_max.z, vz);

                    if (idx.normal_index != -1)
                    {
                        normal_coefficients.push_back(model->attrib.normals[3*idx.normal_index + 0]);
                        normal_coefficients.push_back(model->attrib.normals[3*idx.normal_index + 1]);
                        normal_coefficients.push_back(model->attrib.normals[3*idx.normal_index + 2]);
                        normal_coefficients.push_back(0.0f);
                    }
                    else
                    {
                        normal_coefficients.push_back(0.0f);
                        normal_coefficients.push_back(1.0f);
                        normal_coefficients.push_back(0.0f);
                        normal_coefficients.push_back(0.0f);
                    }

                    if (idx.texcoord_index != -1)
                    {
                        texture_coefficients.push_back(model->attrib.texcoords[2*idx.texcoord_index + 0]);
                        texture_coefficients.push_back(model->attrib.texcoords[2*idx.texcoord_index + 1]);
                    }
                    else
                    {
                        texture_coefficients.push_back(0.0f);
                        texture_coefficients.push_back(0.0f);
                    }

                    // Armazena vértice no triângulo cru
                    if      (vertex == 0) rawTri.v0 = glm::vec3(vx, vy, vz);
                    else if (vertex == 1) rawTri.v1 = glm::vec3(vx, vy, vz);
                    else                  rawTri.v2 = glm::vec3(vx, vy, vz);
                }

                // Guarda o triângulo cru no SceneObject
                theobject.tris.push_back(rawTri);
            }

            size_t num_indices = indices.size() - first_index;

            int tex_slot = -1;
            if (mat_id >= 0 && mat_id < (int)model->materials.size())
            {
                const std::string& matname = model->materials[mat_id].name;
                auto it = g_MaterialTextureIndex.find(matname);
                if (it != g_MaterialTextureIndex.end())
                    tex_slot = it->second;
            }

            theobject.first_index = first_index;
            theobject.num_indices = num_indices;
            theobject.bbox_min    = bbox_min;
            theobject.bbox_max    = bbox_max;
            theobject.material_id = tex_slot;

            asset.parts.push_back(theobject);
        }
    }

    // --- VBOs para GPU (igual ao original) ---
    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    GLuint VBO_normal_coefficients_id;
    glGenBuffers(1, &VBO_normal_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    GLuint VBO_texture_coefficients_id;
    glGenBuffers(1, &VBO_texture_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

    glBindVertexArray(0);

    return asset;
}

void DrawModel(const std::string& model_name, glm::mat4 model_matrix)
{
    if (g_ModelRegistry.find(model_name) == g_ModelRegistry.end())
        return;

    const ModelAsset& asset = g_ModelRegistry[model_name];
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model_matrix));

    for (const SceneObject& obj : asset.parts)
    {
        glUniform4f(g_bbox_min_uniform, obj.bbox_min.x, obj.bbox_min.y, obj.bbox_min.z, 1.0f);
        glUniform4f(g_bbox_max_uniform, obj.bbox_max.x, obj.bbox_max.y, obj.bbox_max.z, 1.0f);
        glUniform1i(g_texture_index_uniform, obj.material_id >= 0 ? obj.material_id : 0);
        glUniform1i(g_has_texture_uniform,   obj.material_id >= 0 ? 1 : 0);

        glBindVertexArray(obj.vertex_array_object_id);
        glDrawElements(obj.rendering_mode, (GLsizei)obj.num_indices,
            GL_UNSIGNED_INT, (void*)(obj.first_index * sizeof(GLuint)));
        glBindVertexArray(0);
    }
}

void LoadPathsCSV(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) return;

    std::string line;
    bool isFirstLine = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (isFirstLine) { isFirstLine = false; continue; }

        std::stringstream ss(line);
        std::string name, modelPath, texturePath, useTexStr;

        if (std::getline(ss, name, ',') &&
            std::getline(ss, modelPath, ',') &&
            std::getline(ss, texturePath, ',') &&
            std::getline(ss, useTexStr, ',')) 
        {
            if (!useTexStr.empty() && useTexStr.back() == '\r') {
                useTexStr.pop_back();
            }

            bool useTexture = (useTexStr == "true" || useTexStr == "1");

            g_PathsRegistry.emplace(name, ModelPaths(name, "../../data/" + modelPath, "../../data/" + texturePath, useTexture));
        }
    }
}