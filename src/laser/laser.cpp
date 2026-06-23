#include "laser.h"
#include <cstdio>
#include <glm/gtc/type_ptr.hpp>

// =========================================================
// Definição das statics
// =========================================================
LaserProjectile LaserRenderer::s_Laser = {};

GLuint LaserRenderer::s_ProgramID       = 0;
GLuint LaserRenderer::s_VAO             = 0;
GLuint LaserRenderer::s_VBO_Beam        = 0;
GLuint LaserRenderer::s_EBO_Beam        = 0;
GLuint LaserRenderer::s_VBO_Glow        = 0;
GLuint LaserRenderer::s_EBO_Glow        = 0;
GLint  LaserRenderer::s_UniformView     = -1;
GLint  LaserRenderer::s_UniformProjection = -1;
GLint  LaserRenderer::s_UniformColor    = -1;
GLint  LaserRenderer::s_UniformModel    = -1;
bool   LaserRenderer::s_Initialized     = false;

// =========================================================
// Constantes do laser
// =========================================================
static const float LASER_SPEED      = 120.0f;   // unidades por segundo
static const float LASER_HOLD_TIME  = 0.15f;     // segundos visível após alcançar o alvo
static const float LASER_BEAM_WIDTH = 0.04f;     // largura do feixe
static const float LASER_GLOW_SIZE  = 0.12f;     // tamanho do glow na ponta

// =========================================================
// Shaders embutidos
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
uniform vec4 laser_color;
out vec4 fragColor;
void main()
{
    fragColor = laser_color;
}
)GLSL";

// =========================================================
// CompileShader_
// =========================================================
GLuint LaserRenderer::CompileShader_(const char* vertSrc, const char* fragSrc)
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
            fprintf(stderr, "[LaserRenderer] Shader compile error:\n%s\n", log);
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
        fprintf(stderr, "[LaserRenderer] Program link error:\n%s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// =========================================================
// Init
// =========================================================
void LaserRenderer::Init()
{
    if (s_Initialized) return;

    s_ProgramID = CompileShader_(s_VertSrc, s_FragSrc);

    s_UniformModel      = glGetUniformLocation(s_ProgramID, "model");
    s_UniformView       = glGetUniformLocation(s_ProgramID, "view");
    s_UniformProjection = glGetUniformLocation(s_ProgramID, "projection");
    s_UniformColor      = glGetUniformLocation(s_ProgramID, "laser_color");

    // --- Beam geometry: a thin quad strip (4 vertices, 6 indices) ---
    // Vertices are placeholder; they will be updated each frame via glBufferSubData.
    float beamVerts[12] = {};
    GLuint beamIndices[6] = { 0, 1, 2, 1, 3, 2 };

    glGenVertexArrays(1, &s_VAO);

    glGenBuffers(1, &s_VBO_Beam);
    glGenBuffers(1, &s_EBO_Beam);

    glBindVertexArray(s_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, s_VBO_Beam);
    glBufferData(GL_ARRAY_BUFFER, sizeof(beamVerts), beamVerts, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO_Beam);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(beamIndices), beamIndices, GL_STATIC_DRAW);

    // --- Glow geometry: a small quad (4 vertices, 6 indices) ---
    float glowVerts[12] = {};
    GLuint glowIndices[6] = { 0, 1, 2, 1, 3, 2 };

    glGenBuffers(1, &s_VBO_Glow);
    glGenBuffers(1, &s_EBO_Glow);

    glBindBuffer(GL_ARRAY_BUFFER, s_VBO_Glow);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glowVerts), glowVerts, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO_Glow);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glowIndices), glowIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    s_Initialized = true;
    printf("[LaserRenderer] Inicializado com sucesso.\n");
}

// =========================================================
// Spawn
// =========================================================
void LaserRenderer::Spawn(
    const glm::vec3& start,
    const glm::vec3& dir,
    float            maxLen,
    bool             hitEnemy,
    int              enemyIdx)
{
    s_Laser.startPos     = start;
    s_Laser.direction    = dir;
    s_Laser.maxLength    = maxLen;
    s_Laser.currentLength = 0.0f;
    s_Laser.speed        = LASER_SPEED;
    s_Laser.holdTimer    = 0.0f;
    s_Laser.active       = true;
    s_Laser.hitEnemy     = hitEnemy;
    s_Laser.enemyIdx     = enemyIdx;
}

