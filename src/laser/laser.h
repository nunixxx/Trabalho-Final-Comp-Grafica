#pragma once

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct LaserProjectile {
    glm::vec3 startPos;
    glm::vec3 direction;
    float     maxLength;
    float     currentLength;
    float     speed;
    float     holdTimer;
    bool      active;
    bool      hitEnemy;
    int       enemyIdx;
};

class LaserRenderer {
public:
    static void Init();
    static void Update(float deltaTime);
    static void Spawn(
        const glm::vec3& start,
        const glm::vec3& dir,
        float            maxLen,
        bool             hitEnemy,
        int              enemyIdx
    );
    static void Render(
        const glm::mat4& view,
        const glm::mat4& projection
    );

    static bool IsActive() { return s_Laser.active; }

private:
    static LaserProjectile s_Laser;

    static GLuint s_ProgramID;
    static GLuint s_VAO;
    static GLuint s_VBO_Beam;
    static GLuint s_EBO_Beam;
    static GLuint s_VBO_Glow;
    static GLuint s_EBO_Glow;
    static GLint  s_UniformView;
    static GLint  s_UniformProjection;
    static GLint  s_UniformColor;
    static GLint  s_UniformModel;
    static bool   s_Initialized;

    static GLuint CompileShader_(const char* vertSrc, const char* fragSrc);
};
