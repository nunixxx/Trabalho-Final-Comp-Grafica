#include "player.h"

// Função externa definida em model_rendering.cpp
void DrawModel(const std::string& model_name, glm::mat4 model_matrix);

// =========================================================
// Construtor
// =========================================================
Player::Player(
    GLFWwindow*          window,
    const CollisionMesh* collisionMesh,
    glm::vec4            initialPosition,
    float                initialYaw
)
    : window(window)
    , collisionMesh(collisionMesh)
    // --- stats ---
    , health(PLAYER_INITIAL_HEALTH)
    , armor(PLAYER_INITIAL_ARMOR)
    , ammo(5)  // começa com 15 balas — player deve coletar mais
    // --- movimento ---
    , movementSpeed(PLAYER_INITIAL_SPEED)
    , verticalVelocity(0.0f)
    , onGround(true)
    , shootCooldown(0.0f)
    // --- câmera LookAt ---
    , cameraPhi(INITIAL_CAMERA_PHI)
    , cameraDistance(INITIAL_CAMERA_DISTANCE)
    // --- animação ---
    , animator("soldier")
    // --- câmera FreeCam ---
    , CamPosition(initialPosition)
    , CamYaw(initialYaw)
    , CamPitch(0.0f)
    // --- modo inicial ---
    , cameraMode(CameraMode::LookAt)
    // --- saídas ---
    , cameraPosition(initialPosition)
    , cameraViewVector(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f))
    // --- cursor ---
    , lastCursorX_(0.0)
    , lastCursorY_(0.0)
{
    // Inicializa dados da base GameObject
    modelName   = "soldier";
    position    = initialPosition;
    yaw         = initialYaw;
    matrixScale = SOLDIERS_SCALE;
    active      = true;

    lookAtTarget_ = position + glm::vec4(0.0f, 1.8f, 0.0f, 0.0f);

    // Captura posição inicial do cursor para evitar salto no primeiro frame
    if (window)
        glfwGetCursorPos(window, &lastCursorX_, &lastCursorY_);
}

// =========================================================
// update
// Chamado a cada frame pelo loop principal.
// deltaTime em segundos.
// =========================================================
void Player::update(float deltaTime)
{
    if (!active) return;

    // --- Detecta estado de animação ---
    bool isMoving = false;
    if (window)
    {
        isMoving = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
    }

    // Shooting tem prioridade sobre Walking
    if (animator.getState() == AnimationState::Shooting)
    {
        // Volta ao idle/walk após a animação de tiro terminar (~0.45s)
        // O Animator continua rodando; verificamos o tempo via estado
        // Solução simples: usar um timer externo no Player
        // (veja shootCooldown que já existe — reutilizamos ele)
        if (shootCooldown <= 0.f)
            animator.setState(isMoving ? AnimationState::Walking : AnimationState::Idle);
    }
    else
    {
        animator.setState(isMoving ? AnimationState::Walking : AnimationState::Idle);
    }

    // Atualiza o animator
    animator.update(deltaTime);

    // Atualiza cooldown de tiro
    if (shootCooldown > 0.0f)
        shootCooldown -= deltaTime;
    
    if (cameraMode == CameraMode::LookAt)
    {
        handleMovementLookAt_(deltaTime);
        updateLookAtCamera_();
    }
    else  // FirstPerson
    {
        handleMovementFirstPerson_(deltaTime);
        updateFirstPersonCamera_();
    }
}

