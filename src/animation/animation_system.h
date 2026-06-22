#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "skeletal_animation.h"
#include <algorithm>

// Avança o tempo da animação e recalcula todas as bone matrices.
// Deve ser chamado a cada frame antes de draw().
void UpdateAnimation(const SkinnedModelAsset& asset,
                     AnimationState& state,
                     float deltaTime);