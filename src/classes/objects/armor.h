#pragma once

#include "world_object.h"

// =========================================================
// Armor
//
// Subclasse concreta de WorldObject.
// Ao ser coletado, aumenta a armadura do player até o limite de maxArmor.
// Gira continuamente no eixo Y.
// =========================================================
class Armor : public WorldObject
{
public:
    int armorAmount;
    int maxArmor;

    Armor(
        glm::vec4 position,
        float     scale      = 0.5f,
        int       armorAmount = 25,
        int       maxArmor   = 100
    );

    void onInteract(Player& player) override;
};
