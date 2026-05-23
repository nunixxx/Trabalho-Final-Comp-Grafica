#pragma once

#include "../modelRendering/model_rendering.h"
#include <unordered_map>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Célula do grid identificada por coordenadas inteiras
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
    float cellSize;
    std::unordered_map<GridCell, std::vector<int>, GridCellHash> cells;
    // indices apontam para CollisionMesh::boxes
};

struct CollisionMesh {
    std::vector<AABB> boxes;
    SpatialGrid grid;
};

bool AABBvsAABB(const AABB& a, const AABB& b);
glm::vec3 ResolveAABB(const AABB& player, const AABB& obstacle);
CollisionMesh BuildCollisionMesh(const ModelAsset& mapAsset);

// Retorna índices das boxes nas células que o jogador ocupa
std::vector<int> QueryGrid(const SpatialGrid& grid, const AABB& playerBox);