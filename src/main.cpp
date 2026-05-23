//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tiny_obj_loader.h>
#include <stb_image.h>

#include "utils.h"
#include "matrices.h"

#include "modelRendering/model_rendering.h"
#include "collision/collision.h"

// Antiga declaração de ObjModel

void BuildTrianglesAndAddToVirtualScene(ObjModel*);
void ComputeNormals(ObjModel* model);
void LoadShadersFromFiles();
GLuint LoadTextureImage(const char* filename);
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Colisao
AABB GetPlayerAABB(glm::vec4 position);

// Antiga declaração de SceneObject

std::map<std::string, SceneObject> g_VirtualScene;

// =========================================================
// Sistema de texturas
// =========================================================
// Mapeia nome do material -> índice na unidade de textura OpenGL
std::map<std::string, int>  g_MaterialTextureIndex;
// Mapeia nome do arquivo de textura -> GLuint (texture object)
std::map<std::string, GLuint> g_LoadedTextures;
// Lista ordenada de GLuints para bind no shader (slot 0..N)
std::vector<GLuint> g_TextureSlots;

struct PlayerInfo
{
    glm::vec4 position;
    float yaw;
};

struct FreeCamInfo
{
    glm::vec4 position;
    float yaw;
    float pitch;
};

// Número máximo de texturas suportado pelo shader
#define MAX_TEXTURES 64
#define PI 3.141592f
#define FREECAM TRUE
#define LOOKAT FALSE
#define PLAYER_HALF_W 0.3f
#define PLAYER_HEIGHT 1.0f

float g_ScreenRatio = 1.0f;

bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;
bool g_MiddleMouseButtonPressed = false;

float g_CameraTheta = 0.0f;
float g_CameraPhi = 0.4f;
float g_CameraDistance = 5.0f;

bool g_CamMode = LOOKAT;

CollisionMesh g_CollisionMesh;
//-38.08f, 0.74f, -161.84f, 1.0f
//0.0f, 0.0f, -1.0f, 1.0f
PlayerInfo g_Player = { glm::vec4(0.0f, 0.0f, -1.0f, 1.0f), -1.57f };

FreeCamInfo g_FreeCam = { g_Player.position, g_Player.yaw, 0.0f };

FreeCamInfo g_FreeCamBackup = g_FreeCam;

glm::vec4 g_LookAtTarget = g_Player.position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);

bool g_ShowInfoText = true;

GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_texture_index_uniform;
GLint g_has_texture_uniform;

// O registro que guarda a geometria pronta para ser instanciada
std::map<std::string, ModelAsset> g_ModelRegistry;

// Protótipos das novas funções

// =========================================================
// Carrega uma textura PNG e retorna o GLuint, usando cache
// =========================================================
GLuint LoadTextureImage(const char* filename)
{
    // Cache: não recarrega a mesma imagem
    auto it = g_LoadedTextures.find(std::string(filename));
    if (it != g_LoadedTextures.end())
        return it->second;

    printf("Carregando textura \"%s\"... ", filename);

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 4);

    if (data == NULL)
    {
        fprintf(stderr, "AVISO: Não foi possível abrir \"%s\".\n", filename);
        g_LoadedTextures[std::string(filename)] = 0;
        return 0;
    }

    printf("OK (%dx%d, %d canais).\n", width, height, channels);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    g_LoadedTextures[std::string(filename)] = texture_id;
    return texture_id;
}

