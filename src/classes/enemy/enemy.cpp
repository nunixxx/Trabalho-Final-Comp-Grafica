#include "enemy.h"

// Função externa definida em model_rendering.cpp
void DrawModel(const std::string& model_name, glm::mat4 model_matrix);

// =========================================================
// Construtor completo
// =========================================================
Enemy::Enemy(
    glm::vec4                        initialPosition,
    float                            initialYaw,
    const std::array<glm::vec4, 4>&  bezierPoints,
    EnemyState                       initialState,
    float                            visionRadius,
    float                            movementSpeed,
    int                              health
)
    : health(health)
    , movementSpeed(movementSpeed)
    , state(initialState)
    , bezierControlPoints(bezierPoints)
    , bezierT(0.0f)
    , bezierSpeed(0.08f)
    , visionRadius(visionRadius)
    , gunModelName("pistol")
    , gunModelScale(Matrix_Scale(2.0f, 2.0f, 2.0f))
    , verticalVelocity(0.0f)
    , onGround(true)
    , collisionMesh(nullptr)
{
    modelName   = "enemy";
    position    = initialPosition;
    yaw         = initialYaw;
    matrixScale = SOLDIERS_SCALE;
    active      = true;
}

// =========================================================
// Construtor simples
// Gera automaticamente uma Bézier quadrada ao redor do ponto
// inicial para que o inimigo patrulhe sem configuração manual.
//
// Formato da rota gerada (vista de cima):
//
//   P0 (spawn) -----> P1 (frente+direita)
//                          |
//                          v
//   P3 (esquerda) <----- P2 (trás+direita)
//
// =========================================================
Enemy::Enemy(
    glm::vec4 initialPosition,
    float     initialYaw,
    float     patrolRadius,
    EnemyState initialState
)
    : health(100)
    , movementSpeed(2.0f)
    , state(initialState)
    , bezierT(0.0f)
    , bezierSpeed(0.08f)
    , visionRadius(10.0f)
    , gunModelName("pistol")
    , gunModelScale(Matrix_Scale(2.0f, 2.0f, 2.0f))
    , verticalVelocity(0.0f)
    , onGround(true)
    , collisionMesh(nullptr)
{
    modelName   = "enemy";
    position    = initialPosition;
    yaw         = initialYaw;
    matrixScale = SOLDIERS_SCALE;
    active      = true;

    // Gera os 4 pontos de controle ao redor do spawn
    float r = patrolRadius;
    float y = initialPosition.y;

    bezierControlPoints[0] = initialPosition;
    bezierControlPoints[1] = glm::vec4(initialPosition.x + r,  y, initialPosition.z + r,  1.0f);
    bezierControlPoints[2] = glm::vec4(initialPosition.x + r,  y, initialPosition.z - r,  1.0f);
    bezierControlPoints[3] = glm::vec4(initialPosition.x - r,  y, initialPosition.z,      1.0f);
}

// =========================================================
// update (sem player — mantém Patrol)
// =========================================================
void Enemy::update(float deltaTime)
{
    update(deltaTime, nullptr);
}

// =========================================================
// update (com player)
// =========================================================
void Enemy::update(float deltaTime, const Player* player)
{
    if (!active || state == EnemyState::Dead) return;

    if (player)
    {
        glm::vec4 diff     = player->position - position;
        float     distSq   = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
        float     radiusSq = visionRadius * visionRadius;

        if (distSq <= radiusSq)
            state = EnemyState::Chase;
        else if (state == EnemyState::Chase)
            state = EnemyState::Idle;  // perdeu o player, para no lugar
    }

    if (state == EnemyState::Idle)
        return;  // parado, não faz nada
    else if (state == EnemyState::Patrol)
        updatePatrol_(deltaTime);
    else if (state == EnemyState::Chase && player)
        updateChase_(deltaTime, player);
}

// =========================================================
// draw
// Desenha o corpo e a arma com matrizes distintas.
// =========================================================
void Enemy::draw()
{
    if (!active || state == EnemyState::Dead) return;

    DrawModel(modelName,    buildModelMatrix());
    DrawModel(gunModelName, buildGunMatrix_());
}

// =========================================================
// onCollision
// Chamado pelo sistema de projéteis ou pelo Player.
// =========================================================
void Enemy::onCollision(GameObject& other)
{
    // A subclasse de projétil chamará takeDamage() diretamente;
    // este método existe para compatibilidade com a interface base.
    (void)other;
}

