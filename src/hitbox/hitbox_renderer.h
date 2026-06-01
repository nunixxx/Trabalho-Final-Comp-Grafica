#pragma once

// =========================================================
// HitboxRenderer
//
// Sistema de debug para visualizar bounding boxes (AABB) dos
// objetos da cena como wireframe colorido.
//
// Uso:
//   1. Chame HitboxRenderer::Init() uma vez após gladLoadGL().
//   2. No loop de renderização, chame DrawHitbox() para cada
//      objeto que quiser visualizar.
//   3. Toggle via tecla F1 (gerenciado externamente em main.cpp).
//
// A hitbox é um AABB em espaço LOCAL do modelo. A model matrix
// (que inclui posição, rotação e escala do objeto) é passada
// como parâmetro e transforma o cubo para o espaço de mundo.
// Isso garante que a hitbox acompanhe corretamente qualquer
// transformação aplicada ao objeto, incluindo rotação no eixo Y.
// =========================================================

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <string>

// Cores pré-definidas para categorias de objetos
namespace HitboxColor
{
    constexpr glm::vec3 ENEMY       = {1.0f, 0.2f, 0.2f};  // vermelho
    constexpr glm::vec3 PLAYER      = {0.2f, 0.8f, 0.2f};  // verde
    constexpr glm::vec3 ITEM        = {0.2f, 0.6f, 1.0f};  // azul claro
    constexpr glm::vec3 WEAPON      = {1.0f, 0.8f, 0.1f};  // amarelo
    constexpr glm::vec3 DEFAULT     = {1.0f, 1.0f, 1.0f};  // branco
}

class HitboxRenderer
{
public:
    // ----------------------------------------------------------
    // Inicialização — deve ser chamado após gladLoadGL()
    // Compila o shader dedicado e cria o VAO/VBO do cubo unitário.
    // ----------------------------------------------------------
    static void Init();

    // ----------------------------------------------------------
    // Desenha a hitbox de um objeto.
    //
    // Parâmetros:
    //   modelMatrix — a mesma matrix usada no DrawModel() do objeto.
    //                 Deve já incluir translate + rotate + scale.
    //   bboxMin     — canto mínimo da AABB em espaço local do modelo
    //   bboxMax     — canto máximo da AABB em espaço local do modelo
    //   view        — matrix de view atual
    //   projection  — matrix de projeção atual
    //   color       — cor do wireframe (use HitboxColor::*)
    // ----------------------------------------------------------
    static void DrawHitbox(
        const glm::mat4& modelMatrix,
        const glm::vec3& bboxMin,
        const glm::vec3& bboxMax,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& color = HitboxColor::DEFAULT
    );

    // ----------------------------------------------------------
    // Toggle de visibilidade (chamado pelo KeyCallback no F1)
    // ----------------------------------------------------------
    static bool IsVisible() { return s_Visible; }
    static void Toggle()    { s_Visible = !s_Visible; }
    static void SetVisible(bool v) { s_Visible = v; }

private:
    static GLuint s_ProgramID;
    static GLuint s_VAO;
    static GLuint s_VBO;
    static GLuint s_EBO;

    static GLint  s_UniformModel;
    static GLint  s_UniformView;
    static GLint  s_UniformProjection;
    static GLint  s_UniformColor;

    static bool   s_Visible;
    static bool   s_Initialized;

    // Compila e linka o shader de hitbox
    static GLuint CompileShader_(const char* vertSrc, const char* fragSrc);
};
