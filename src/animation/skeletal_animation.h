#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <map>
#include <assimp/scene.h>
#include <glad/glad.h>

// Limite de bones por vértice (padrão da indústria)
#define MAX_BONE_INFLUENCE 4
// Limite total de bones no esqueleto
#define MAX_BONES 100

// =========================================================
// Dados por vértice — enviados para a GPU como atributos
// =========================================================
struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords;
    int       boneIDs[MAX_BONE_INFLUENCE];
    float     boneWeights[MAX_BONE_INFLUENCE];
};

// =========================================================
// Um bone: offset matrix + índice
// offset matrix = transforma vértice do espaço local do bone
// para o espaço de bind-pose do modelo
// =========================================================
struct BoneInfo {
    int       id;
    glm::mat4 offsetMatrix;  // bind-pose inverse
};

// =========================================================
// Keyframes individuais por canal
// =========================================================
struct KeyPosition { glm::vec3 value; float time; };
struct KeyRotation { glm::quat value; float time; };
struct KeyScale    { glm::vec3 value; float time; };

// =========================================================
// Canal de animação de um bone
// =========================================================
struct BoneChannel {
    std::string boneName;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale>    scales;
};

// =========================================================
// Uma animação completa (ex: "walk", "idle", "death")
// =========================================================
struct AnimationClip {
    std::string            name;
    float                  duration;    // em ticks
    float                  ticksPerSec;
    std::vector<BoneChannel> channels;
};

// =========================================================
// Nó da hierarquia de bones (árvore)
// =========================================================
struct SkeletonNode {
    std::string               name;
    glm::mat4                 localTransform;
    std::vector<SkeletonNode> children;

    // Garante move correto da estrutura recursiva
    SkeletonNode() = default;
    SkeletonNode(const SkeletonNode&) = default;
    SkeletonNode(SkeletonNode&&) noexcept = default;
    SkeletonNode& operator=(const SkeletonNode&) = default;
    SkeletonNode& operator=(SkeletonNode&&) noexcept = default;
};

// =========================================================
// Asset completo com esqueleto + animações
// =========================================================
struct SkinnedModelAsset {
    GLuint VAO = 0, VBO = 0, EBO = 0;
    size_t indexCount = 0;

    SkeletonNode rootNode;
    std::map<std::string, BoneInfo> boneMap;
    int boneCount = 0;

    std::vector<AnimationClip> animations;
    glm::mat4 globalInverseTransform = glm::mat4(1.0f);

    // Sem cópia — buffers GPU não podem ser duplicados
    SkinnedModelAsset() = default;
    SkinnedModelAsset(const SkinnedModelAsset&) = delete;
    SkinnedModelAsset& operator=(const SkinnedModelAsset&) = delete;

    // Move é permitido
    SkinnedModelAsset(SkinnedModelAsset&& o) noexcept
        : VAO(o.VAO), VBO(o.VBO), EBO(o.EBO)
        , indexCount(o.indexCount)
        , rootNode(std::move(o.rootNode))
        , boneMap(std::move(o.boneMap))
        , boneCount(o.boneCount)
        , animations(std::move(o.animations))
        , globalInverseTransform(o.globalInverseTransform)
    {
        // Zera o original para não deletar os buffers duas vezes
        o.VAO = o.VBO = o.EBO = 0;
        o.indexCount = 0;
        o.boneCount  = 0;
    }

    SkinnedModelAsset& operator=(SkinnedModelAsset&& o) noexcept
    {
        if (this == &o) return *this;
        // Libera buffers atuais se existirem
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);

        VAO = o.VAO; VBO = o.VBO; EBO = o.EBO;
        indexCount            = o.indexCount;
        rootNode              = std::move(o.rootNode);
        boneMap               = std::move(o.boneMap);
        boneCount             = o.boneCount;
        animations            = std::move(o.animations);
        globalInverseTransform = o.globalInverseTransform;

        o.VAO = o.VBO = o.EBO = 0;
        o.indexCount = 0;
        o.boneCount  = 0;
        return *this;
    }

    ~SkinnedModelAsset()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }
};

// =========================================================
// Estado de animação em tempo de execução
// =========================================================
struct AnimationState {
    int   clipIndex    = 0;     // qual AnimationClip está tocando
    float currentTime  = 0.0f;  // tempo atual em ticks
    bool  playing      = true;
    bool  loop         = true;

    // Resultado calculado por UpdateAnimation():
    // uma matrix por bone, enviada ao shader via UBO/uniform array
    std::vector<glm::mat4> finalBoneMatrices;  // tamanho = boneCount
};