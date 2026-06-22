#include "animator.h"

// =========================================================
// Construtor
// =========================================================
Animator::Animator(const std::string& prefix)
    : prefix_(prefix)
    , state_(AnimationState::Idle)
    , time_(0.f)
    , speed(1.0f)
{}

// =========================================================
// setState
// =========================================================
void Animator::setState(AnimationState newState)
{
    if (newState == state_) return;
    state_ = newState;
    time_  = 0.f;  // reinicia o tempo ao trocar de estado
}

// =========================================================
// update
// Avança o tempo e recalcula todas as PartTransforms.
// =========================================================
void Animator::update(float deltaTime)
{
    time_ += deltaTime * speed;

    switch (state_)
    {
        case AnimationState::Idle:     updateIdle_(time_);     break;
        case AnimationState::Walking:  updateWalking_(time_);  break;
        case AnimationState::Shooting: updateShooting_(time_); break;
        case AnimationState::Dead:     updateDead_(time_);     break;
    }
}

// =========================================================
// getPartMatrix
//
// Retorna a model matrix final para a parte solicitada,
// aplicando a PartTransform calculada em update() sobre
// a objectMatrix do objeto inteiro.
// =========================================================
glm::mat4 Animator::getPartMatrix(const glm::mat4& objectMatrix,
                                  const std::string& partName) const
{
    // Mapeia o nome da parte para o PartTransform correto
    const std::string torsoName    = prefix_ + "_torso";
    const std::string headName     = prefix_ + "_head";
    const std::string armRightName = prefix_ + "_arm_right";
    const std::string armLeftName  = prefix_ + "_arm_left";
    const std::string legRightName = prefix_ + "_leg_right";
    const std::string legLeftName  = prefix_ + "_leg_left";

    if      (partName == torsoName)    return buildPartMatrix_(objectMatrix, torso_);
    else if (partName == headName)     return buildPartMatrix_(objectMatrix, head_);
    else if (partName == armRightName) return buildPartMatrix_(objectMatrix, armRight_);
    else if (partName == armLeftName)  return buildPartMatrix_(objectMatrix, armLeft_);
    else if (partName == legRightName) return buildPartMatrix_(objectMatrix, legRight_);
    else if (partName == legLeftName)  return buildPartMatrix_(objectMatrix, legLeft_);

    // Parte desconhecida — retorna a matrix do objeto sem modificação
    return objectMatrix;
}

// =========================================================
// buildPartMatrix_  (privado)
//
// Aplica a PartTransform sobre a matrix do objeto.
//
// A ordem das operações é:
//   1. Ir até o pivô (Translate(pivot))
//   2. Rotacionar em torno do pivô (RotateX * RotateY * RotateZ)
//   3. Voltar do pivô (Translate(-pivot))
//   4. Aplicar deslocamento adicional (Translate(offset))
//
// Tudo isso é pré-multiplicado pela objectMatrix que já
// contém posição, yaw e scale do objeto no mundo.
// =========================================================
glm::mat4 Animator::buildPartMatrix_(const glm::mat4& objectMatrix,
                                     const PartTransform& pt) const
{
    glm::mat4 pivotFwd  = Matrix_Translate( pt.pivot.x,  pt.pivot.y,  pt.pivot.z);
    glm::mat4 pivotBack = Matrix_Translate(-pt.pivot.x, -pt.pivot.y, -pt.pivot.z);
    glm::mat4 offset    = Matrix_Translate( pt.offset.x,  pt.offset.y,  pt.offset.z);

    glm::mat4 rot =
          Matrix_Rotate_X(pt.rotAngles.x)
        * Matrix_Rotate_Y(pt.rotAngles.y)
        * Matrix_Rotate_Z(pt.rotAngles.z);

    // objectMatrix * Translate(offset) * Translate(pivot) * Rotate * Translate(-pivot)
    return objectMatrix * offset * pivotFwd * rot * pivotBack;
}

