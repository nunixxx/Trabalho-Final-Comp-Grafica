#pragma once

#include "../modelRendering/model_rendering.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>
#include <vector>

// =========================================================
// Triângulo da malha de colisão
// =========================================================
struct CollisionTriangle {
    glm::vec3 v0, v1, v2;  // vértices no espaço de mundo
    glm::vec3 normal;       // normal do plano (normalizada)
    float     d;            // distância do plano à origem (normal · v0)
};

// =========================================================
// Grid espacial 2D (XZ) para acelerar queries
// =========================================================
struct GridCell {
    int x, z;
    bool operator==(const GridCell& o) const { return x == o.x && z == o.z; }
};

struct GridCellHash {
    size_t operator()(const GridCell& c) const {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.z) << 16);
    }
};

struct SpatialGrid {
    float cellSize = 2.0f;
    std::unordered_map<GridCell, std::vector<int>, GridCellHash> cells;
    // índices apontam para CollisionMesh::triangles
};

// =========================================================
// Malha de colisão: lista de triângulos + grid
// =========================================================
struct CollisionMesh {
    std::vector<CollisionTriangle> triangles;
    SpatialGrid grid;
};

// =========================================================
// Cilindro do jogador
// =========================================================
struct PlayerCylinder {
    glm::vec3 base;   // centro da base (pé do jogador)
    float     radius;
    float     height;
};

// =========================================================
// API pública
// =========================================================

// Constrói a malha de colisão a partir do asset do mapa.
// mapScale deve ser igual à escala usada no DrawModel do mapa.
CollisionMesh BuildCollisionMesh(const ModelAsset& mapAsset, float mapScale = 0.05f);

// Retorna índices dos triângulos nas células que o cilindro ocupa.
std::vector<int> QueryGrid(const SpatialGrid& grid,
                           const glm::vec3& center, float radius);

// Resolve colisão do cilindro do jogador contra a malha.
// Recebe a posição desejada (após movimento) e devolve a posição corrigida.
// Deve ser chamada iterativamente (2-3 passes) para estabilidade.
glm::vec3 ResolvePlayerCollision(const CollisionMesh& mesh,
                                 glm::vec3 desiredPos,
                                 float radius,
                                 float height);
