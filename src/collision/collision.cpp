#include "collision.h"

bool AABBvsAABB(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

glm::vec3 ResolveAABB(const AABB& player, const AABB& obstacle) {
    // Calcula sobreposição em cada eixo
    float dx1 = obstacle.max.x - player.min.x;
    float dx2 = obstacle.min.x - player.max.x;
    float dy1 = obstacle.max.y - player.min.y;
    float dy2 = obstacle.min.y - player.max.y;
    float dz1 = obstacle.max.z - player.min.z;
    float dz2 = obstacle.min.z - player.max.z;

    // Menor sobreposição por eixo
    float dx = (abs(dx1) < abs(dx2)) ? dx1 : dx2;
    float dy = (abs(dy1) < abs(dy2)) ? dy1 : dy2;
    float dz = (abs(dz1) < abs(dz2)) ? dz1 : dz2;

    // Empurra pelo eixo de menor penetração (ignora Y para mover só no plano)
    if (abs(dx) <= abs(dz))
        return glm::vec3(dx, 0.0f, 0.0f);
    else
        return glm::vec3(0.0f, 0.0f, dz);
}

CollisionMesh BuildCollisionMesh(const ModelAsset& mapAsset) {
    CollisionMesh mesh;
    float mapScale = 0.01f;

    // --- Igual ao anterior: constrói as boxes ---
    for (const SceneObject& obj : mapAsset.parts) {
        AABB box;
        box.min = obj.bbox_min * mapScale;
        box.max = obj.bbox_max * mapScale;

        if (box.max.x - box.min.x < 0.001f &&
            box.max.z - box.min.z < 0.001f)
            continue;

        mesh.boxes.push_back(box);
    }

    // --- NOVO: popula o grid espacial ---
    mesh.grid.cellSize = 2.0f; // tamanho de cada célula em unidades de mundo

    for (int i = 0; i < (int)mesh.boxes.size(); i++) {
        const AABB& box = mesh.boxes[i];

        // Quais células esta box ocupa? (pode ocupar mais de uma)
        int x0 = (int)floor(box.min.x / mesh.grid.cellSize);
        int x1 = (int)floor(box.max.x / mesh.grid.cellSize);
        int z0 = (int)floor(box.min.z / mesh.grid.cellSize);
        int z1 = (int)floor(box.max.z / mesh.grid.cellSize);

        for (int cx = x0; cx <= x1; cx++)
            for (int cz = z0; cz <= z1; cz++)
                mesh.grid.cells[{cx, cz}].push_back(i);
    }

    printf("Grid: %zu células, %zu boxes.\n",
           mesh.grid.cells.size(), mesh.boxes.size());

    return mesh;
}

std::vector<int> QueryGrid(const SpatialGrid& grid, const AABB& playerBox) {
    std::vector<int> candidates;

    // Células que o jogador ocupa
    int x0 = (int)floor(playerBox.min.x / grid.cellSize);
    int x1 = (int)floor(playerBox.max.x / grid.cellSize);
    int z0 = (int)floor(playerBox.min.z / grid.cellSize);
    int z1 = (int)floor(playerBox.max.z / grid.cellSize);

    for (int cx = x0; cx <= x1; cx++) {
        for (int cz = z0; cz <= z1; cz++) {
            auto it = grid.cells.find({cx, cz});
            if (it == grid.cells.end()) continue;

            for (int idx : it->second) {
                // Evita duplicatas (box pode estar em múltiplas células)
                bool already = false;
                for (int c : candidates)
                    if (c == idx) { already = true; break; }
                if (!already)
                    candidates.push_back(idx);
            }
        }
    }

    return candidates;
}