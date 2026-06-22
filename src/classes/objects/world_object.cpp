#include "world_object.h"

// Função externa definida em model_rendering.cpp
void DrawModel(const std::string& model_name, glm::mat4 model_matrix);

// =========================================================
// WorldObject — Construtor
// =========================================================
WorldObject::WorldObject(
    const std::string& modelName,
    glm::vec4          position,
    glm::mat4          matrixScale,
    float              rotateSpeed,
    float              interactRadius
)
    : rotateSpeed(rotateSpeed)
    , currentAngle(0.0f)
    , interactRadius(interactRadius)
    , consumed(false)
{
    this->modelName   = modelName;
    this->position    = position;
    this->matrixScale = matrixScale;
    this->active      = true;
}

// =========================================================
// WorldObject::update (sem player — só animação)
// =========================================================
void WorldObject::update(float deltaTime)
{
    if (!active || consumed) return;

    currentAngle += rotateSpeed * deltaTime;

    // Mantém o ângulo em [0, 2π] para evitar overflow em sessões longas
    if (currentAngle > 2.0f * PI)
        currentAngle -= 2.0f * PI;
}

// =========================================================
// WorldObject::update (com player — animação + detecção)
// =========================================================
void WorldObject::update(float deltaTime, Player* player)
{
    if (!active || consumed) return;

    // Avança animação
    update(deltaTime);

    // Testa proximidade com o player
    if (!player) return;

    glm::vec4 diff   = player->position - position;
    float     distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
    float     radSq  = interactRadius * interactRadius;

    if (distSq <= radSq)
        onInteract(*player);
}

// =========================================================
// WorldObject::draw
// =========================================================
void WorldObject::draw()
{
    if (!active || consumed) return;
    std::string model = animator.hasClip(animator.getCurrentClip()) ? animator.getCurrentModel() : modelName;
    DrawModel(model, buildAnimatedMatrix_());
}

// =========================================================
// WorldObject::onCollision
// Interface base — delega para onInteract se o outro objeto
// for um Player. Caso contrário, não faz nada.
// =========================================================
void WorldObject::onCollision(GameObject& other)
{
    // Tenta fazer downcast para Player
    Player* player = dynamic_cast<Player*>(&other);
    if (player)
        onInteract(*player);
}

// =========================================================
// WorldObject::buildAnimatedMatrix_  (privado)
// Usa currentAngle em vez de yaw fixo, para que a animação
// de rotação não interfira na orientação base do objeto.
// =========================================================
glm::mat4 WorldObject::buildAnimatedMatrix_() const
{
    return Matrix_Translate(position.x, position.y, position.z)
         * Matrix_Rotate_Y(currentAngle)
         * matrixScale;
}