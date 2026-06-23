 #include "camera_collision.h"
#include <cmath>
#include <algorithm>

// Teste raio vs triângulo (Möller–Trumbore)
static bool RayVsTriangle(
    const glm::vec3& orig, const glm::vec3& dir,
    const glm::vec3& v0,   const glm::vec3& v1, const glm::vec3& v2,
    float& t)
{
    const float EPSILON = 1e-7f;
    glm::vec3 e1 = v1 - v0;
    glm::vec3 e2 = v2 - v0;
    glm::vec3 h  = glm::cross(dir, e2);
    float     a  = glm::dot(e1, h);

    if (std::abs(a) < EPSILON) return false;   // paralelo

    float     f = 1.0f / a;
    glm::vec3 s = orig - v0;
    float     u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, e1);
    float     v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    t = f * glm::dot(e2, q);
    return t > EPSILON;
}

float SafeCameraDistance(
    const CollisionMesh& mesh,
    const glm::vec3&     target,
    const glm::vec3&     idealCamPos,
    float                idealDist,
    float                minDist)
{
    glm::vec3 dir = idealCamPos - target;   // do alvo para a câmera
    float     len = glm::length(dir);
    if (len < 1e-6f) return minDist;
    glm::vec3 dirN = dir / len;

    // Consulta apenas células próximas ao segmento (centro e raio)
    glm::vec3 segCenter = target + dir * 0.5f;
    auto candidates = QueryGrid(mesh.grid, segCenter, len * 0.5f + 1.0f);

    float closestT = idealDist;   // começa com a distância ideal

    for (int idx : candidates)
    {
        const CollisionTriangle& tri = mesh.triangles[idx];
        float t;
        if (RayVsTriangle(target, dirN, tri.v0, tri.v1, tri.v2, t))
        {
            // Impacto dentro do segmento target→idealCamPos
            if (t > 0.0f && t < closestT)
                closestT = t;
        }
    }

    // Garante distância mínima e deixa um offset de 0.2 antes da parede
    return std::max(minDist, closestT - 0.2f);
}