// =========================================================
// updateIdle_
//
// Animação de respiração suave:
//   - Torso sobe e desce levemente (senoide lenta)
//   - Cabeça balança minimamente
//   - Braços e pernas quase estáticos, leve oscilação
// =========================================================
void Animator::updateIdle_(float t)
{
    const float breathFreq  = 1.2f;   // ciclos por segundo
    const float breathAmp   = 0.04f;  // amplitude do balanço (radianos)
    const float sway        = sinf(t * breathFreq * 2.0f * 3.14159f);

    // Torso — leve inclinação de respiração no eixo Z
    torso_.pivot     = PIVOT_TORSO;
    torso_.rotAngles = { breathAmp * 0.5f * sway, 0.f, breathAmp * 0.3f * sway };
    torso_.offset    = { 0.f, 0.f, 0.f };

    // Cabeça — movimento mínimo
    head_.pivot     = PIVOT_HEAD;
    head_.rotAngles = { breathAmp * 0.3f * sway, 0.f, 0.f };
    head_.offset    = { 0.f, 0.f, 0.f };

    // Braços — pendulam levemente com a respiração
    armRight_.pivot     = PIVOT_ARM_RIGHT;
    armRight_.rotAngles = { breathAmp * 0.4f * sway, 0.f, 0.f };
    armRight_.offset    = { 0.f, 0.f, 0.f };

    armLeft_.pivot     = PIVOT_ARM_LEFT;
    armLeft_.rotAngles = { breathAmp * 0.4f * sway, 0.f, 0.f };
    armLeft_.offset    = { 0.f, 0.f, 0.f };

    // Pernas — estáticas no idle
    legRight_.pivot     = PIVOT_LEG_RIGHT;
    legRight_.rotAngles = { 0.f, 0.f, 0.f };
    legRight_.offset    = { 0.f, 0.f, 0.f };

    legLeft_.pivot     = PIVOT_LEG_LEFT;
    legLeft_.rotAngles = { 0.f, 0.f, 0.f };
    legLeft_.offset    = { 0.f, 0.f, 0.f };
}

// =========================================================
// updateWalking_
//
// Walk cycle clássico:
//   - Pernas alternam frente/trás em senoide (fase oposta)
//   - Braços alternam no sentido oposto às pernas
//   - Torso oscila levemente para os lados
//   - Cabeça acompanha o torso com amortecimento
// =========================================================
void Animator::updateWalking_(float t)
{
    const float freq     = 2.5f;   // passos por segundo
    const float legAmp   = 0.45f;  // amplitude máxima da perna (radianos ~26°)
    const float armAmp   = 0.30f;  // amplitude do braço
    const float torsoAmp = 0.05f;  // oscilação lateral do torso

    const float phase    = t * freq * 2.0f * 3.14159f;
    const float legSwing = sinf(phase);

    // Pernas — fase oposta entre si
    legRight_.pivot     = PIVOT_LEG_RIGHT;
    legRight_.rotAngles = { legAmp * legSwing, 0.f, 0.f };
    legRight_.offset    = { 0.f, 0.f, 0.f };

    legLeft_.pivot     = PIVOT_LEG_LEFT;
    legLeft_.rotAngles = { -legAmp * legSwing, 0.f, 0.f };
    legLeft_.offset    = { 0.f, 0.f, 0.f };

    // Braços — opostos às pernas (braço direito sobe quando perna direita vai pra trás)
    armRight_.pivot     = PIVOT_ARM_RIGHT;
    armRight_.rotAngles = { -armAmp * legSwing, 0.f, 0.f };
    armRight_.offset    = { 0.f, 0.f, 0.f };

    armLeft_.pivot     = PIVOT_ARM_LEFT;
    armLeft_.rotAngles = { armAmp * legSwing, 0.f, 0.f };
    armLeft_.offset    = { 0.f, 0.f, 0.f };

    // Torso — oscilação lateral suave
    const float torsoSway = sinf(phase * 0.5f);  // metade da frequência
    torso_.pivot     = PIVOT_TORSO;
    torso_.rotAngles = { 0.f, 0.f, torsoAmp * torsoSway };
    torso_.offset    = { 0.f, 0.f, 0.f };

    // Cabeça — acompanha o torso com amortecimento
    head_.pivot     = PIVOT_HEAD;
    head_.rotAngles = { 0.f, 0.f, torsoAmp * 0.5f * torsoSway };
    head_.offset    = { 0.f, 0.f, 0.f };
}

