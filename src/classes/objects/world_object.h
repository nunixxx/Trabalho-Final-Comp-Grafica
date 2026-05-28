#pragma once

#include "../game_object.h"
#include "../player/player.h"

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "../../constants.h"   // PI
#include "../../../include/matrices.h"    // Matrix_Translate, Matrix_Rotate_Y

// Forward declaration
class Player;

// =========================================================
// WorldObject
//
// Classe intermediária para objetos estáticos ou interativos
// do cenário: healthpacks, armas no chão, armaduras, chaves.
//
// Adiciona sobre GameObject:
//   - Rotação animada por tempo (rotateSpeed)
//   - Raio de interação com o player (interactRadius)
//   - Callback onInteract — chamado quando o player entra
//     no raio. Subclasses sobrescrevem para definir o efeito.
//
// O update() já cuida da animação e da detecção do player.
// Subclasses geralmente só precisam sobrescrever onInteract().
// =========================================================
class WorldObject : public GameObject
{
public:
    // ----------------------------------------------------------
    // Animação
    // ----------------------------------------------------------
    float rotateSpeed;      // radianos por segundo (0 = estático)
    float currentAngle;     // ângulo acumulado — atualizado em update()

    // ----------------------------------------------------------
    // Interação com o player
    // ----------------------------------------------------------
    float interactRadius;   // distância máxima para ativar o objeto
    bool  consumed;         // true = já foi coletado/usado, não interage mais

    // ----------------------------------------------------------
    // Construtor
    // ----------------------------------------------------------
    WorldObject(
        const std::string& modelName,
        glm::vec4          position,
        glm::mat4          matrixScale,
        float              rotateSpeed   = 0.0f,
        float              interactRadius = 1.2f
    );

    // ----------------------------------------------------------
    // Interface GameObject
    // ----------------------------------------------------------

    // Avança a animação e testa proximidade com o player.
    // Subclasses devem chamar WorldObject::update() no próprio
    // update() se quiserem manter a animação base.
    void update(float deltaTime) override;

    // Versão preferencial: recebe o player para testar colisão.
    void update(float deltaTime, Player* player);

    // draw() usa buildModelMatrix() com o ângulo animado —
    // sobrescreva apenas se o objeto tiver múltiplas partes.
    void draw() override;

    // Chamado automaticamente quando o player entra no raio.
    // Subclasses sobrescrevem para aplicar o efeito (cura, munição, etc).
    virtual void onInteract(Player& player) {}

    // Chamado pelo sistema de colisão genérico (interface base).
    void onCollision(GameObject& other) override;

private:
    // Monta a matrix com o ângulo animado atual
    glm::mat4 buildAnimatedMatrix_() const;
};