// =========================================================
// Carrega texturas de todos os materiais do modelo
// Retorna o índice de slot para um dado nome de material
// =========================================================
void LoadMaterialTextures(ObjModel* model, const char* basepath)
{
    std::string base(basepath ? basepath : "");

    for (size_t i = 0; i < model->materials.size(); ++i)
    {
        const tinyobj::material_t& mat = model->materials[i];
        const std::string& matname = mat.name;

        // Já processamos este material?
        if (g_MaterialTextureIndex.count(matname))
            continue;

        if (mat.diffuse_texname.empty())
        {
            // Sem textura: índice -1
            g_MaterialTextureIndex[matname] = -1;
            continue;
        }

        // Monta o caminho completo para o arquivo de textura
        std::string texpath = base + mat.diffuse_texname;

        GLuint tex_id = LoadTextureImage(texpath.c_str());

        if (tex_id == 0)
        {
            // Tenta com extensão em minúsculas caso falhe
            std::string lower = texpath;
            for (auto& c : lower) c = tolower(c);
            tex_id = LoadTextureImage(lower.c_str());
        }

        if (tex_id == 0)
        {
            g_MaterialTextureIndex[matname] = -1;
            continue;
        }

        // Atribui ao próximo slot disponível
        int slot = (int)g_TextureSlots.size();
        if (slot >= MAX_TEXTURES)
        {
            fprintf(stderr, "AVISO: Limite de %d texturas atingido. Material '%s' sem textura.\n",
                    MAX_TEXTURES, matname.c_str());
            g_MaterialTextureIndex[matname] = -1;
            continue;
        }

        g_TextureSlots.push_back(tex_id);
        g_MaterialTextureIndex[matname] = slot;
        printf("Material '%s' -> slot %d (tex_id=%u)\n", matname.c_str(), slot, tex_id);
    }
}

// =========================================================
// Ativa todas as texturas nos slots correspondentes
// Deve ser chamado após UseProgram e antes do draw loop
// =========================================================
void BindAllTextures(GLuint program_id)
{
    // Reserva a unidade 31 para o text rendering
    // Usamos unidades 0..N-1 para nossas texturas
    for (int i = 0; i < (int)g_TextureSlots.size() && i < MAX_TEXTURES; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, g_TextureSlots[i]);
    }

    // Passa o array de samplers para o shader
    // (precisa passar os índices das unidades de textura, não os IDs)
    int samplers[MAX_TEXTURES];
    for (int i = 0; i < MAX_TEXTURES; ++i)
        samplers[i] = i;

    GLint loc = glGetUniformLocation(program_id, "texture_map");
    if (loc >= 0)
        glUniform1iv(loc, MAX_TEXTURES, samplers);
}

