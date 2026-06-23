#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>

class Player;

class HUD
{
public:
    void init();
    void update(float dt, const Player& player);
    void render(GLFWwindow* window, const Player& player);

private:
    void drawBar(GLFWwindow* window, float x, float y, float w, float h,
                 float fill, const glm::vec3& color, const std::string& label,
                 const std::string& valueStr);
    void drawHealth(GLFWwindow* window, const Player& player);
    void drawArmor(GLFWwindow* window, const Player& player);
    void drawAmmo(GLFWwindow* window, const Player& player);
    void drawFace(GLFWwindow* window, float x, float y, float hpFrac);
    void drawCrosshair(GLFWwindow* window, const Player& player);
    void drawDamageFlash(GLFWwindow* window);
    void drawAlertLowHealth(GLFWwindow* window, float hpFrac);

    float damageFlashTimer_ = 0.0f;
    int   lastHealth_ = 0;

    GLuint barProgramID_ = 0;
    GLuint barVAO_ = 0;
    GLuint barVBO_ = 0;
    GLint  barUniformColor_ = -1;
    bool   initialized_ = false;
};
