#include "gun.h"

// =========================================================
// Gun — Construtor
// =========================================================
Gun::Gun(
    glm::vec4 position,
    float     scale,
    int       ammoAmount,
    int       maxAmmo,
    int       damage,
    int       range
)
    : WorldObject(
        modelName,                                  // modelName em g_ModelRegistry
        position,
        Matrix_Scale(scale, scale, scale),      // matrixScale
        1.5f,                                   // rotateSpeed (rad/s) — mesmo valor do main original
        1.2f                                    // interactRadius
    )
    , ammoAmount(ammoAmount) 
    , maxAmmo(maxAmmo)
    , damage(damage)
    , range(range)
{}

// =========================================================
// Gun::onInteract
// Adiciona uma arma ao inventário do player, respeitando o teto de maxAmmo,
// marca o objeto como consumido e o desativa.
// =========================================================
void Gun::onInteract(Player& player)
{
    if (consumed) return;

    int before = player.ammo;
    player.ammo = std::min(player.ammo + ammoAmount, maxAmmo);
    int gained = player.ammo - before;

    printf("[Gun] Player coletou uma arma! Munição aumentada em %d. Munição atual: %d/%d\n",
           gained, player.ammo, maxAmmo);

    consumed = true;
    active   = false;
}