int main(int argc, char* argv[])
{
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Doom E1M1", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    LoadShadersFromFiles();

    // =========================================================
    // Carrega o jogador Doom E1M1
    // =========================================================
    ObjModel soldierModel("../../data/Soldier/model.obj");
    ComputeNormals(&soldierModel);
    LoadMaterialTextures(&soldierModel, "../../data/Soldier/");
    /// BuildTrianglesAndAddToVirtualScene(&soldierModel);

    /// Nova Função para renderização de objs:
    g_ModelRegistry["soldier"] = BuildModelAsset(&soldierModel);


    // =========================================================
    // Carrega o mapa Doom E1M1
    // =========================================================
    ObjModel mapmodel("../../data/Map/Doom_E1M1.obj");
    ComputeNormals(&mapmodel);
    //LoadMaterialTextures(&mapmodel, "../../data/Map/");
    //BuildTrianglesAndAddToVirtualScene(&mapmodel);

    /// Nova Função para renderização de objs:
    g_ModelRegistry["map"] = BuildModelAsset(&mapmodel);

    g_CollisionMesh = BuildCollisionMesh(g_ModelRegistry["map"]);

    printf("Colisão: %zu boxes construídas.\n", g_CollisionMesh.boxes.size());
    
    TextRendering_Init();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Ativa todas as texturas (uma vez, antes do loop)
    glUseProgram(g_GpuProgramID);
    BindAllTextures(g_GpuProgramID);
    glUseProgram(0);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_GpuProgramID);

        glm::vec4 camera_position_c;
        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        if (g_CamMode == LOOKAT)
        {
            float r = g_CameraDistance;
            float y = r * sin(g_CameraPhi);

            float behindAngle = g_Player.yaw + PI;  // direção de trás do soldado
            float z = r * cos(g_CameraPhi) * cos(behindAngle);
            float x = r * cos(g_CameraPhi) * sin(behindAngle);

            glm::vec4 lookat = g_LookAtTarget;
            camera_position_c  = lookat + glm::vec4(x, y, z, 0.0f);
            camera_view_vector = lookat - camera_position_c;
        }
        else
        {
            glm::vec4 front;
            front.x = cos(g_FreeCam.pitch) * cos(g_FreeCam.yaw);
            front.y = sin(g_FreeCam.pitch);
            front.z = cos(g_FreeCam.pitch) * sin(g_FreeCam.yaw);
            front.w = 0.0f;

            camera_position_c  = g_FreeCam.position;
            camera_view_vector = front;
        }

        glm::mat4 view = Matrix_Camera_View(
            camera_position_c,
            camera_view_vector,
            camera_up_vector
        );

        float nearplane = -0.1f;
        float farplane  = -200.0f;

        float field_of_view = PI / 3.0f;
        glm::mat4 projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        glUniformMatrix4fv(g_view_uniform,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        // =========================================================
        // Renderiza o mapa Doom
        // =========================================================
        glm::mat4 mapModelMatrix = Matrix_Scale(0.05f, 0.05f, 0.05f);
        DrawModel("map", mapModelMatrix); // <-- Renderização direta e rápida

        // =========================================================
        // Renderiza o soldado (se não estiver na câmera livre)
        // =========================================================
        if (g_CamMode == LOOKAT) 
        {
            glm::mat4 soldierModelMatrix =
                Matrix_Translate(g_Player.position.x, g_Player.position.y, g_Player.position.z)
                * Matrix_Rotate_Y(g_Player.yaw)
                * Matrix_Scale(0.1f, 0.1f, 0.1f);

            DrawModel("soldier", soldierModelMatrix); // <-- Desenha o soldado usando a mesma função
        }

        // =========================================================
        // Movimentação FreeCam
        // =========================================================
        if (g_CamMode == FREECAM)
        {
            float cameraSpeed = 0.1f;

            glm::vec4 front;
            front.x = cos(g_FreeCam.pitch) * cos(g_FreeCam.yaw);
            front.y = sin(g_FreeCam.pitch);
            front.z = cos(g_FreeCam.pitch) * sin(g_FreeCam.yaw);
            front.w = 0.0f;
            front = normalize(front);

            glm::vec4 right = crossproduct(front, glm::vec4(0, 1, 0, 0));
            right = normalize(right);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                g_FreeCam.position += front * cameraSpeed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                g_FreeCam.position -= front * cameraSpeed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                g_FreeCam.position -= right * cameraSpeed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                g_FreeCam.position += right * cameraSpeed;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void LoadShadersFromFiles()
{
    GLuint vertex_shader_id   = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if (g_GpuProgramID != 0)
        glDeleteProgram(g_GpuProgramID);

    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    g_model_uniform         = glGetUniformLocation(g_GpuProgramID, "model");
    g_view_uniform          = glGetUniformLocation(g_GpuProgramID, "view");
    g_projection_uniform    = glGetUniformLocation(g_GpuProgramID, "projection");
    g_object_id_uniform     = glGetUniformLocation(g_GpuProgramID, "object_id");
    g_bbox_min_uniform      = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform      = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_texture_index_uniform = glGetUniformLocation(g_GpuProgramID, "texture_index");
    g_has_texture_uniform   = glGetUniformLocation(g_GpuProgramID, "has_texture");

    glUseProgram(g_GpuProgramID);
    glUseProgram(0);
}

void ComputeNormals(ObjModel* model)
{
    if (!model->attrib.normals.empty())
        return;

    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();
        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);
        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve(3 * num_vertices);

    for (const unsigned int& sgroup : sgroup_ids)
    {
        std::vector<int>        num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4>  vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();
            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                if (model->shapes[shape].mesh.smoothing_group_ids[triangle] != sgroup)
                    continue;

                glm::vec4 vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    vertices[vertex] = glm::vec4(
                        model->attrib.vertices[3*idx.vertex_index + 0],
                        model->attrib.vertices[3*idx.vertex_index + 1],
                        model->attrib.vertices[3*idx.vertex_index + 2],
                        1.0f
                    );
                }

                glm::vec4 n = crossproduct(vertices[1] - vertices[0], vertices[2] - vertices[0]);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        std::vector<size_t> normal_indices(num_vertices, 0);
        for (size_t vi = 0; vi < vertex_normals.size(); ++vi)
        {
            if (num_triangles_per_vertex[vi] == 0)
                continue;

            glm::vec4 n = vertex_normals[vi] / (float)num_triangles_per_vertex[vi];
            n /= norm(n);

            model->attrib.normals.push_back(n.x);
            model->attrib.normals.push_back(n.y);
            model->attrib.normals.push_back(n.z);

            normal_indices[vi] = (model->attrib.normals.size() / 3) - 1;
        }

        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();
            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                if (model->shapes[shape].mesh.smoothing_group_ids[triangle] != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[idx.vertex_index];
                }
            }
        }
    }
}

