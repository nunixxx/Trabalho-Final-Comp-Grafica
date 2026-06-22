#pragma once

#include <string>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <cmath>

#include "../include/matrices.h"   // Matrix_Translate, Matrix_Rotate_X/Y/Z, Matrix_Scale

// =========================================================
// AnimationState
//
// Enum com todos os estados de animação possíveis.
// O Animator usa isso para saber qual lógica rodar por frame.
// =========================================================
enum class AnimationState
{
    Idle,       // parado, respiração leve
    Walking,    // andando — walk cycle das pernas e balanço dos braços
    Shooting,   // recuo do braço direito
    Dead,       // colapsa no chão
};

// =========================================================
// PartTransform
//
// Transformação de uma parte do modelo relativa ao torso.
// A matrix final aplicada à parte é:
//
//   world = objectModel * Translate(pivot) * Rotate * Translate(-pivot) * Translate(offset)
//
// onde objectModel é a matrix do objeto inteiro (posição, yaw, scale).
// =========================================================
struct PartTransform
{
    glm::vec3 pivot;        // ponto de rotação no espaço local do modelo
    glm::vec3 rotAngles;    // rotação em X, Y, Z (radianos)
    glm::vec3 offset;       // deslocamento adicional (para separar a parte do pivot)

    PartTransform()
        : pivot(0.f), rotAngles(0.f), offset(0.f)
    {}
};

// =========================================================
// Animator
//
// Calcula as matrizes de cada parte do soldier/enemy
// com base no estado e no tempo acumulado (deltaTime).
//
// Uso:
//   1. Chame setState() quando o personagem mudar de ação.
//   2. Chame update(deltaTime) a cada frame.
//   3. Chame getPartMatrix(objectModelMatrix, partName) para
//      obter a matrix final de cada parte antes de desenhar.
//
// As partes esperadas são:
//   "soldier_torso", "soldier_head",
//   "soldier_arm_right", "soldier_arm_left",
//   "soldier_leg_right", "soldier_leg_left"
//
// (ou prefixo "enemy_" para os inimigos, configurável)
// =========================================================
class Animator
{
public:
    // ----------------------------------------------------------
    // Construtor
    // prefix: "soldier" ou "enemy" — prefixo dos nomes das partes
    // ----------------------------------------------------------
    explicit Animator(const std::string& prefix = "soldier");

    // ----------------------------------------------------------
    // Atualiza o tempo interno e recalcula as transformações.
    // Deve ser chamado a cada frame com o deltaTime do loop.
    // ----------------------------------------------------------
    void update(float deltaTime);

    // ----------------------------------------------------------
    // Muda o estado de animação.
    // A transição é imediata — sem blending por enquanto.
    // ----------------------------------------------------------
    void setState(AnimationState newState);

    AnimationState getState() const { return state_; }

    // ----------------------------------------------------------
    // Retorna a model matrix final de uma parte específica.
    //
    // objectMatrix — matrix do objeto inteiro (translate * rotateY * scale)
    //                exatamente o que você passaria para DrawModel normalmente
    // partName     — nome completo da parte, ex: "soldier_leg_right"
    //
    // Internamente aplica: objectMatrix * pivotRotation * offsetTranslation
    // ----------------------------------------------------------
    glm::mat4 getPartMatrix(const glm::mat4& objectMatrix,
                            const std::string& partName) const;

    // ----------------------------------------------------------
    // Velocidade de reprodução (1.0 = normal, 2.0 = dobro da vel.)
    // ----------------------------------------------------------
    float speed;

private:
    std::string   prefix_;
    AnimationState state_;
    float          time_;    // tempo acumulado dentro do estado atual

    // Transformações calculadas por update() para cada parte
    PartTransform torso_;
    PartTransform head_;
    PartTransform armRight_;
    PartTransform armLeft_;
    PartTransform legRight_;
    PartTransform legLeft_;

    // Pivôs em espaço LOCAL do modelo (calculados a partir do .obj)
    // Esses valores foram extraídos da geometria do soldier.obj
    // e representam os pontos de articulação de cada parte.
    static constexpr float SCALE = 0.2f;  // deve bater com SOLDIERS_SCALE

    // Pivôs no espaço do modelo (antes da escala SOLDIERS_SCALE)
    // Usamos o topo de cada parte como pivô de rotação.
    // Cabeça: base do pescoço (~y=6.43 no espaço local)
    const glm::vec3 PIVOT_HEAD      = { 0.00f,  6.43f,  0.08f };
    // Braços: ombro (~y=6.50, x=±0.93)
    const glm::vec3 PIVOT_ARM_RIGHT = {-0.93f,  6.43f,  0.18f };
    const glm::vec3 PIVOT_ARM_LEFT  = { 0.93f,  6.43f,  0.18f };
    // Pernas: quadril (~y=3.48)
    const glm::vec3 PIVOT_LEG_RIGHT = {-0.91f,  3.48f,  0.71f };
    const glm::vec3 PIVOT_LEG_LEFT  = { 0.91f,  3.48f,  0.71f };
    // Torso: centro (~y=5.02)
    const glm::vec3 PIVOT_TORSO     = { 0.00f,  5.02f,  0.08f };

    // ----------------------------------------------------------
    // Funções de animação por estado
    // ----------------------------------------------------------
    void updateIdle_(float t);
    void updateWalking_(float t);
    void updateShooting_(float t);
    void updateDead_(float t);

    // Constrói a matrix de uma parte a partir do seu PartTransform
    glm::mat4 buildPartMatrix_(const glm::mat4& objectMatrix,
                               const PartTransform& pt) const;
};