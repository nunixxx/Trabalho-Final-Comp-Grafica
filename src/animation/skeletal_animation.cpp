#include "skeletal_animation.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

// =========================================================
// Conversão Assimp → GLM
// =========================================================
static glm::mat4 ToGLM(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
static glm::vec3 ToGLM(const aiVector3D& v) { return {v.x, v.y, v.z}; }
static glm::quat ToGLM(const aiQuaternion& q) { return {q.w, q.x, q.y, q.z}; }

// =========================================================
// Lê a hierarquia de nós recursivamente
// =========================================================
static void ReadSkeleton(SkeletonNode& out, const aiNode* node) {
    out.name           = node->mName.C_Str();
    out.localTransform = ToGLM(node->mTransformation);
    out.children.resize(node->mNumChildren);
    for (unsigned i = 0; i < node->mNumChildren; i++)
        ReadSkeleton(out.children[i], node->mChildren[i]);
}

// =========================================================
// Lê vértices, normais, UVs e pesos de bones de uma aiMesh
// =========================================================
static void ProcessMesh(
    const aiMesh* mesh,
    std::map<std::string, BoneInfo>& boneMap,
    int& boneCount,
    std::vector<SkinnedVertex>& outVerts,
    std::vector<unsigned int>&  outIdx)
{
    // Vértices base
    outVerts.resize(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; i++) {
        outVerts[i].position  = ToGLM(mesh->mVertices[i]);
        outVerts[i].normal    = mesh->HasNormals()
                                    ? ToGLM(mesh->mNormals[i])
                                    : glm::vec3(0,1,0);
        outVerts[i].texcoords = mesh->HasTextureCoords(0)
                                    ? glm::vec2(mesh->mTextureCoords[0][i].x,
                                                mesh->mTextureCoords[0][i].y)
                                    : glm::vec2(0);
        // Inicializa bone slots como inválidos
        for (int b = 0; b < MAX_BONE_INFLUENCE; b++) {
            outVerts[i].boneIDs[b]     = -1;
            outVerts[i].boneWeights[b] = 0.0f;
        }
    }

    // Índices
    for (unsigned f = 0; f < mesh->mNumFaces; f++)
        for (unsigned j = 0; j < mesh->mFaces[f].mNumIndices; j++)
            outIdx.push_back(mesh->mFaces[f].mIndices[j]);

    // Bones e pesos
    for (unsigned b = 0; b < mesh->mNumBones; b++) {
        const aiBone* bone = mesh->mBones[b];
        std::string boneName = bone->mName.C_Str();

        // Registra o bone se ainda não existe
        if (boneMap.find(boneName) == boneMap.end()) {
            BoneInfo info;
            info.id           = boneCount++;
            info.offsetMatrix = ToGLM(bone->mOffsetMatrix);
            boneMap[boneName] = info;
        }
        int boneID = boneMap[boneName].id;

        // Distribui os pesos nos slots disponíveis do vértice
        for (unsigned w = 0; w < bone->mNumWeights; w++) {
            unsigned vertIdx = bone->mWeights[w].mVertexId;
            float    weight  = bone->mWeights[w].mWeight;

            for (int s = 0; s < MAX_BONE_INFLUENCE; s++) {
                if (outVerts[vertIdx].boneIDs[s] == -1) {
                    outVerts[vertIdx].boneIDs[s]     = boneID;
                    outVerts[vertIdx].boneWeights[s] = weight;
                    break;
                }
            }
        }
    }
}

// =========================================================
// Lê todos os AnimationClips da cena
// =========================================================
static void LoadAnimations(const aiScene* scene,
                           std::vector<AnimationClip>& out)
{
    for (unsigned a = 0; a < scene->mNumAnimations; a++) {
        const aiAnimation* anim = scene->mAnimations[a];
        AnimationClip clip;
        clip.name       = anim->mName.C_Str();
        clip.duration   = (float)anim->mDuration;
        clip.ticksPerSec = anim->mTicksPerSecond > 0
                               ? (float)anim->mTicksPerSecond
                               : 25.0f;

        for (unsigned c = 0; c < anim->mNumChannels; c++) {
            const aiNodeAnim* ch = anim->mChannels[c];
            BoneChannel bc;
            bc.boneName = ch->mNodeName.C_Str();

            for (unsigned k = 0; k < ch->mNumPositionKeys; k++)
                bc.positions.push_back({ToGLM(ch->mPositionKeys[k].mValue),
                                        (float)ch->mPositionKeys[k].mTime});
            for (unsigned k = 0; k < ch->mNumRotationKeys; k++)
                bc.rotations.push_back({ToGLM(ch->mRotationKeys[k].mValue),
                                        (float)ch->mRotationKeys[k].mTime});
            for (unsigned k = 0; k < ch->mNumScalingKeys; k++)
                bc.scales.push_back({ToGLM(ch->mScalingKeys[k].mValue),
                                     (float)ch->mScalingKeys[k].mTime});

            clip.channels.push_back(bc);
        }
        out.push_back(clip);
        printf("[Anim] Clip '%s': %.0f ticks @ %.0f tps\n",
               clip.name.c_str(), clip.duration, clip.ticksPerSec);
    }
}

// =========================================================
// Função pública: carrega um arquivo FBX completo
// =========================================================
SkinnedModelAsset LoadSkinnedModel(const char* path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate      |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs          |
        aiProcess_LimitBoneWeights   // garante max MAX_BONE_INFLUENCE por vértice
    );

    if (!scene || !scene->mRootNode) {
        fprintf(stderr, "[SkinnedModel] Erro ao carregar '%s': %s\n",
                path, importer.GetErrorString());
        return {};
    }

    SkinnedModelAsset asset;
    asset.globalInverseTransform = glm::inverse(ToGLM(scene->mRootNode->mTransformation));

    std::vector<SkinnedVertex> vertices;
    std::vector<unsigned int>  indices;
    
    printf("[LoadSkinned] Processando %u meshes...\n", scene->mNumMeshes);
    fflush(stdout);
    
    for (unsigned m = 0; m < scene->mNumMeshes; m++)
        ProcessMesh(scene->mMeshes[m], asset.boneMap,
                    asset.boneCount, vertices, indices);

    printf("[LoadSkinned] Meshes ok: %zu verts, %zu idx\n", vertices.size(), indices.size());
    fflush(stdout);

    asset.indexCount = indices.size();
    ReadSkeleton(asset.rootNode, scene->mRootNode);
    
    printf("[LoadSkinned] Skeleton ok\n");
    fflush(stdout);
    
    LoadAnimations(scene, asset.animations);
    
    printf("[LoadSkinned] Animations ok\n");
    fflush(stdout);

    // Upload GPU
    printf("[LoadSkinned] Iniciando upload GPU...\n");
    fflush(stdout);
    
    glGenVertexArrays(1, &asset.VAO);
    printf("[LoadSkinned] VAO: %u\n", asset.VAO);
    fflush(stdout);
    
    glGenBuffers(1, &asset.VBO);
    glGenBuffers(1, &asset.EBO);
    glBindVertexArray(asset.VAO);
    
    printf("[LoadSkinned] Buffers gerados\n");
    fflush(stdout);

    glBindBuffer(GL_ARRAY_BUFFER, asset.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(SkinnedVertex),
                 vertices.data(), GL_STATIC_DRAW);

    printf("[LoadSkinned] VBO ok\n");
    fflush(stdout);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asset.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    printf("[LoadSkinned] EBO ok\n");
    fflush(stdout);

    // attribs...

    glBindVertexArray(0);
    
    printf("[LoadSkinned] Upload completo\n");
    fflush(stdout);

    printf("[SkinnedModel] '%s': %zu verts, %zu idx, %d bones, %zu clips\n",
           path, vertices.size(), indices.size(),
           asset.boneCount, asset.animations.size());
    return asset;
}