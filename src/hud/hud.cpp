#include "hud.h"
#include "../classes/player/player.h"
#include "../constants.h"

#include <cstdio>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

// TextRendering functions (declared in main.cpp, forward-declared here)
void TextRendering_PrintString(GLFWwindow* window, const std::string &str,
                               float x, float y, float scale = 1.0f);
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_SetColor(float r, float g, float b, float a);

// ── Bar shader (2D colored quads in NDC) ─────────────────────────────
static const char* s_BarVertSrc = R"GLSL(
#version 330 core
layout (location = 0) in vec2 position;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}
)GLSL";

static const char* s_BarFragSrc = R"GLSL(
#version 330 core
uniform vec4 bar_color;
out vec4 fragColor;
void main()
{
    fragColor = bar_color;
}
)GLSL";

static GLuint CompileShaderGL(GLenum type, const char* src)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetShaderInfoLog(id, 512, nullptr, log);
        fprintf(stderr, "[HUD] Shader compile error:\n%s\n", log);
    }
    return id;
}

static GLuint LinkProgram(GLuint vert, GLuint frag)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        fprintf(stderr, "[HUD] Program link error:\n%s\n", log);
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ── Init ─────────────────────────────────────────────────────────────
void HUD::init()
{
    if (initialized_) return;

    GLuint vert = CompileShaderGL(GL_VERTEX_SHADER, s_BarVertSrc);
    GLuint frag = CompileShaderGL(GL_FRAGMENT_SHADER, s_BarFragSrc);
    barProgramID_ = LinkProgram(vert, frag);
    barUniformColor_ = glGetUniformLocation(barProgramID_, "bar_color");

    glGenVertexArrays(1, &barVAO_);
    glGenBuffers(1, &barVBO_);

    glBindVertexArray(barVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, barVBO_);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    initialized_ = true;
}

// ── Update ───────────────────────────────────────────────────────────
void HUD::update(float dt, const Player& player)
{
    if (player.health < lastHealth_)
        damageFlashTimer_ = 0.3f;
    lastHealth_ = player.health;

    if (damageFlashTimer_ > 0.0f)
        damageFlashTimer_ -= dt;
}

// ── Render ───────────────────────────────────────────────────────────
void HUD::render(GLFWwindow* window, const Player& player)
{
    float lh = TextRendering_LineHeight(window);
    float cw = TextRendering_CharWidth(window);

    float hudY  = -1.0f + lh * 1.5f;
    float hudY2 = hudY  + lh * 1.2f;

    // Bars at same horizontal positions as old HUD
    float hpX = -1.0f + cw * 1.5f;
    float armX = cw * 7.0f;
    float ammoX = 1.0f - cw * 17.0f;

    drawHealth(window, player);
    drawArmor(window, player);
    drawAmmo(window, player);
    drawCrosshair(window, player);
    drawDamageFlash(window);
    drawAlertLowHealth(window, (float)player.health / PLAYER_MAX_HEALTH);
}

