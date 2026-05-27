#include "player.h"

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <limits>

#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

#include "../constants.h"   // PLAYER_RADIUS, PLAYER_HEIGHT, SOLDIERS_SCALE, etc.

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
    // --- movimento ---
    , movementSpeed(PLAYER_INITIAL_SPEED)
    // --- câmera LookAt ---
    , cameraPhi(INITIAL_CAMERA_PHI)
    , cameraDistance(INITIAL_CAMERA_DISTANCE)
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

    lookAtTarget_ = position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);

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

    if (cameraMode == CameraMode::LookAt)
    {
        handleMovementLookAt_(deltaTime);
        updateLookAtCamera_();
    }
    else
    {
        handleMovementFreeCam_(deltaTime);
        updateFreeCamCamera_();
    }
}

// =========================================================
// draw
// Em LookAt: desenha o modelo do soldado no mundo.
// Em FreeCam: o jogador É a câmera — nada a desenhar.
// =========================================================
void Player::draw()
{
    if (!active) return;
    if (cameraMode == CameraMode::FreeCam) return;

    glm::mat4 modelMatrix =
        Matrix_Translate(position.x, position.y, position.z)
        * Matrix_Rotate_Y(yaw)
        * matrixScale;

    DrawModel(modelName, modelMatrix);
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
        // Mouse gira o personagem horizontalmente e a câmera verticalmente
        yaw      -= 0.01f * dx;
        cameraPhi += 0.01f * dy;

        const float phiMax =  PI / 2.0f;
        const float phiMin = -PI / 2.0f;
        cameraPhi = std::max(phiMin, std::min(phiMax, cameraPhi));
    }
    else // FreeCam
    {
        CamYaw   += 0.003f * dx;
        CamPitch -= 0.003f * dy;

        const float limit = PI / 2.0f - 0.01f;
        CamPitch = std::max(-limit, std::min(limit, CamPitch));
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
        CamYaw      = yaw;
        CamPitch    = 0.0f;

        cameraMode = CameraMode::FreeCam;

        if (window)
        {
            glfwGetCursorPos(window, &lastCursorX_, &lastCursorY_);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    else // FreeCam → LookAt
    {
        // Personagem aparece onde a câmera estava, olhando para onde ela olhava
        position = CamPosition;
        yaw      = CamYaw;

        // Reseta órbita para ângulo limpo — evita câmera embaixo do chão
        cameraPhi      = INITIAL_CAMERA_PHI;
        cameraDistance = INITIAL_CAMERA_DISTANCE;
        lookAtTarget_  = position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);

        // Log de posição no terminal
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

    // A câmera fica atrás do personagem: ângulo oposto ao yaw
    float behindAngle = yaw + PI;
    float z = r * cos(cameraPhi) * cos(behindAngle);
    float x = r * cos(cameraPhi) * sin(behindAngle);

    lookAtTarget_    = position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);
    cameraPosition   = lookAtTarget_ + glm::vec4(x, y, z, 0.0f);
    cameraViewVector = lookAtTarget_ - cameraPosition;
}

// =========================================================
// updateFreeCamCamera_  (privado)
// =========================================================
void Player::updateFreeCamCamera_()
{
    glm::vec4 front;
    front.x = cos(CamPitch) * cos(CamYaw);
    front.y = sin(CamPitch);
    front.z = cos(CamPitch) * sin(CamYaw);
    front.w = 0.0f;

    cameraPosition   = CamPosition;
    cameraViewVector = front;
}

// =========================================================
// handleMovementLookAt_  (privado)
// =========================================================
void Player::handleMovementLookAt_(float deltaTime)
{
    if (!window) return;

    // Vetores de direção no plano XZ baseados no yaw atual
    glm::vec3 front(cos(yaw), 0.0f, sin(yaw));
    glm::vec3 right(front.z, 0.0f, -front.x);  // perpendicular no plano XZ

    // Velocidade escalonada por deltaTime para movimento frame-rate independent
    float speed = movementSpeed * deltaTime * 60.0f; // 60 = fator de normalização

    glm::vec3 desiredPos(position.x, position.y, position.z);

    // Nota: W/S movem lateralmente (strafe), A/D movem frente/trás
    // para corresponder ao comportamento original do projeto
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) desiredPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) desiredPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) desiredPos -= front * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) desiredPos += front * speed;

    // Resolve colisão com a malha do mapa
    if (collisionMesh)
    {
        glm::vec3 resolved = ResolvePlayerCollision(
            *collisionMesh,
            desiredPos,
            PLAYER_RADIUS,
            PLAYER_HEIGHT
        );
        position = glm::vec4(resolved.x, resolved.y, resolved.z, 1.0f);
    }
    else
    {
        position = glm::vec4(desiredPos.x, desiredPos.y, desiredPos.z, 1.0f);
    }
}

// =========================================================
// handleMovementFreeCam_  (privado)
// =========================================================
void Player::handleMovementFreeCam_(float deltaTime)
{
    if (!window) return;

    glm::vec4 front;
    front.x = cos(CamPitch) * cos(CamYaw);
    front.y = sin(CamPitch);
    front.z = cos(CamPitch) * sin(CamYaw);
    front.w = 0.0f;

    glm::vec4 right = crossproduct(front, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    right = normalize(right);

    float speed = CAMERA_SPEED * deltaTime * 60.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) CamPosition += front  * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) CamPosition -= front  * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) CamPosition -= right  * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) CamPosition += right  * speed;
}