// =========================================================
// BuildTrianglesAndAddToVirtualScene
//
// Estratégia: um VAO por shape. Dentro de cada shape,
// as faces são agrupadas por material_id. Para cada grupo
// (shape, material_id) criamos um SceneObject separado,
// todos compartilhando o mesmo VAO mas com first_index e
// num_indices diferentes.
// =========================================================
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    // Vamos criar UM VAO global que contém todos os vértices
    // e um EBO (índices) compartilhado. Cada SceneObject
    // referencia um subintervalo do EBO.

    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        const tinyobj::mesh_t& mesh = model->shapes[shape].mesh;
        size_t num_triangles = mesh.num_face_vertices.size();

        // Agrupa triângulos por material
        // material_id -> lista de índices de triângulo
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

            for (size_t tri : tris)
            {
                assert(mesh.num_face_vertices[tri] == 3);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = mesh.indices[3*tri + vertex];

                    // O índice aponta para a posição atual no array de vértices
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
                        // Normal dummy para não desalinhar o VBO
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
                }
            }

            size_t num_indices = indices.size() - first_index;

            // Determina o slot de textura para este material
            int tex_slot = -1;
            if (mat_id >= 0 && mat_id < (int)model->materials.size())
            {
                const std::string& matname = model->materials[mat_id].name;
                auto it = g_MaterialTextureIndex.find(matname);
                if (it != g_MaterialTextureIndex.end())
                    tex_slot = it->second;
            }

            // Nome único: "shape_name#mat_id"
            std::string obj_name = model->shapes[shape].name + "#" + std::to_string(mat_id);

            SceneObject theobject;
            theobject.name                   = obj_name;
            theobject.first_index            = first_index;
            theobject.num_indices            = num_indices;
            theobject.rendering_mode         = GL_TRIANGLES;
            theobject.vertex_array_object_id = vertex_array_object_id;
            theobject.bbox_min               = bbox_min;
            theobject.bbox_max               = bbox_max;
            theobject.material_id            = tex_slot;

            g_VirtualScene[obj_name] = theobject;
        }
    }

    // -------------------------------------------------------
    // Envia VBOs para a GPU
    // -------------------------------------------------------

    // VBO posições
    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // VBO normais
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // VBO coordenadas de textura
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // EBO índices
    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

    glBindVertexArray(0);

    printf("Cena construída: %zu objetos, %zu vértices, %zu índices.\n",
           g_VirtualScene.size(), model_coefficients.size()/4, indices.size());
}

GLuint LoadShader_Vertex(const char* filename)
{
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    LoadShader(filename, vertex_shader_id);
    return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char* filename)
{
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    LoadShader(filename, fragment_shader_id);
    return fragment_shader_id;
}

void LoadShader(const char* filename, GLuint shader_id)
{
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch (std::exception& e) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>(str.length());

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);
    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if (log_length != 0)
    {
        std::string output;
        if (!compiled_ok)
            output += "ERROR: OpenGL compilation of \"";
        else
            output += "WARNING: OpenGL compilation of \"";
        output += filename;
        output += "\".\n== Start of compilation log\n";
        output += log;
        output += "== End of compilation log\n";
        fprintf(stderr, "%s", output.c_str());
    }
    delete [] log;
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        GLchar* log = new GLchar[log_length];
        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;
        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";
        delete [] log;
        fprintf(stderr, "%s", output.c_str());
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    return program_id;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

