#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <algorithm>
#include <limits>

struct RayHit {
    bool  hit;
    float t;        // distância ao ponto de impacto
    int   enemyIdx; // índice no vetor g_Enemies
};

// Testa um raio contra uma AABB em espaço de mundo
inline bool RayVsAABB(
    const glm::vec3& origin,
    const glm::vec3& dir,
    const glm::vec3& bmin,
    const glm::vec3& bmax,
    float& tOut)
{
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; i++)
    {
        float d = (i == 0) ? dir.x : (i == 1) ? dir.y : dir.z;
        float o = (i == 0) ? origin.x : (i == 1) ? origin.y : origin.z;
        float mn = (i == 0) ? bmin.x : (i == 1) ? bmin.y : bmin.z;
        float mx = (i == 0) ? bmax.x : (i == 1) ? bmax.y : bmax.z;

        if (std::abs(d) < 1e-8f)
        {
            if (o < mn || o > mx) return false;
        }
        else
        {
            float t1 = (mn - o) / d;
            float t2 = (mx - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }

    tOut = tmin;
    return true;
}