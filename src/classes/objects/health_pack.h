#pragma once

#include "world_object.h"

// =========================================================
// HealthPack
//
// Subclasse concreta de WorldObject.
// Ao ser coletado, cura o player até o limite de maxHealth.
// Gira continuamente no eixo Y.
// =========================================================
class HealthPack : public WorldObject
{
public:
    int healAmount;   // pontos de vida restaurados ao coletar
    int maxHealth;    // teto de HP do player (padrão: 100)

    // ----------------------------------------------------------
    // Construtor
    // ----------------------------------------------------------
    HealthPack(
        glm::vec4 position,
        float     scale      = 0.5f,
        int       healAmount = 25,
        int       maxHealth  = 100
    );

    // ----------------------------------------------------------
    // Efeito ao coletar
    // ----------------------------------------------------------
    // Cura o player, marca consumed = true e desativa o objeto.
    void onInteract(Player& player) override;
};
