#pragma once

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <limits>
#include <vector>
#include <memory>
#include <map>
#include <string>

#include "../enemy/enemy.h"
#include "../game_object.h"    
#include "../../constants.h"                // PLAYER_RADIUS, PLAYER_HEIGHT, SOLDIERS_SCALE, etc.
#include "../../collision/raycast.h"
#include "../../collision/collision.h"     
#include "../../../include/matrices.h"      // Matrix_Translate, Matrix_Rotate_Y, norm, crossproduct, normalize
#include "../../animation/skeletal_animation.h" 
#include "../../animation/animation_system.h"


#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

struct GLFWwindow;  // forward declaration — evita incluir o GLFW aqui

// =========================================================
// Modos de câmera
// =========================================================
enum class CameraMode
{
    LookAt,   // câmera orbita atrás do jogador (gameplay)
    FirstPerson   // modo da camera
};

// =========================================================
// Player
//
// Responsabilidades:
//   - Ler input de teclado e mouse (WASD + arrastar)
//   - Mover o personagem e resolver colisão com o mapa
//   - Gerenciar os dois modos de câmera (LookAt e FreeCam)
//   - Expor os vetores de câmera para o main montar view/projection
//   - Guardar stats (health, armor)
//
// O modelo 3D é sempre "soldier" (fixo em constants.h).
// A escala é sempre SOLDIERS_SCALE.
// =========================================================

class Enemy;  
class Player : public GameObject
{
public:
    // ----------------------------------------------------------
    // Stats
    // ----------------------------------------------------------
    int   health;
    int   armor;
    int   ammo;

    // ----------------------------------------------------------
    // Movimento
    // ----------------------------------------------------------
    float movementSpeed;
    float verticalVelocity;  
    bool  onGround;    
    
    // ----------------------------------------------------------
    // Animação
    // ----------------------------------------------------------
    std::unique_ptr<SkinnedModelAsset> skinnedAsset;
    AnimationState    animState;
    GLuint            skinnedProgramID;
    bool              isMoving = false;

    // ----------------------------------------------------------
    // Tiro
    // ----------------------------------------------------------
    float shootCooldown;   

    // ----------------------------------------------------------
    // Câmera LookAt — parâmetros de órbita
    // ----------------------------------------------------------
    float cameraPhi;       // ângulo vertical da órbita
    float cameraDistance;  // distância câmera-alvo

    // ----------------------------------------------------------
    // Câmera Cam — posição e orientação independentes
    // ----------------------------------------------------------
    glm::vec4 CamPosition;
    float     CamYaw;
    float     CamPitch;

    // ----------------------------------------------------------
    // Modo de câmera atual
    // ----------------------------------------------------------
    CameraMode cameraMode;

    // ----------------------------------------------------------
    // Saídas de câmera — lidas pelo main a cada frame
    // ----------------------------------------------------------
    glm::vec4 cameraPosition;    // posição da câmera no mundo
    glm::vec4 cameraViewVector;  // direção para onde a câmera aponta

    // ----------------------------------------------------------
    // Referência à malha de colisão do mapa
    // ----------------------------------------------------------
    const CollisionMesh* collisionMesh;  // ponteiro — não owna

    // ----------------------------------------------------------
    // Referência à janela GLFW para leitura de input
    // ----------------------------------------------------------
    GLFWwindow* window;  // ponteiro — não owna

    // ----------------------------------------------------------
    // Construtor
    // ----------------------------------------------------------
    Player(
        GLFWwindow*          window,
        const CollisionMesh* collisionMesh,
        glm::vec4            initialPosition,
        float                initialYaw
    );

    // ----------------------------------------------------------
    // Interface GameObject
    // ----------------------------------------------------------
    void update(float deltaTime) override;

    // draw() sobrescrito: em LookAt desenha o modelo do soldado;
    // em FreeCam o jogador é a câmera, não há modelo a desenhar.
    void draw() override;

    void onCollision(GameObject& other) override;

    // ----------------------------------------------------------
    // Callbacks de mouse — chamados pelo main nos callbacks GLFW
    // ----------------------------------------------------------

    // Arrastar o mouse com botão esquerdo pressionado
    void onMouseDrag(float dx, float dy);

    // Scroll do mouse — ajusta distância da câmera LookAt
    void onScroll(float yoffset);

    // ----------------------------------------------------------
    // Ações do jogador
    // ----------------------------------------------------------

    // Dispara um tiro testando colisão com inimigos.
    void shoot(std::vector<std::unique_ptr<Enemy>>& enemies,
           const std::map<std::string, ModelAsset>& modelRegistry);

    // ----------------------------------------------------------
    // Troca de modo de câmera
    // ----------------------------------------------------------
    // Deve ser chamado pelo KeyCallback quando a tecla ';' for pressionada.
    // Internamente salva/restaura o estado da FreeCam e
    // imprime a posição atual no terminal (comportamento original mantido).
    void toggleCameraMode();

    // ----------------------------------------------------------
    // Helpers de estado
    // ----------------------------------------------------------
    bool isAlive()    const { return health > 0; }
    bool isFirstPerson()  const { return cameraMode == CameraMode::FirstPerson; }
    bool isLookAt()   const { return cameraMode == CameraMode::LookAt; }

private:
    // Ponto que a câmera LookAt orbita (ligeiramente acima do personagem)
    glm::vec4 lookAtTarget_;

    // Última posição do cursor — usada para calcular dx/dy no drag
    double lastCursorX_;
    double lastCursorY_;

    // ----------------------------------------------------------
    // Métodos internos
    // ----------------------------------------------------------

    // Atualiza cameraPosition e cameraViewVector no modo LookAt
    void updateLookAtCamera_();

    // Atualiza cameraPosition e cameraViewVector no modo FirstPerson
    void updateFirstPersonCamera_();

    // Lê WASD e move o personagem (modo LookAt)
    void handleMovementLookAt_(float deltaTime);

    // Lê WASD e move a câmera livre (modo FirstPerson)
    void handleMovementFirstPerson_(float deltaTime);

    // Aplica gravidade ao movimento vertical (stub para uso futuro)
    void applyGravity_(float deltaTime);

    // Tenta iniciar um pulo se estiver no chão (stub para uso futuro)
    bool handleJump_();

};