// ── Draw a single bar (background + fill) ────────────────────────────
void HUD::drawBar(GLFWwindow* window, float x, float y, float w, float h,
                  float fill, const glm::vec3& color,
                  const std::string& label, const std::string& valueStr)
{
    float cw = TextRendering_CharWidth(window);
    float lh = TextRendering_LineHeight(window);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);

    // ── Bar background ──
    glm::vec4 bgColor(0.15f, 0.15f, 0.15f, 0.85f);
    glUseProgram(barProgramID_);
    glUniform4fv(barUniformColor_, 1, &bgColor[0]);

    float verts[12] = {
        x,     y,
        x + w, y,
        x + w, y + h,
        x,     y,
        x + w, y + h,
        x,     y + h,
    };
    glBindBuffer(GL_ARRAY_BUFFER, barVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(barVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // ── Bar fill ──
    float fillW = w * std::min(1.0f, std::max(0.0f, fill));
    if (fillW > 0.0f)
    {
        glm::vec4 fillColor(color, 0.9f);
        glUseProgram(barProgramID_);
        glUniform4fv(barUniformColor_, 1, &fillColor[0]);

        float fverts[12] = {
            x,         y,
            x + fillW, y,
            x + fillW, y + h,
            x,         y,
            x + fillW, y + h,
            x,         y + h,
        };
        glBindBuffer(GL_ARRAY_BUFFER, barVBO_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fverts), fverts);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(barVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    // ── Label above ──
    TextRendering_SetColor(1, 1, 1, 1);
    TextRendering_PrintString(window, label, x, y + h + lh * 0.3f, 1.0f);

    // ── Value text at end of bar ──
    float valX = x + w + cw * 0.5f;
    TextRendering_PrintString(window, valueStr, valX, y, 1.0f);
}

// ── Health bar ───────────────────────────────────────────────────────
void HUD::drawHealth(GLFWwindow* window, const Player& player)
{
    float lh = TextRendering_LineHeight(window);
    float cw = TextRendering_CharWidth(window);

    float hpFrac = (float)player.health / PLAYER_MAX_HEALTH;
    float x = -1.0f + cw * 1.5f;
    float y = -1.0f + lh * 1.5f;
    float w = 12.0f * cw;
    float h = lh * 0.5f;

    glm::vec3 color;
    if (hpFrac > 0.60f)
        color = glm::vec3(0.2f, 0.9f, 0.2f);
    else if (hpFrac > 0.30f)
        color = glm::vec3(0.9f, 0.8f, 0.1f);
    else
        color = glm::vec3(0.9f, 0.2f, 0.2f);

    char val[16];
    snprintf(val, 16, "%d", player.health);
    drawBar(window, x, y, w, h, hpFrac, color, "HP", val);
}

// ── Armor bar ────────────────────────────────────────────────────────
void HUD::drawArmor(GLFWwindow* window, const Player& player)
{
    float lh = TextRendering_LineHeight(window);
    float cw = TextRendering_CharWidth(window);

    const int ARM_MAX = 100;
    float armFrac = (float)player.armor / ARM_MAX;
    float x = cw * 7.0f;
    float y = -1.0f + lh * 1.5f;
    float w = 12.0f * cw;
    float h = lh * 0.5f;

    glm::vec3 color(0.2f, 0.6f, 1.0f);

    char val[16];
    snprintf(val, 16, "%d", player.armor);
    drawBar(window, x, y, w, h, armFrac, color, "ARM", val);
}

// ── Ammo bar ─────────────────────────────────────────────────────────
void HUD::drawAmmo(GLFWwindow* window, const Player& player)
{
    float lh = TextRendering_LineHeight(window);
    float cw = TextRendering_CharWidth(window);

    const int AMMO_MAX = 60;
    float ammoFrac = std::min(1.0f, (float)player.ammo / AMMO_MAX);
    float x = 1.0f - cw * 16.0f;
    float y = -1.0f + lh * 1.5f;
    float w = 12.0f * cw;
    float h = lh * 0.5f;

    glm::vec3 color;
    if (player.ammo > 10)
        color = glm::vec3(1.0f, 0.8f, 0.2f);
    else if (player.ammo > 0)
        color = glm::vec3(1.0f, 0.4f, 0.1f);
    else
        color = glm::vec3(0.9f, 0.1f, 0.1f);

    char val[16];
    snprintf(val, 16, "%d", player.ammo);
    drawBar(window, x, y, w, h, ammoFrac, color, "AMMO", val);

    // ── "SEM MUNICAO" warning ──
    if (player.ammo == 0)
    {
        float sinTime = sin(glfwGetTime() * 6.0f) * 0.5f + 0.5f;
        TextRendering_SetColor(1, sinTime * 0.5f + 0.5f, sinTime * 0.5f + 0.5f, 1);
        float warnX = x + w * 0.5f - cw * 5.0f;
        TextRendering_PrintString(window, "SEM MUNICAO!", warnX, y + lh * 1.5f, 1.0f);
    }
}

// ── Crosshair ────────────────────────────────────────────────────────
void HUD::drawCrosshair(GLFWwindow* window, const Player& player)
{
    if (!player.isFirstPerson()) return;

    float cw = TextRendering_CharWidth(window);
    float lh = TextRendering_LineHeight(window);

    TextRendering_SetColor(0, 1, 0, 1);
    TextRendering_PrintString(window, "+", -cw * 0.5f, lh * 0.5f, 2.0f);
}

// ── Damage flash ─────────────────────────────────────────────────────
void HUD::drawDamageFlash(GLFWwindow* window)
{
    if (damageFlashTimer_ <= 0.0f) return;

    float alpha = damageFlashTimer_ * 1.5f;
    if (alpha > 1.0f) alpha = 1.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);

    glm::vec4 flashColor(0.8f, 0.1f, 0.1f, alpha * 0.6f);

    glUseProgram(barProgramID_);
    glUniform4fv(barUniformColor_, 1, &flashColor[0]);

    float verts[12] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, barVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(barVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

// ── Low health pulsing alert ─────────────────────────────────────────
void HUD::drawAlertLowHealth(GLFWwindow* window, float hpFrac)
{
    if (hpFrac > 0.30f) return;

    float lh = TextRendering_LineHeight(window);
    float cw = TextRendering_CharWidth(window);
    float pulse = sin(glfwGetTime() * 8.0f) * 0.3f + 0.7f;

    TextRendering_SetColor(1, pulse * 0.3f, pulse * 0.3f, 1);
    TextRendering_PrintString(window, "",
        -cw * 5.0f, 1.0f - lh * 3.0f, 1.5f);
}
