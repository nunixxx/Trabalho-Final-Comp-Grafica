#include "health_pack.h"

// =========================================================
// HealthPack — Construtor
// =========================================================
HealthPack::HealthPack(
    glm::vec4 position,
    float     scale,
    int       healAmount,
    int       maxHealth
)
    : WorldObject(
        "healthpack",                           // modelName em g_ModelRegistry
        position,
        Matrix_Scale(scale, scale, scale),      // matrixScale
        1.5f,                                   // rotateSpeed (rad/s) — mesmo valor do main original
        1.2f                                    // interactRadius
    )
    , healAmount(healAmount)
    , maxHealth(maxHealth)
{}

// =========================================================
// HealthPack::onInteract
// Cura o player respeitando o teto de maxHealth,
// marca o objeto como consumido e o desativa.
// =========================================================
void HealthPack::onInteract(Player& player)
{
    if (consumed) return;

    int before = player.health;
    player.health = std::min(player.health + healAmount, maxHealth);
    int gained = player.health - before;

    printf("[HealthPack] Player curado em %d HP. HP atual: %d/%d\n",
           gained, player.health, maxHealth);

    consumed = true;
    active   = false;
}