// =========================================================
// draw
// Em LookAt: desenha o modelo do soldado no mundo.
// Em FirstPerson: jogador esta nos olhos do solado
// =========================================================
void Player::draw()
{
    if (!active) return;
    if (cameraMode == CameraMode::FirstPerson)
    {
        // Direção frontal da câmera (pitch + yaw)
        glm::vec3 front(
            cos(CamPitch) * cos(CamYaw),
            sin(CamPitch),
            cos(CamPitch) * sin(CamYaw)
        );

        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
        glm::vec3 up    = glm::normalize(glm::cross(right, front));

        glm::vec3 eyePos(cameraPosition.x, cameraPosition.y, cameraPosition.z);

        float forwardOffset = 0.6f;
        float rightOffset   = 0.25f;
        float downOffset    = -0.25f;

        glm::vec3 gunPos = eyePos
            + front * forwardOffset
            + right * rightOffset
            + up    * downOffset;

        // Monta uma matriz de rotação com os vetores da câmera
        // para que a arma sempre aponte exatamente para onde a câmera olha
        glm::mat4 rotMatrix = glm::mat4(
            glm::vec4(right,   0.0f),   // eixo X = direita da câmera
            glm::vec4(up,      0.0f),   // eixo Y = cima da câmera
            glm::vec4(-front,  0.0f),   // eixo Z = oposto ao front (OpenGL right-hand)
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );

        glm::mat4 gunMatrix =
            Matrix_Translate(gunPos.x, gunPos.y, gunPos.z)
            * rotMatrix
            * Matrix_Rotate_Y(-PI/2.0f)  // arma nasce deitada no chão, precisa rotacionar
            * Matrix_Scale(2.0f, 2.0f, 2.0f);

        DrawModel("pistol", gunMatrix);
        return;
    }

    // Monta a matrix base do objeto (igual ao buildModelMatrix())
    glm::mat4 base = Matrix_Translate(position.x, position.y, position.z)
                   * Matrix_Rotate_Y(yaw)
                   * matrixScale;

    // Desenha cada parte com sua matrix animada
    DrawModelPart("soldier", "soldier_torso",     animator.getPartMatrix(base, "soldier_torso"));
    DrawModelPart("soldier", "soldier_head",      animator.getPartMatrix(base, "soldier_head"));
    DrawModelPart("soldier", "soldier_arm_right", animator.getPartMatrix(base, "soldier_arm_right"));
    DrawModelPart("soldier", "soldier_arm_left",  animator.getPartMatrix(base, "soldier_arm_left"));
    DrawModelPart("soldier", "soldier_leg_right", animator.getPartMatrix(base, "soldier_leg_right"));
    DrawModelPart("soldier", "soldier_leg_left",  animator.getPartMatrix(base, "soldier_leg_left"));
    DrawModelPart("soldier", "Sphere",           animator.getPartMatrix(base, "Sphere"));

}
// =========================================================
// onCollision
// =========================================================
void Player::onCollision(GameObject& other)
{
    // A resolução de colisão com o mapa já acontece em handleMovementLookAt_()
    // via ResolvePlayerCollision. Este método é reservado para colisões com
    // outros GameObjects (inimigos, itens), que as subclasses dispararão.
    (void)other;
}

// =========================================================
// onMouseDrag
// Chamado pelo CursorPosCallback do main com as posições atuais do cursor.
// =========================================================
void Player::onMouseDrag(float dx, float dy)
{
    if (cameraMode == CameraMode::LookAt)
    {
        yaw      -= 0.01f * dx;
        cameraPhi += 0.01f * dy;
        cameraPhi = std::max(-PI/2.0f, std::min(PI/2.0f, cameraPhi));
    }
    else // FirstPerson
    {
        CamYaw   += 0.003f * dx;   // note: -= para que mover direita gire direita
        CamPitch -= 0.003f * dy;
        CamPitch = std::max(-PI/2.0f + 0.01f, std::min(PI/2.0f - 0.01f, CamPitch));
    }
}

// =========================================================
// onScroll
// =========================================================
void Player::onScroll(float yoffset)
{
    if (cameraMode != CameraMode::LookAt) return;

    cameraDistance -= 0.5f * yoffset;
    const float verySmall = std::numeric_limits<float>::epsilon();
    if (cameraDistance < verySmall)
        cameraDistance = verySmall;
}