// =========================================================
// updateShooting_
//
// Animação de tiro:
//   - Fase 0..0.1s: recuo rápido do braço direito para trás
//   - Fase 0.1..0.4s: retorno suave à posição inicial
//   - Após 0.4s: volta ao idle (o Player gerencia o setState)
//
// Usa uma curva suave baseada em smoothstep para o recuo.
// =========================================================
void Animator::updateShooting_(float t)
{
    const float recoilDuration = 0.08f;   // tempo até o pico do recuo
    const float returnDuration = 0.35f;   // tempo de volta
    const float recoilAngle    = 0.55f;   // ~31° de recuo para cima

    float armAngle = 0.f;

    if (t < recoilDuration)
    {
        // Recuo: sobe rápido
        float u = t / recoilDuration;
        armAngle = -recoilAngle * u;
    }
    else if (t < recoilDuration + returnDuration)
    {
        // Retorno: desce suavemente
        float u = (t - recoilDuration) / returnDuration;
        // Smoothstep para retorno mais natural
        float smooth = u * u * (3.f - 2.f * u);
        armAngle = -recoilAngle * (1.f - smooth);
    }
    // Após o retorno: ângulo permanece em 0 até o Player trocar de estado

    // Aplica recuo ao braço esquerdo (que segura a arma na perspectiva do modelo)
    armLeft_.pivot     = PIVOT_ARM_LEFT;
    armLeft_.rotAngles = { armAngle, 0.f, 0.f };
    armLeft_.offset    = { 0.f, 0.f, 0.f };

    // Braço direito estático durante o tiro
    armRight_.pivot     = PIVOT_ARM_RIGHT;
    armRight_.rotAngles = { 0.f, 0.f, 0.f };
    armRight_.offset    = { 0.f, 0.f, 0.f };

    // Torso recua levemente
    torso_.pivot     = PIVOT_TORSO;
    torso_.rotAngles = { armAngle * 0.2f, 0.f, 0.f };
    torso_.offset    = { 0.f, 0.f, 0.f };

    // Cabeça e pernas estáticas
    head_.pivot     = PIVOT_HEAD;
    head_.rotAngles = { 0.f, 0.f, 0.f };
    head_.offset    = { 0.f, 0.f, 0.f };

    legRight_.pivot     = PIVOT_LEG_RIGHT;
    legRight_.rotAngles = { 0.f, 0.f, 0.f };
    legRight_.offset    = { 0.f, 0.f, 0.f };

    legLeft_.pivot     = PIVOT_LEG_LEFT;
    legLeft_.rotAngles = { 0.f, 0.f, 0.f };
    legLeft_.offset    = { 0.f, 0.f, 0.f };
}

// =========================================================
// updateDead_
//
// Animação de morte:
//   - Torso cai para frente ao longo de 0.5s
//   - Pernas e braços acompanham com leve atraso
//   - Depois de cair (>0.5s) permanece estático no chão
// =========================================================
void Animator::updateDead_(float t)
{
    const float fallDuration = 0.5f;
    const float maxFall      = 1.45f;  // ~83° — quase deitado

    float u = (t < fallDuration) ? (t / fallDuration) : 1.f;
    // Ease-out: começa rápido, desacelera ao tocar o chão
    float smooth = 1.f - (1.f - u) * (1.f - u);

    float fallAngle = maxFall * smooth;

    // Torso cai para frente
    torso_.pivot     = PIVOT_TORSO;
    torso_.rotAngles = { fallAngle, 0.f, 0.f };
    torso_.offset    = { 0.f, 0.f, 0.f };

    // Cabeça cai junto, com leve atraso (usa u com delay)
    float headU      = (t < fallDuration * 1.1f) ? (t / (fallDuration * 1.1f)) : 1.f;
    float headSmooth = 1.f - (1.f - headU) * (1.f - headU);
    head_.pivot     = PIVOT_HEAD;
    head_.rotAngles = { maxFall * 0.6f * headSmooth, 0.f, 0.f };
    head_.offset    = { 0.f, 0.f, 0.f };

    // Braços abrem ao cair
    armRight_.pivot     = PIVOT_ARM_RIGHT;
    armRight_.rotAngles = { fallAngle * 0.5f, 0.f, fallAngle * 0.3f };
    armRight_.offset    = { 0.f, 0.f, 0.f };

    armLeft_.pivot     = PIVOT_ARM_LEFT;
    armLeft_.rotAngles = { fallAngle * 0.5f, 0.f, -fallAngle * 0.3f };
    armLeft_.offset    = { 0.f, 0.f, 0.f };

    // Pernas dobram levemente
    legRight_.pivot     = PIVOT_LEG_RIGHT;
    legRight_.rotAngles = { -fallAngle * 0.3f, 0.f, 0.f };
    legRight_.offset    = { 0.f, 0.f, 0.f };

    legLeft_.pivot     = PIVOT_LEG_LEFT;
    legLeft_.rotAngles = { -fallAngle * 0.3f, 0.f, 0.f };
    legLeft_.offset    = { 0.f, 0.f, 0.f };
}