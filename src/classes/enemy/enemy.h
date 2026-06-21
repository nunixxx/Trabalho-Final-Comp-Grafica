#pragma once

#include "../game_object.h"
#include "../player/player.h"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <array>

#include <cmath>
#include <cstdio>
#include <algorithm>

#include "../../constants.h"   // PI, SOLDIERS_SCALE
#include "../../../include/matrices.h"    // Matrix_Translate, Matrix_Rotate_Y, Matrix_Scale

// Forward declaration — evita incluir player.h aqui
class Player;

// =========================================================
// Estados de comportamento do inimigo
// =========================================================
enum class EnemyState
{
    Idle,    // parado
    Patrol,  // percorre a curva de Bézier pré-definida
    Chase,   // detectou o player — se move na direção dele
    Dead     // sem vida — para de atualizar e de desenhar
};

// =========================================================
// Enemy
//
// Responsabilidades:
//   - Patrulhar uma rota definida por curva de Bézier cúbica
//   - Detectar o player dentro de um raio de visão e persegui-lo
//   - Desenhar o corpo ("enemie") e a arma ("enemieGun")
//     com matrizes independentes
//   - Receber dano e morrer
//
// Herda de GameObject: position, yaw, matrixScale, modelName,
// active, update(), draw(), onCollision().
// =========================================================
class Enemy : public GameObject
{
public:
    // ----------------------------------------------------------
    // Stats
    // ----------------------------------------------------------
    int   health;
    float movementSpeed;

    // ----------------------------------------------------------
    // Estado atual de comportamento
    // ----------------------------------------------------------
    EnemyState state;

    // ----------------------------------------------------------
    // Bézier cúbica — 4 pontos de controle no plano XZ
    // O Y é mantido constante (nível do chão) durante o patrol.
    // ----------------------------------------------------------
    std::array<glm::vec4, 4> bezierControlPoints;

    // Parâmetro t ∈ [0, 1] que avança ao longo da curva.
    // Quando chega em 1 reinicia em 0 (loop).
    float bezierT;

    // Velocidade de avanço do t por segundo (1.0 = percorre a
    // curva inteira em 1 segundo; 0.1 = em 10 segundos).
    float bezierSpeed;

    // ----------------------------------------------------------
    // Detecção do player
    // ----------------------------------------------------------
    float visionRadius;  // distância máxima para detectar o player

    // ----------------------------------------------------------
    // Modelo da arma (desenhado separado do corpo)
    // ----------------------------------------------------------
    std::string gunModelName;   // chave em g_ModelRegistry ("enemieGun")
    glm::mat4   gunModelScale;  // escala específica da arma

    // ----------------------------------------------------------
    // Construtores
    // ----------------------------------------------------------

    // Construtor completo — controle total sobre todos os parâmetros
    Enemy(
        glm::vec4                        initialPosition,
        float                            initialYaw,
        const std::array<glm::vec4, 4>&  bezierPoints,
        EnemyState                       initialState  = EnemyState::Idle,
        float                            visionRadius  = 10.0f,
        float                            movementSpeed = 2.0f,
        int                              health        = 100
    );

    // Construtor simples — cria o inimigo parado em uma posição
    explicit Enemy(
        glm::vec4  initialPosition,
        float      initialYaw    = 0.0f,
        float      patrolRadius  = 3.0f,
        EnemyState initialState  = EnemyState::Idle 
    );

    // ----------------------------------------------------------
    // Interface GameObject
    // ----------------------------------------------------------

    // Avança t na Bézier (Patrol) ou persegue o player (Chase).
    // Passa ponteiro nulo se não houver referência ao player ainda;
    // nesse caso o inimigo permanece em Patrol indefinidamente.
    void update(float deltaTime) override;

    // Versão preferencial: recebe o player diretamente.
    void update(float deltaTime, const Player* player);

    // Desenha o corpo e a arma com matrizes separadas.
    void draw() override;

    // Recebe dano de um projétil ou do player.
    void onCollision(GameObject& other) override;

    // Aplica dano diretamente (chamado pelo sistema de projéteis).
    void takeDamage(int amount);

    // ----------------------------------------------------------
    // Helpers de estado
    // ----------------------------------------------------------
    bool isAlive()    const { return state != EnemyState::Dead; }
    bool isPatrol()   const { return state == EnemyState::Patrol; }
    bool isChasing()  const { return state == EnemyState::Chase; }

private:
    // ----------------------------------------------------------
    // Métodos internos
    // ----------------------------------------------------------

    // Avalia a posição na curva de Bézier cúbica para o t atual.
    glm::vec4 evaluateBezier_(float t) const;

    // Avança ao longo da Bézier e atualiza position + yaw.
    void updatePatrol_(float deltaTime);

    // Move em direção ao player e atualiza yaw para encará-lo.
    void updateChase_(float deltaTime, const Player* player);

    // Constrói a model matrix da arma com base na posição e yaw do inimigo.
    glm::mat4 buildGunMatrix_() const;
};
