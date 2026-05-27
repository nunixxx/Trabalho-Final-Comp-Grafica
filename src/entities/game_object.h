#pragma once

#include <string>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// Forward declaration — evita incluir o header inteiro aqui
void DrawModel(const std::string& model_name, glm::mat4 model_matrix);

// Funções de matriz definidas em matrices.h
glm::mat4 Matrix_Translate(float tx, float ty, float tz);
glm::mat4 Matrix_Rotate_Y(float angle);
glm::mat4 Matrix_Scale(float sx, float sy, float sz);

// =========================================================
// Classe base abstrata para todos os objetos da cena.
//
// Centraliza os dados que todo objeto compartilha:
//   - posição no mundo (position)
//   - orientação horizontal (yaw)
//   - escala (matrixScale)
//   - nome do modelo 3D registrado em g_ModelRegistry
//
// As subclasses concretas implementam:
//   - update()      — lógica por frame (movimento, animação)
//   - draw()        — monta a model matrix e chama DrawModel()
//   - onCollision() — resposta à colisão com outro objeto
// =========================================================
class GameObject
{
public:
    // ----------------------------------------------------------
    // Dados de transform
    // ----------------------------------------------------------
    glm::vec4   position;       // posição no espaço de mundo (w=1)
    float       yaw;            // rotação em torno do eixo Y (radianos)
    glm::mat4   matrixScale;    // escala do modelo (ex: Matrix_Scale(0.1f,...))

    // ----------------------------------------------------------
    // Referência ao modelo 3D (chave em g_ModelRegistry)
    // ----------------------------------------------------------
    std::string modelName;

    // ----------------------------------------------------------
    // Estado geral
    // ----------------------------------------------------------
    bool        active;         // false = não atualiza nem desenha

    // ----------------------------------------------------------
    // Construtor
    // ----------------------------------------------------------
    GameObject(
        const std::string& modelName,
        glm::vec4          position,
        float              yaw         = 0.0f,
        glm::mat4          matrixScale = glm::mat4(1.0f)
    )
        : modelName(modelName)
        , position(position)
        , yaw(yaw)
        , matrixScale(matrixScale)
        , active(true)
    {}

    // Destrutor virtual — necessário para delete correto via ponteiro base
    virtual ~GameObject() = default;

    // ----------------------------------------------------------
    // Interface virtual pura
    // ----------------------------------------------------------

    // Lógica por frame. deltaTime em segundos (já calculado no main).
    virtual void update(float deltaTime) = 0;

    // Monta a model matrix e chama DrawModel(). Pode ser sobrescrito
    // para adicionar animações ou múltiplas partes (ex: arma do inimigo).
    virtual void draw();

    // Chamado quando este objeto colide com outro.
    // A implementação padrão não faz nada; subclasses sobrescrevem.
    virtual void onCollision(GameObject& other) {}

    // ----------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------

    // Retorna a model matrix padrão: Translate * RotateY * Scale.
    // Subclasses podem usar isso dentro do próprio draw() ou ignorar.
    glm::mat4 buildModelMatrix() const;

protected:
    // Construtor protegido sem modelName — para subclasses que definem
    // o modelo internamente (ex: Player tem modelo fixo "soldier").
    GameObject() : yaw(0.0f), matrixScale(glm::mat4(1.0f)), active(true) {}
};
