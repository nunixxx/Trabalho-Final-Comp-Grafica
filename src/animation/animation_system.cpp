#include "animation_system.h"

// Interpola linearmente entre dois keyframes de posição
static glm::vec3 InterpPosition(const std::vector<KeyPosition>& keys, float t) {
    if (keys.size() == 1) return keys[0].value;
    for (size_t i = 0; i + 1 < keys.size(); i++) {
        if (t < keys[i+1].time) {
            float factor = (t - keys[i].time)
                         / (keys[i+1].time - keys[i].time);
            return glm::mix(keys[i].value, keys[i+1].value, factor);
        }
    }
    return keys.back().value;
}

static glm::quat InterpRotation(const std::vector<KeyRotation>& keys, float t) {
    if (keys.size() == 1) return keys[0].value;
    for (size_t i = 0; i + 1 < keys.size(); i++) {
        if (t < keys[i+1].time) {
            float factor = (t - keys[i].time)
                         / (keys[i+1].time - keys[i].time);
            return glm::normalize(glm::slerp(keys[i].value,
                                             keys[i+1].value, factor));
        }
    }
    return keys.back().value;
}

static glm::vec3 InterpScale(const std::vector<KeyScale>& keys, float t) {
    if (keys.size() == 1) return keys[0].value;
    for (size_t i = 0; i + 1 < keys.size(); i++) {
        if (t < keys[i+1].time) {
            float factor = (t - keys[i].time)
                         / (keys[i+1].time - keys[i].time);
            return glm::mix(keys[i].value, keys[i+1].value, factor);
        }
    }
    return keys.back().value;
}

// =========================================================
// Traversal recursivo da hierarquia de bones
// =========================================================
static void ComputeBoneTransforms(
    const SkeletonNode&               node,
    const glm::mat4&                  parentTransform,
    const AnimationClip&              clip,
    const std::map<std::string,BoneInfo>& boneMap,
    const glm::mat4&                  globalInverse,
    float                             currentTime,
    std::vector<glm::mat4>&           out)
{
    glm::mat4 nodeTransform = node.localTransform;

    // Procura se este nó tem canal de animação no clip atual
    for (const auto& ch : clip.channels) {
        if (ch.boneName != node.name) continue;

        glm::vec3 pos   = InterpPosition(ch.positions, currentTime);
        glm::quat rot   = InterpRotation(ch.rotations, currentTime);
        glm::vec3 scale = InterpScale(ch.scales, currentTime);

        // TRS → matrix
        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 R = glm::toMat4(rot);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        nodeTransform = T * R * S;
        break;
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // Se este nó é um bone real, calcula a final matrix
    auto it = boneMap.find(node.name);
    if (it != boneMap.end()) {
        int id = it->second.id;
        if (id < (int)out.size())
            out[id] = globalInverse * globalTransform * it->second.offsetMatrix;
    }

    for (const auto& child : node.children)
        ComputeBoneTransforms(child, globalTransform, clip,
                              boneMap, globalInverse, currentTime, out);
}

// =========================================================
// Chamada a cada frame
// =========================================================
void UpdateAnimation(const SkinnedModelAsset& asset,
                     AnimationState& state,
                     float deltaTime)
{
    if (asset.animations.empty() || !state.playing) return;

    const AnimationClip& clip = asset.animations[state.clipIndex];
    float ticksPerSec = clip.ticksPerSec;

    state.currentTime += ticksPerSec * deltaTime;

    if (state.loop)
        state.currentTime = fmod(state.currentTime, clip.duration);
    else
        state.currentTime = std::min(state.currentTime, clip.duration);

    // Inicializa as matrizes finais com identidade
    state.finalBoneMatrices.assign(asset.boneCount, glm::mat4(1.0f));

    ComputeBoneTransforms(
        asset.rootNode,
        glm::mat4(1.0f),
        clip,
        asset.boneMap,
        asset.globalInverseTransform,
        state.currentTime,
        state.finalBoneMatrices
    );
}