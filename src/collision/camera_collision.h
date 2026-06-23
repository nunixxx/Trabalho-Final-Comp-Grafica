#pragma once

#include "collision.h"
#include <glm/vec3.hpp>

// Retorna a distância máxima segura entre 'target' e 'idealPos',
// testando o segmento contra a CollisionMesh via ray-triangle.
// Se não houver colisão, retorna idealDist.
float SafeCameraDistance(
    const CollisionMesh& mesh,
    const glm::vec3&     target,       // ponto orbitado (lookAtTarget)
    const glm::vec3&     idealCamPos,  // posição ideal sem colisão
    float                idealDist,    // cameraDistance atual
    float                minDist = 0.5f
);

// Dispara um raio da origem na direção (normalizada) contra a malha de colisão.
// Retorna true se atingir algo dentro de maxDist, e preenche outHitDist com a distância.
bool RayCastMesh(
    const CollisionMesh& mesh,
    const glm::vec3&     origin,
    const glm::vec3&     direction,
    float                maxDist,
    float&               outHitDist
);