// =========================================================
// toggleCameraMode
// =========================================================
void Player::toggleCameraMode()
{
    if (cameraMode == CameraMode::LookAt)
    {
        // Herda transform do personagem — câmera nasce "dentro" dele
        CamPosition = position;
        CamYaw   = yaw - PI / 2.0f;
        CamPitch    = 0.0f;

        cameraMode = CameraMode::FirstPerson;

        if (window)
        {
            glfwGetCursorPos(window, &lastCursorX_, &lastCursorY_);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    else // FirstPerson → LookAt
    {
        yaw = CamYaw + PI / 2.0f;   // player herda direção que estava olhando

        cameraPhi      = INITIAL_CAMERA_PHI;
        cameraDistance = INITIAL_CAMERA_DISTANCE;
        lookAtTarget_  = position + glm::vec4(0.0f, 1.8f, 0.0f, 0.0f);

        printf("\n[POSICAO ATUAL]\n");
        printf("position = glm::vec4(%.2ff, %.2ff, %.2ff, 1.0f);\n",
               position.x, position.y, position.z);
        printf("yaw      = %.2ff;\n\n", yaw);
        fflush(stdout);

        cameraMode = CameraMode::LookAt;

        if (window)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

// =========================================================
// updateLookAtCamera_  (privado)
// =========================================================
void Player::updateLookAtCamera_()
{
    float r = cameraDistance;
    float y = r * sin(cameraPhi);

    float behindAngle = yaw + PI;
    float z = r * cos(cameraPhi) * cos(behindAngle);
    float x = r * cos(cameraPhi) * sin(behindAngle);

    lookAtTarget_  = position + glm::vec4(0.0f, 1.8f, 0.0f, 0.0f);

    // posição ideal (sem colisão) 
    glm::vec3 target   = glm::vec3(lookAtTarget_);
    glm::vec3 idealPos = target + glm::vec3(x, y, z);

    // distância segura com colisão
    float safeDist = cameraDistance;
    if (collisionMesh)
    {
        safeDist = SafeCameraDistance(
            *collisionMesh, target, idealPos, cameraDistance);
    }

    // suavização da distância (evita "teleporte")
    // Aproxima instantaneamente quando a câmera colide (pull-in),
    // mas retorna devagar quando o obstáculo sai do caminho (pull-out).
    static float smoothDist = cameraDistance;
    if (safeDist < smoothDist)
        smoothDist = safeDist;                          // pull-in: imediato
    else
        smoothDist += (safeDist - smoothDist) * 0.08f; // pull-out: suave

    // Reconstrói a posição final com a distância suavizada
    float ratio   = smoothDist / (cameraDistance + 1e-6f);
    glm::vec4 finalPos = lookAtTarget_ + glm::vec4(x * ratio, y * ratio, z * ratio, 0.0f);

    cameraPosition   = finalPos;
    cameraViewVector = lookAtTarget_ - cameraPosition;
}

// =========================================================
// updateFirstPersonCamera_  (privado)
// =========================================================
void Player::updateFirstPersonCamera_()
{
    // Câmera na altura dos olhos
    const float eyeHeight = 1.7f;
    cameraPosition = position + glm::vec4(0.0f, eyeHeight, 0.0f, 0.0f);

    glm::vec4 front;
    front.x = cos(CamPitch) * cos(CamYaw);
    front.y = sin(CamPitch);
    front.z = cos(CamPitch) * sin(CamYaw);
    front.w = 0.0f;

    cameraViewVector = front;
}

// =========================================================
// handleMovementLookAt_  (privado)
// =========================================================
void Player::handleMovementLookAt_(float deltaTime)
{
    if (!window) return;

    // Direção frontal relativa à câmera no plano XZ.
    // A câmera está sempre atrás do personagem (ângulo yaw + PI),
    // portanto a direção câmera→jogador (para onde a câmera aponta)
    // no plano XZ é (sin(yaw), 0, cos(yaw)).
    glm::vec3 frontXZ(sin(yaw), 0.0f, cos(yaw));
    glm::vec3 rightXZ(-frontXZ.z, 0.0f, frontXZ.x);  // direita no espaço da câmera

    float speed = movementSpeed * deltaTime * 60.0f;

    // Movimento XZ vindo do WASD
    glm::vec3 moveXZ(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveXZ += frontXZ;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveXZ -= frontXZ;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveXZ -= rightXZ;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveXZ += rightXZ;

    float len = glm::length(moveXZ);
    if (len > 0.0001f) moveXZ *= speed / len;

    // Aplica gravidade e pulo
    applyGravity_(deltaTime);
    handleJump_();

    // Posição desejada combinando XZ (WASD) e Y (gravidade/pulo)
    glm::vec3 desiredPos(
        position.x + moveXZ.x,
        position.y + verticalVelocity * deltaTime,
        position.z + moveXZ.z
    );

    // Resolve colisão com a malha do mapa
    if (collisionMesh)
    {
        glm::vec3 resolved = ResolvePlayerCollision(
            *collisionMesh,
            desiredPos,
            PLAYER_RADIUS,
            PLAYER_HEIGHT
        );

        // Detecta contato com o chão
        if (resolved.y > desiredPos.y)
        {
            onGround = true;
            verticalVelocity = 0.0f;
        }
        else if (resolved.y < desiredPos.y)
        {
            // Colidiu com teto
            verticalVelocity = 0.0f;
        }

        position = glm::vec4(resolved.x, resolved.y, resolved.z, 1.0f);
    }
    else
    {
        position = glm::vec4(desiredPos.x, desiredPos.y, desiredPos.z, 1.0f);
    }
}

// =========================================================
// handleMovementFirstPerson_  (privado)
// =========================================================
void Player::handleMovementFirstPerson_(float deltaTime)
{
    if (!window) return;

    // Direção frontal no plano XZ baseada no yaw atual
    glm::vec3 frontXZ(cos(CamYaw), 0.0f, sin(CamYaw));
    glm::vec3 rightXZ(-frontXZ.z, 0.0f, frontXZ.x);

    float speed = movementSpeed * deltaTime * 60.0f;

    glm::vec3 moveXZ(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveXZ += frontXZ;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveXZ -= frontXZ;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveXZ -= rightXZ;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveXZ += rightXZ;

    float len = glm::length(moveXZ);
    if (len > 0.0001f) moveXZ *= speed / len;

    applyGravity_(deltaTime);
    handleJump_();

    glm::vec3 desiredPos(
        position.x + moveXZ.x,
        position.y + verticalVelocity * deltaTime,
        position.z + moveXZ.z
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

    // Sincroniza yaw do player com a câmera
    yaw = CamYaw;
}

// =========================================================
// applyGravity_  (privado)
// Acumula aceleração gravitacional na velocidade vertical.
// =========================================================
void Player::applyGravity_(float deltaTime)
{
    verticalVelocity += GRAVITY * deltaTime;
}

// =========================================================
// handleJump_  (privado)
// Inicia um pulo se o jogador estiver no chão e Space for pressionado.
// =========================================================
bool Player::handleJump_()
{
    if (onGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        verticalVelocity = JUMP_VELOCITY;
        onGround = false;
        return true;
    }
    return false;
}

// =========================================================
// shoot (publico)
// Verifica se o tiro pode ser disparado (cooldown e munição), então dispara um raio
// da posição da câmera na direção que ela aponta, testando colisão com inimigos.
// =========================================================

void Player::shoot(std::vector<std::unique_ptr<Enemy>>& enemies,
                   const std::map<std::string, ModelAsset>& modelRegistry)
{
    if (shootCooldown > 0.0f || ammo <= 0) return;

    shootCooldown = 0.5f;  // meio segundo entre tiros
    ammo--;

    // Origem e direção do raio (da câmera para onde está olhando)
    glm::vec3 origin(cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glm::vec3 dir(cameraViewVector.x, cameraViewVector.y, cameraViewVector.z);
    float dirLen = glm::length(dir);
    if (dirLen < 1e-8f) return;
    dir /= dirLen;

    // Testa contra cada inimigo vivo
    float  closestT   = std::numeric_limits<float>::max();
    int    closestIdx = -1;

    for (int i = 0; i < (int)enemies.size(); i++) 
    {
        if (!enemies[i]->active) continue;

        // Pega bbox do modelo do inimigo
        auto it = modelRegistry.find(enemies[i]->modelName);
        if (it == modelRegistry.end() || it->second.parts.empty()) continue;

        // Calcula AABB em espaço de mundo
        glm::vec3 bmin(std::numeric_limits<float>::max());
        glm::vec3 bmax(std::numeric_limits<float>::lowest());
        for (const auto& part : it->second.parts)
        {
            bmin = glm::min(bmin, part.bbox_min);
            bmax = glm::max(bmax, part.bbox_max);
        }

        // Aplica escala e posição do inimigo
        glm::vec3 scale(enemies[i]->matrixScale[0][0],
                        enemies[i]->matrixScale[1][1],
                        enemies[i]->matrixScale[2][2]);
        glm::vec3 pos(enemies[i]->position.x,
                      enemies[i]->position.y,
                      enemies[i]->position.z);

        glm::vec3 worldMin = pos + bmin * scale;
        glm::vec3 worldMax = pos + bmax * scale;

        float t;
        if (RayVsAABB(origin, dir, worldMin, worldMax, t))
        {
            if (t < closestT)
            {
                closestT   = t;
                closestIdx = i;
            }
        }
    }

    if (closestIdx >= 0)
    {
        enemies[closestIdx]->takeDamage(35, this);
        printf("[Player] Acertou inimigo %d! Dano: 35\n", closestIdx);
    }
    else
    {
        printf("[Player] Errou!\n");
    }
}
