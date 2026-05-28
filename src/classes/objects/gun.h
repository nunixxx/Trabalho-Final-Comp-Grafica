#pragma once

#include <string>
#include "world_object.h"

// =========================================================
// Gun
//
// Subclasse concreta de WorldObject.
// Ao ser coletada, o player ganha uma arma (ainda sem funcionalidade de tiro).
// Gira continuamente no eixo Y (comportamento do Doom original).
// =========================================================
class Gun : public WorldObject
{
public:
    int ammoAmount;   // quantidade de munição fornecida ao coletar
    int maxAmmo;      // teto de munição do player (padrão: 100)
    int damage;       // dano causado por cada tiro (ainda sem implementação)
    int range;        // alcance efetivo da arma (ainda sem implementação)
    std::string gunType;   // tipo da arma
    
    // ----------------------------------------------------------
    // Construtor
    // ----------------------------------------------------------
    Gun(
        glm::vec4 position,
        float     scale,
        int       ammoAmount,
        int       maxAmmo,
        int       damage,
        int       range
    );

    // ----------------------------------------------------------
    // Efeito ao coletar
    // ----------------------------------------------------------
    void onInteract(Player& player) override;

};

class ShotGun : public Gun
{
public:
    ShotGun(
        glm::vec4 position,
        float     scale      = 0.05f,
        int       ammoAmount = 5,
        int       maxAmmo    = 20,
        int       damage     = 100,
        int       range      = 10
    ) : Gun(position, scale, ammoAmount, maxAmmo, damage, range) {modelName = "shotgun";}
};

class Pistol : public Gun
{
public:
    Pistol(
        glm::vec4 position,
        float     scale         = 1.0f,
        int       ammoAmount    = 15,
        int       maxAmmo       = 60,
        int       damage        = 35,
        int       range         = 30
    ) : Gun(position, scale, ammoAmount, maxAmmo, damage, range) {modelName = "pistol";}
};