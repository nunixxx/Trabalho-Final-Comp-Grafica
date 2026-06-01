#include "hitbox_renderer.h"

#include <cstdio>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>

// =========================================================
// Definição das statics
// =========================================================
GLuint HitboxRenderer::s_ProgramID   = 0;
GLuint HitboxRenderer::s_VAO         = 0;
GLuint HitboxRenderer::s_VBO         = 0;
GLuint HitboxRenderer::s_EBO         = 0;
GLint  HitboxRenderer::s_UniformModel      = -1;
GLint  HitboxRenderer::s_UniformView       = -1;
GLint  HitboxRenderer::s_UniformProjection = -1;
GLint  HitboxRenderer::s_UniformColor      = -1;
bool   HitboxRenderer::s_Visible     = false;
bool   HitboxRenderer::s_Initialized = false;

// =========================================================
// Shaders embutidos (sem arquivo externo)
//
// Vértices do cubo são transformados pela model matrix que
// contém scale(bbox), translate(center) * modelTransform.
// O fragment emite apenas a cor uniforme — sem iluminação.
// =========================================================
static const char* s_VertSrc = R"GLSL(
#version 330 core
layout (location = 0) in vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)GLSL";

static const char* s_FragSrc = R"GLSL(
#version 330 core
uniform vec3 hitbox_color;
out vec4 fragColor;
void main()
{
    fragColor = vec4(hitbox_color, 1.0);
}
)GLSL";

// =========================================================
// Geometria do cubo unitário [-0.5, +0.5]^3
//
// 8 vértices, 12 arestas = 24 indices (GL_LINES).
// Usamos um cubo unitário centrado na origem; a model matrix
// escala e posiciona para a bbox real do objeto.
// =========================================================
static const float s_CubeVertices[8 * 3] = {
    // x      y      z
    -0.5f, -0.5f, -0.5f,  // 0 esquerda-baixo-trás
     0.5f, -0.5f, -0.5f,  // 1 direita-baixo-trás
     0.5f,  0.5f, -0.5f,  // 2 direita-cima-trás
    -0.5f,  0.5f, -0.5f,  // 3 esquerda-cima-trás
    -0.5f, -0.5f,  0.5f,  // 4 esquerda-baixo-frente
     0.5f, -0.5f,  0.5f,  // 5 direita-baixo-frente
     0.5f,  0.5f,  0.5f,  // 6 direita-cima-frente
    -0.5f,  0.5f,  0.5f,  // 7 esquerda-cima-frente
};

// Pares de índices formando as 12 arestas do cubo
static const GLuint s_CubeEdges[24] = {
    0,1, 1,2, 2,3, 3,0,   // face traseira
    4,5, 5,6, 6,7, 7,4,   // face dianteira
    0,4, 1,5, 2,6, 3,7    // arestas laterais
};

// =========================================================
// CompileShader_
// =========================================================
GLuint HitboxRenderer::CompileShader_(const char* vertSrc, const char* fragSrc)
{
    auto compileStage = [](GLenum type, const char* src) -> GLuint {
        GLuint id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        GLint ok = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(id, 512, nullptr, log);
            fprintf(stderr, "[HitboxRenderer] Shader compile error:\n%s\n", log);
        }
        return id;
    };

    GLuint vert = compileStage(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        fprintf(stderr, "[HitboxRenderer] Program link error:\n%s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// =========================================================
// Init
// =========================================================
void HitboxRenderer::Init()
{
    if (s_Initialized) return;

    // Compila shaders
    s_ProgramID = CompileShader_(s_VertSrc, s_FragSrc);

    // Obtém locais dos uniforms
    s_UniformModel      = glGetUniformLocation(s_ProgramID, "model");
    s_UniformView       = glGetUniformLocation(s_ProgramID, "view");
    s_UniformProjection = glGetUniformLocation(s_ProgramID, "projection");
    s_UniformColor      = glGetUniformLocation(s_ProgramID, "hitbox_color");

    // Cria VAO/VBO/EBO do cubo unitário
    glGenVertexArrays(1, &s_VAO);
    glGenBuffers(1, &s_VBO);
    glGenBuffers(1, &s_EBO);

    glBindVertexArray(s_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_CubeVertices), s_CubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_CubeEdges), s_CubeEdges, GL_STATIC_DRAW);

    glBindVertexArray(0);

    s_Initialized = true;
    printf("[HitboxRenderer] Inicializado com sucesso.\n");
}

// =========================================================
// DrawHitbox
//
// Estratégia de construção da model matrix da hitbox:
//
//   1. Calcula o centro e as dimensões da AABB em espaço local.
//   2. Monta uma matrix local: Translate(center) * Scale(size).
//      Isso move o cubo unitário para cobrir exatamente a AABB.
//   3. Pré-multiplica pela model matrix do objeto para levar
//      a hitbox ao espaço de mundo com a mesma transformação
//      (posição, rotação, escala) que o objeto usa.
//
// Nota: para objetos com rotação não-uniforme, a AABB em espaço
// de mundo se expande. Esse comportamento é o padrão para OBB
// (Oriented Bounding Box) — a hitbox gira com o objeto.
// =========================================================
void HitboxRenderer::DrawHitbox(
    const glm::mat4& modelMatrix,
    const glm::vec3& bboxMin,
    const glm::vec3& bboxMax,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& color
)
{
    if (!s_Visible || !s_Initialized) return;

    // Centro e dimensões da AABB local
    glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 size   = bboxMax - bboxMin;

    // Garante dimensão mínima para evitar hitbox degenerada
    const float minDim = 0.001f;
    size.x = std::max(size.x, minDim);
    size.y = std::max(size.y, minDim);
    size.z = std::max(size.z, minDim);

    // Matrix local: posiciona e escala o cubo unitário para a AABB
    glm::mat4 localMatrix = glm::mat4(1.0f);

    // Translate pelo centro local
    localMatrix[3][0] = center.x;
    localMatrix[3][1] = center.y;
    localMatrix[3][2] = center.z;

    // Scale pelas dimensões
    glm::mat4 scaleMatrix = glm::mat4(1.0f);
    scaleMatrix[0][0] = size.x;
    scaleMatrix[1][1] = size.y;
    scaleMatrix[2][2] = size.z;

    localMatrix = localMatrix * scaleMatrix;

    // Combina com a model matrix do objeto
    glm::mat4 hitboxModel = modelMatrix * localMatrix;

    // Salva estado OpenGL
    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    // Renderiza wireframe
    glUseProgram(s_ProgramID);

    glUniformMatrix4fv(s_UniformModel,      1, GL_FALSE, glm::value_ptr(hitboxModel));
    glUniformMatrix4fv(s_UniformView,       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(s_UniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(s_UniformColor, 1, glm::value_ptr(color));

    // Desativa depth write para que as linhas não bloqueiem objetos atrás
    GLboolean prevDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    glDepthMask(GL_FALSE);

    glBindVertexArray(s_VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restaura estado
    glDepthMask(prevDepthMask);
    glUseProgram(prevProgram);
}
