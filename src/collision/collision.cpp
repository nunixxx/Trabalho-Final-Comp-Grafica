#include "collision.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

// =========================================================
// Helpers internos
// =========================================================

static glm::vec3 cross3(const glm::vec3& a, const glm::vec3& b)
{
    return glm::vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

static float dot3(const glm::vec3& a, const glm::vec3& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static float len3(const glm::vec3& v)
{
    return std::sqrt(dot3(v, v));
}

static glm::vec3 norm3(const glm::vec3& v)
{
    float l = len3(v);
    if (l < 1e-8f) return glm::vec3(0.0f);
    return v / l;
}

// Ponto mais próximo no segmento [a,b] ao ponto p
static glm::vec3 ClosestPointOnSegment(const glm::vec3& a,
                                       const glm::vec3& b,
                                       const glm::vec3& p)
{
    glm::vec3 ab = b - a;
    float t = dot3(p - a, ab) / (dot3(ab, ab) + 1e-12f);
    t = std::max(0.0f, std::min(1.0f, t));
    return a + ab * t;
}

// Ponto mais próximo no triângulo ao ponto p (espaço 3D)
// Usa o método de Ericson (Real-Time Collision Detection, §5.1.5)
static glm::vec3 ClosestPointOnTriangle(const glm::vec3& p,
                                        const glm::vec3& a,
                                        const glm::vec3& b,
                                        const glm::vec3& c)
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;

    float d1 = dot3(ab, ap);
    float d2 = dot3(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    glm::vec3 bp = p - b;
    float d3 = dot3(ab, bp);
    float d4 = dot3(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return a + ab * v;
    }

    glm::vec3 cp = p - c;
    float d5 = dot3(ab, cp);
    float d6 = dot3(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return a + ac * w;
    }

    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

// =========================================================
// BuildCollisionMesh
// =========================================================
CollisionMesh BuildCollisionMesh(const ModelAsset& mapAsset, float mapScale)
{
    CollisionMesh mesh;
    mesh.grid.cellSize = 2.0f;

    for (const SceneObject& obj : mapAsset.parts)
    {
        // Recupera os triângulos da GPU seria caro; em vez disso,
        // precisamos dos vértices CPU-side.
        // Como BuildModelAsset não guarda os vértices, precisamos usar
        // bbox_min/bbox_max — mas isso perde resolução.
        //
        // SOLUÇÃO: guardamos os triângulos diretamente no SceneObject
        // via o campo extra "tris" adicionado abaixo (veja model_rendering.h).
        // Se o campo não existir no seu projeto ainda, veja as instruções
        // no comentário ao final deste arquivo.

        for (const auto& tri : obj.tris)
        {
            CollisionTriangle ct;
            ct.v0 = tri.v0 * mapScale;
            ct.v1 = tri.v1 * mapScale;
            ct.v2 = tri.v2 * mapScale;

            glm::vec3 e1 = ct.v1 - ct.v0;
            glm::vec3 e2 = ct.v2 - ct.v0;
            ct.normal = norm3(cross3(e1, e2));
            ct.d      = dot3(ct.normal, ct.v0);

            // Só nos interessa triângulos que tenham componente vertical
            // relevante (paredes e pisos) — filtra faces degeneradas
            if (len3(ct.normal) < 0.5f) continue;

            int idx = (int)mesh.triangles.size();
            mesh.triangles.push_back(ct);

            // Registra no grid usando o AABB XZ do triângulo
            float xmin = std::min({ct.v0.x, ct.v1.x, ct.v2.x});
            float xmax = std::max({ct.v0.x, ct.v1.x, ct.v2.x});
            float zmin = std::min({ct.v0.z, ct.v1.z, ct.v2.z});
            float zmax = std::max({ct.v0.z, ct.v1.z, ct.v2.z});

            int cx0 = (int)std::floor(xmin / mesh.grid.cellSize);
            int cx1 = (int)std::floor(xmax / mesh.grid.cellSize);
            int cz0 = (int)std::floor(zmin / mesh.grid.cellSize);
            int cz1 = (int)std::floor(zmax / mesh.grid.cellSize);

            for (int cx = cx0; cx <= cx1; cx++)
                for (int cz = cz0; cz <= cz1; cz++)
                    mesh.grid.cells[{cx, cz}].push_back(idx);
        }
    }

    printf("CollisionMesh: %zu triângulos, %zu células de grid.\n",
           mesh.triangles.size(), mesh.grid.cells.size());
    return mesh;
}

// =========================================================
// QueryGrid
// =========================================================
std::vector<int> QueryGrid(const SpatialGrid& grid,
                           const glm::vec3& center, float radius)
{
    std::vector<int> candidates;

    int cx0 = (int)std::floor((center.x - radius) / grid.cellSize);
    int cx1 = (int)std::floor((center.x + radius) / grid.cellSize);
    int cz0 = (int)std::floor((center.z - radius) / grid.cellSize);
    int cz1 = (int)std::floor((center.z + radius) / grid.cellSize);

    for (int cx = cx0; cx <= cx1; cx++)
    {
        for (int cz = cz0; cz <= cz1; cz++)
        {
            auto it = grid.cells.find({cx, cz});
            if (it == grid.cells.end()) continue;
            for (int idx : it->second)
            {
                bool dup = false;
                for (int c : candidates) if (c == idx) { dup = true; break; }
                if (!dup) candidates.push_back(idx);
            }
        }
    }
    return candidates;
}

// =========================================================
// ResolvePlayerCollision
//
// Algoritmo:
//   Para cada triângulo candidato, testa a esfera de raio `radius`
//   centrada no ponto de referência do jogador contra o triângulo.
//   Se houver penetração, empurra o jogador para fora ao longo do
//   vetor mínimo de separação (MTV).
//
//   O cilindro é aproximado por DUAS esferas:
//     - esfera inferior (pé): base + (0, radius, 0)
//     - esfera superior (cabeça): base + (0, height - radius, 0)
//   Isso cobre paredes, pisos e tetos corretamente.
// =========================================================
glm::vec3 ResolvePlayerCollision(const CollisionMesh& mesh,
                                 glm::vec3 desiredPos,
                                 float radius,
                                 float height)
{
    // Número de passes de resolução (2-3 é suficiente)
    const int PASSES = 3;

    glm::vec3 pos = desiredPos;

    for (int pass = 0; pass < PASSES; pass++)
    {
        // Centros das duas esferas representando o cilindro
        glm::vec3 centers[2] = {
            pos + glm::vec3(0.0f, radius,          0.0f),  // esfera do pé
            pos + glm::vec3(0.0f, height - radius, 0.0f)   // esfera da cabeça
        };

        // Query do grid usando o centro horizontal do jogador
        glm::vec3 hCenter = pos + glm::vec3(0.0f, height * 0.5f, 0.0f);
        std::vector<int> candidates = QueryGrid(mesh.grid, hCenter, radius + 0.5f);

        glm::vec3 totalPush(0.0f);

        for (int idx : candidates)
        {
            const CollisionTriangle& tri = mesh.triangles[idx];

            // Pula triângulos que estão completamente acima ou abaixo do jogador
            float yMin = std::min({tri.v0.y, tri.v1.y, tri.v2.y});
            float yMax = std::max({tri.v0.y, tri.v1.y, tri.v2.y});
            if (yMin > pos.y + height + radius) continue;
            if (yMax < pos.y - radius)          continue;

            // Testa cada esfera contra o triângulo
            for (int s = 0; s < 2; s++)
            {
                const glm::vec3& center = centers[s];

                // Ponto mais próximo no triângulo ao centro da esfera
                glm::vec3 closest = ClosestPointOnTriangle(center,
                                                            tri.v0, tri.v1, tri.v2);
                glm::vec3 diff    = center - closest;
                float     dist    = len3(diff);

                if (dist < radius && dist > 1e-6f)
                {
                    // Vetor de separação mínima
                    glm::vec3 pushDir = diff / dist;
                    float     overlap = radius - dist;

                    // Acumula o push total deste triângulo
                    glm::vec3 push = pushDir * overlap;

                    // Para pisos/tetos (normal predominantemente vertical),
                    // só aplica componente Y do push para não "voar"
                    float normalY = std::abs(tri.normal.y);
                    if (normalY > 0.7f)
                    {
                        // Superfície horizontal: empurra só em Y
                        totalPush.y += push.y;
                    }
                    else
                    {
                        // Superfície vertical (parede): empurra só em XZ
                        totalPush.x += push.x;
                        totalPush.z += push.z;
                    }
                }
            }
        }

        pos += totalPush;

        // Sai cedo se não houve colisão neste passe
        if (len3(totalPush) < 1e-5f) break;
    }

    return pos;
}

// =========================================================
// INSTRUÇÕES: Adicionando o campo `tris` ao SceneObject
// =========================================================
//
// Para que BuildCollisionMesh funcione, o SceneObject em
// model_rendering.h precisa de um campo extra:
//
//   struct RawTriangle { glm::vec3 v0, v1, v2; };
//
//   struct SceneObject {
//       ...campos existentes...
//       std::vector<RawTriangle> tris;  // <-- ADICIONE ISTO
//   };
//
// E em BuildModelAsset (model_rendering.cpp), dentro do loop
// que preenche os `indices`, adicione após calcular os vértices
// de cada triângulo:
//
//   // A cada 3 vértices completos (1 triângulo):
//   if ((current_vertex % 3) == 2) {
//       size_t base = model_coefficients.size() / 4;
//       RawTriangle rt;
//       rt.v0 = glm::vec3(model_coefficients[(base-3)*4+0],
//                         model_coefficients[(base-3)*4+1],
//                         model_coefficients[(base-3)*4+2]);
//       rt.v1 = glm::vec3(model_coefficients[(base-2)*4+0],
//                         model_coefficients[(base-2)*4+1],
//                         model_coefficients[(base-2)*4+2]);
//       rt.v2 = glm::vec3(model_coefficients[(base-1)*4+0],
//                         model_coefficients[(base-1)*4+1],
//                         model_coefficients[(base-1)*4+2]);
//       theobject.tris.push_back(rt);
//   }
//
// Veja o arquivo model_rendering_updated.h/.cpp fornecidos junto.
// =========================================================