double g_LastCursorPosX, g_LastCursorPosY;

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        g_LeftMouseButtonPressed = false;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        g_RightMouseButtonPressed = false;

    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
        g_MiddleMouseButtonPressed = false;
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (g_LeftMouseButtonPressed && g_CamMode == LOOKAT)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        //g_CameraTheta -= 0.01f * dx;
        g_Player.yaw -= 0.01f * dx;   // mouse gira o soldado
        g_CameraPhi   += 0.01f * dy;

        float phimax =  PI / 2;
        float phimin = -phimax;
        if (g_CameraPhi > phimax) g_CameraPhi = phimax;
        if (g_CameraPhi < phimin) g_CameraPhi = phimin;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_CamMode == FREECAM)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_FreeCam.yaw += 0.003f * dx;
        g_FreeCam.pitch -= 0.003f * dy;

        float limit = PI / 2.0f - 0.01f;
        if (g_FreeCam.pitch >  limit) g_FreeCam.pitch =  limit;
        if (g_FreeCam.pitch < -limit) g_FreeCam.pitch = -limit;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_CameraDistance -= 0.5f * yoffset;
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void Correcao_KeyCallback(int key, int action, int mod);

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    Correcao_KeyCallback(key, action, mod);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_SEMICOLON && action == GLFW_PRESS)
    {
        g_CamMode = !g_CamMode;
        if (g_CamMode == FREECAM)
        {
            // LookAt → FreeCam: restaura o estado salvo anteriormente
            g_FreeCam.position = g_FreeCamBackup.position;
            g_FreeCam.yaw      = g_FreeCamBackup.yaw;
            g_FreeCam.pitch    = g_FreeCamBackup.pitch;

            glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            // FreeCam → LookAt: salva o estado atual da FreeCam
            g_FreeCamBackup.position = g_FreeCam.position;
            g_FreeCamBackup.yaw      = g_FreeCam.yaw;
            g_FreeCamBackup.pitch    = g_FreeCam.pitch;

            // Spawn do personagem na posição atual da FreeCam
            g_Player.position = g_FreeCam.position;
            g_Player.yaw      = g_FreeCam.yaw;

            printf("\n[POSICAO ATUAL]\n");
            printf("g_PlayerSpawnPosition = glm::vec4(%.2ff, %.2ff, %.2ff, 1.0f);\n", 
                g_Player.position.x, g_Player.position.y, g_Player.position.z);
            printf("g_PlayerSpawnYaw      = %.2ff;\n\n", g_Player.yaw);
            fflush(stdout);

            // Herda posição da FreeCam como novo alvo da LookAt
            glm::vec4 front;
            front.x = cos(g_FreeCam.pitch) * cos(g_FreeCam.yaw);
            front.y = sin(g_FreeCam.pitch);
            front.z = cos(g_FreeCam.pitch) * sin(g_FreeCam.yaw);
            front.w = 0.0f;
            front   = normalize(front);

            // O ponto que a câmera estava "olhando" vira o novo lookat_target
            float orbit_dist = 4.0f;
            glm::vec4 lookat_target = g_FreeCam.position + front * orbit_dist;

            // Recalcula theta/phi/distance
            glm::vec4 diff = g_FreeCam.position - lookat_target;
            g_LookAtTarget = g_Player.position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);
            g_CameraDistance = norm(diff);

            if (g_CameraDistance < 0.01f) g_CameraDistance = orbit_dist;

            g_CameraPhi   = asin(diff.y / g_CameraDistance);
            g_CameraTheta = atan2(diff.x, diff.z);

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
        g_ShowInfoText = !g_ShowInfoText;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        // Re-bind texturas após recarregar shaders
        glUseProgram(g_GpuProgramID);
        BindAllTextures(g_GpuProgramID);
        glUseProgram(0);
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }
}

AABB GetPlayerAABB(glm::vec4 position) {
    AABB box;
    box.min = glm::vec3(
        position.x - PLAYER_HALF_W,
        position.y,
        position.z - PLAYER_HALF_W
    );
    box.max = glm::vec3(
        position.x + PLAYER_HALF_W,
        position.y + PLAYER_HEIGHT,
        position.z + PLAYER_HALF_W
    );
    return box;
}

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if (!g_ShowInfoText) return;

    static float old_seconds    = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20]     = "?? fps";
    static int   numchars       = 7;

    ellapsed_frames += 1;
    float seconds          = (float)glfwGetTime();
    float ellapsed_seconds = seconds - old_seconds;

    if (ellapsed_seconds > 1.0f)
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
        old_seconds     = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth  = TextRendering_CharWidth(window);
    TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1)*charwidth, 1.0f - lineheight, 1.0f);
}