// =========================================================
// takeDamage
// =========================================================
void Enemy::takeDamage(int amount)
{
    if (state == EnemyState::Dead) return;

    health -= amount;
    printf("[Enemy] tomou %d de dano. HP restante: %d\n", amount, health);

    if (health <= 0)
    {
        health = 0;
        state  = EnemyState::Dead;
        active = false;
        printf("[Enemy] morreu.\n");
    }
}

// =========================================================
// evaluateBezier_  (privado)
//
// Fórmula da Bézier cúbica:
//   B(t) = (1-t)³·P0 + 3(1-t)²t·P1 + 3(1-t)t²·P2 + t³·P3
// =========================================================
glm::vec4 Enemy::evaluateBezier_(float t) const
{
    float u  = 1.0f - t;
    float u2 = u  * u;
    float u3 = u2 * u;
    float t2 = t  * t;
    float t3 = t2 * t;

    return   u3        * bezierControlPoints[0]
           + 3*u2*t    * bezierControlPoints[1]
           + 3*u *t2   * bezierControlPoints[2]
           + t3        * bezierControlPoints[3];
}

// =========================================================
// updatePatrol_  (privado)
//
// Avança t, avalia a posição na curva e atualiza yaw para
// que o inimigo encare a direção do movimento.
// =========================================================
void Enemy::updatePatrol_(float deltaTime)
{
    const float lookAheadDelta = 0.01f;
    float tNext = bezierT + lookAheadDelta;

    glm::vec4 current = evaluateBezier_(bezierT);
    glm::vec4 next    = evaluateBezier_(std::min(tNext, 1.0f));

    glm::vec4 dir = next - current;
    if (std::abs(dir.x) > 1e-5f || std::abs(dir.z) > 1e-5f)
        yaw = std::atan2(dir.z, dir.x);

    bezierT += bezierSpeed * deltaTime;
    if (bezierT > 1.0f)
        bezierT = 0.0f;

    glm::vec4 pos = evaluateBezier_(bezierT);

    applyGravity_(deltaTime);

    glm::vec3 desiredPos(
        pos.x,
        position.y + verticalVelocity * deltaTime,
        pos.z
    );

    if (collisionMesh)
    {
        glm::vec3 resolved = ResolvePlayerCollision(
            *collisionMesh, desiredPos, PLAYER_RADIUS, PLAYER_HEIGHT);

        if (resolved.y > desiredPos.y) { onGround = true;  verticalVelocity = 0.0f; }
        else if (resolved.y < desiredPos.y)                verticalVelocity = 0.0f;

        position = glm::vec4(resolved, 1.0f);
    }
    else
    {
        position = glm::vec4(desiredPos, 1.0f);
    }
}

// =========================================================
// updateChase_  (privado)
//
// Move o inimigo na direção do player no plano XZ e atualiza
// o yaw para encará-lo.
// =========================================================
void Enemy::updateChase_(float deltaTime, const Player* player)
{
    glm::vec4 diff = player->position - position;

    float distXZ = std::sqrt(diff.x*diff.x + diff.z*diff.z);

    const float stopDistance = 1.5f;
    if (distXZ > stopDistance)
    {
        float invDist = 1.0f / (distXZ + 1e-8f);
        float dx = diff.x * invDist;
        float dz = diff.z * invDist;

        yaw = std::atan2(dz, dx);

        float speed = movementSpeed * deltaTime;
        position.x += dx * speed;
        position.z += dz * speed;
    }

    applyGravity_(deltaTime);

    glm::vec3 desiredPos(position.x, position.y + verticalVelocity * deltaTime, position.z);

    if (collisionMesh)
    {
        glm::vec3 resolved = ResolvePlayerCollision(
            *collisionMesh, desiredPos, PLAYER_RADIUS, PLAYER_HEIGHT);

        if (resolved.y > desiredPos.y) { onGround = true;  verticalVelocity = 0.0f; }
        else if (resolved.y < desiredPos.y)                verticalVelocity = 0.0f;

        position = glm::vec4(resolved, 1.0f);
    }
    else
    {
        position = glm::vec4(desiredPos, 1.0f);
    }
}

// =========================================================
// buildGunMatrix_  (privado)
//
// A arma acompanha o corpo do inimigo: mesma posição e yaw,
// porém com escala própria (gunModelScale).
// =========================================================
glm::mat4 Enemy::buildGunMatrix_() const
{
    return Matrix_Translate(position.x, position.y, position.z)
         * Matrix_Rotate_Y(yaw)
         * gunModelScale;
}

// =========================================================
// applyGravity_  (privado)
// 
// Aplica gravidade ao inimigo, atualizando verticalVelocity e
// position.y.
// =========================================================

void Enemy::applyGravity_(float deltaTime)
{
    verticalVelocity += GRAVITY * deltaTime;
}