// =========================================================
// Update
// =========================================================
void LaserRenderer::Update(float deltaTime)
{
    if (!s_Laser.active) return;

    if (s_Laser.currentLength < s_Laser.maxLength)
    {
        s_Laser.currentLength += s_Laser.speed * deltaTime;
        if (s_Laser.currentLength >= s_Laser.maxLength)
        {
            s_Laser.currentLength = s_Laser.maxLength;
            s_Laser.holdTimer = LASER_HOLD_TIME;
        }
    }
    else
    {
        s_Laser.holdTimer -= deltaTime;
        if (s_Laser.holdTimer <= 0.0f)
            s_Laser.active = false;
    }
}

// =========================================================
// Render
// =========================================================
void LaserRenderer::Render(
    const glm::mat4& view,
    const glm::mat4& projection)
{
    if (!s_Laser.active || !s_Initialized) return;

    glm::vec3 startPos = s_Laser.startPos;
    glm::vec3 dir      = s_Laser.direction;
    float     len      = s_Laser.currentLength;
    glm::vec3 endPos   = startPos + dir * len;

    // --- Camera right vector (from view matrix) ---
    // View matrix columns: [right_x, up_x, -front_x, ...]
    //                       [right_y, up_y, -front_y, ...]
    //                       [right_z, up_z, -front_z, ...]
    glm::vec3 camRight(
        view[0][0],
        view[1][0],
        view[2][0]
    );

    glm::vec3 camUp(
        view[0][1],
        view[1][1],
        view[2][1]
    );

    // Salva estado OpenGL
    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    GLboolean prevDepthWrite;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthWrite);

    glUseProgram(s_ProgramID);
    glUniformMatrix4fv(s_UniformView,       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(s_UniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

    // ── Desenha o feixe do laser ──
    float halfW = LASER_BEAM_WIDTH * 0.5f;

    // Cria os 4 vértices do quad do feixe
    // (um ribbon que encara a câmera)
    glm::vec3 perp1 = glm::normalize(glm::cross(dir, camRight));
    if (glm::length(perp1) < 0.001f)
        perp1 = camUp;
    perp1 = glm::normalize(glm::cross(dir, perp1));

    glm::vec3 bVerts[4] = {
        startPos + perp1 * (-halfW),
        startPos + perp1 * halfW,
        endPos   + perp1 * (-halfW),
        endPos   + perp1 * halfW
    };

    glBindBuffer(GL_ARRAY_BUFFER, s_VBO_Beam);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bVerts), bVerts);

    // Cor laranja-avermelhada brilhante
    glm::vec4 beamColor(1.0f, 0.35f, 0.05f, 0.85f);
    glUniform4fv(s_UniformColor, 1, glm::value_ptr(beamColor));

    // Para o beam, usamos model = identidade (coords já são mundiais)
    glm::mat4 identity(1.0f);
    glUniformMatrix4fv(s_UniformModel, 1, GL_FALSE, glm::value_ptr(identity));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glBindVertexArray(s_VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO_Beam);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // ── Desenha o glow na ponta do laser ──
    float glowHalf = LASER_GLOW_SIZE * 0.5f;
    glm::vec3 gVerts[4] = {
        endPos + camRight * (-glowHalf) + camUp * (-glowHalf),
        endPos + camRight * glowHalf    + camUp * (-glowHalf),
        endPos + camRight * (-glowHalf) + camUp * glowHalf,
        endPos + camRight * glowHalf    + camUp * glowHalf
    };

    glBindBuffer(GL_ARRAY_BUFFER, s_VBO_Glow);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gVerts), gVerts);

    // Glow branco-amarelado
    glm::vec4 glowColor(1.0f, 0.9f, 0.4f, 0.9f);
    glUniform4fv(s_UniformColor, 1, glm::value_ptr(glowColor));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO_Glow);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // ── Se colidiu, desenha um flash de impacto ──
    if (len >= s_Laser.maxLength - 0.01f && s_Laser.holdTimer > 0.0f)
    {
        float flashScale = 1.0f - (s_Laser.holdTimer / LASER_HOLD_TIME);
        float flashSize  = LASER_GLOW_SIZE * (0.5f + flashScale * 1.5f);
        float alpha      = 1.0f - flashScale;

        float fHalf = flashSize * 0.5f;
        glm::vec3 fVerts[4] = {
            endPos + camRight * (-fHalf) + camUp * (-fHalf),
            endPos + camRight * fHalf    + camUp * (-fHalf),
            endPos + camRight * (-fHalf) + camUp * fHalf,
            endPos + camRight * fHalf    + camUp * fHalf
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fVerts), fVerts);

        glm::vec4 flashColor(1.0f, 1.0f, 1.0f, alpha);
        glUniform4fv(s_UniformColor, 1, glm::value_ptr(flashColor));

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glDepthMask(prevDepthWrite);
    glDisable(GL_BLEND);
    glUseProgram(prevProgram);
}
