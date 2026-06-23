#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tiny_obj_loader.h>
#include <stb_image.h>

#include "utils.h"
#include "matrices.h"

#include "modelRendering/model_rendering.h"
#include "collision/collision.h"

#include "constants.h"
#include "textureRendering/texture_rendering.h"
#include "hitbox/hitbox_renderer.h"

#include "classes/enemy/enemy.h"
#include "classes/player/player.h"
#include "classes/objects/health_pack.h"
#include "classes/objects/world_object.h"
#include "classes/objects/gun.h"
#include "classes/objects/armor.h"
#include "laser/laser.h"


void LoadShadersFromFiles();
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);
static void GetModelBBox(const std::string& modelName, glm::vec3& outMin, glm::vec3& outMax);


void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Instancias de Objetos
std::unique_ptr<Player> g_Player;
std::vector<std::unique_ptr<Enemy>> g_Enemies;
std::vector<std::unique_ptr<HealthPack>> g_HealthPacks;
std::vector<std::unique_ptr<Gun>> g_Guns;
std::vector<std::unique_ptr<Armor>> g_Armors;

// =========================================================
// Sistema de texturas
// =========================================================
std::map<std::string, int>    g_MaterialTextureIndex;
std::map<std::string, GLuint> g_LoadedTextures;
std::vector<GLuint>           g_TextureSlots;

GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_texture_index_uniform;
GLint g_has_texture_uniform;

// Point lights
GLint g_num_point_lights_uniform;
GLint g_point_light_positions_uniform;
GLint g_point_light_colors_uniform;
GLint g_point_light_intensities_uniform;

// Phong specular
GLint g_camera_pos_uniform;
GLint g_shininess_uniform;
GLint g_specular_strength_uniform;
GLint g_ambient_intensity_uniform;

float g_ScreenRatio = 1.0f;

bool g_LeftMouseButtonPressed   = false;
bool g_RightMouseButtonPressed  = false;
bool g_MiddleMouseButtonPressed = false;

CollisionMesh g_CollisionMesh;

bool g_ShowInfoText = true;

GLuint g_GpuProgramID = 0;

std::map<std::string, ModelAsset> g_ModelRegistry;
std::map<std::string, ModelPaths> g_PathsRegistry;

int main(int argc, char* argv[])
{
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(WINDOW_SIZE_X, WINDOW_SIZE_Y, "INF01047 - Doom E1M1", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, WINDOW_SIZE_X, WINDOW_SIZE_Y);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    LoadShadersFromFiles();
    LoadModelsFromCSV(PATH_CSV);

    printf("\nTodos os modelos foram carregados com sucesso!\n\n");

    g_CollisionMesh = BuildCollisionMesh(g_ModelRegistry["map"], 0.05f);

    // Objetos da cena
    g_HealthPacks.push_back(std::make_unique<HealthPack>(glm::vec4( -2.56f, 5.50f, -158.36f, 1.0f), 1.0f));
    g_HealthPacks.push_back(std::make_unique<HealthPack>(glm::vec4(-6.25f, 0.1f, -165.15f, 1.0f), 1.0f));

    g_Guns.push_back(std::make_unique<Pistol>(glm::vec4(-6.25f, 3.1f, -165.15f, 1.0f), 2.0f, 15, 50));

    g_Armors.push_back(std::make_unique<Armor>(glm::vec4(11.17f, 7.20f, -161.61f, 1.0f), 1.75f, 25, 100));

    g_Enemies.push_back(std::make_unique<Enemy>(
        glm::vec4(-38.08f, 0.0f, -161.84f, 1.0f),
        PI / 4.0f,
        std::vector<glm::vec4>{
            glm::vec4(-43.0f, 0.0f, -167.7f, 1.0f),
            glm::vec4(-38.0f, 0.0f, -167.7f, 1.0f),
            glm::vec4(-38.0f, 0.0f, -155.48f, 1.0f),
            glm::vec4(-43.0f, 0.0f, -155.48f, 1.0f),

            // pontos extras do “∞”
            glm::vec4(-40.5f, 0.0f, -161.0f, 1.0f),
            glm::vec4(-41.5f, 0.0f, -158.0f, 1.0f),
            glm::vec4(-39.5f, 0.0f, -164.0f, 1.0f),
            glm::vec4(-40.5f, 0.0f, -161.0f, 1.0f)
        },
        EnemyState::Patrol
    ));
    g_Enemies.back()->collisionMesh = &g_CollisionMesh;

    g_Enemies.push_back(std::make_unique<Enemy>(
        glm::vec4(-2.29f, 5.20f, -166.29f, 1.0f),
        0.0f,
        4.0f,
        EnemyState::Idle
    ));
    g_Enemies.back()->collisionMesh = &g_CollisionMesh;

    g_Player = std::make_unique<Player>(
        window,
        &g_CollisionMesh,
        PLAYER_INITIAL_POSITION,
        PLAYER_INITIAL_YAW
    );

    TextRendering_Init();
    HitboxRenderer::Init();
    LaserRenderer::Init();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glUseProgram(g_GpuProgramID);
    BindAllTextures(g_GpuProgramID);
    glUseProgram(0);

    // Pré-calcula bboxes dos modelos usados frequentemente
    glm::vec3 enemyBMin, enemyBMax;
    GetModelBBox("enemy", enemyBMin, enemyBMax);

    glm::vec3 soldierBMin, soldierBMax;
    GetModelBBox("soldier", soldierBMin, soldierBMax);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_GpuProgramID);

        static float lastTime = (float)glfwGetTime();
        float currentTime = (float)glfwGetTime();
        float deltaTime   = currentTime - lastTime;
        lastTime          = currentTime;

        g_Player->update(deltaTime);

        // Verifica morte do player
        if (!g_Player->active)
        {
            // Exibe mensagem por 3 segundos e fecha
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            TextRendering_PrintString(window, "VOCE MORREU!", -0.25f, 0.05f, 3.0f);
            glfwSwapBuffers(window);
            glfwPollEvents();
            glfwWaitEventsTimeout(10.0);
            break;
        }

        glm::mat4 view = Matrix_Camera_View(
            g_Player->cameraPosition,
            g_Player->cameraViewVector,
            CAMERA_UP_VECTOR
        );

        float field_of_view = PI / 3.0f;
        glm::mat4 projection = Matrix_Perspective(
            field_of_view,
            g_ScreenRatio,
            NEARPLANE,
            FARPLANE
        );

        glUniformMatrix4fv(g_view_uniform,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        // ── Player ────────────────────────────────────────────
        glUniform1f(g_ambient_intensity_uniform, 0.2f);
        g_Player->draw();

        // Hitbox do player
        if (g_Player->active && g_Player->isLookAt())
        {
            glm::mat4 playerModel =
                Matrix_Translate(g_Player->position.x,
                                 g_Player->position.y,
                                 g_Player->position.z)
                * Matrix_Rotate_Y(g_Player->yaw)
                * g_Player->matrixScale;

            HitboxRenderer::DrawHitbox(
                playerModel,
                soldierBMin, soldierBMax,
                view, projection,
                HitboxColor::PLAYER
            );
        }

        glUniformMatrix4fv(g_view_uniform,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        // ── Point Lights ────────────────────────────────────────
        // Coleta objetos ativos como fontes de luz pontual
        struct PointLight { glm::vec3 pos; glm::vec3 color; float intensity; };
        PointLight pointLights[16];
        int numPointLights = 0;

        for (auto& obj : g_HealthPacks)
            if (obj->active && !obj->consumed)
                pointLights[numPointLights++] = {
                    glm::vec3(obj->position), glm::vec3(0.0f, 1.0f, 0.0f), 0.5f // verde
                };

        for (auto& obj : g_Armors)
            if (obj->active && !obj->consumed)
                pointLights[numPointLights++] = {
                    glm::vec3(obj->position), glm::vec3(1.0f, 1.0f, 0.0f), 0.5f // amarelo
                };

        for (auto& gun : g_Guns)
            if (gun->active && !gun->consumed)
                pointLights[numPointLights++] = {
                    glm::vec3(gun->position), glm::vec3(1.0f, 0.6f, 0.0f), 0.4f // laranja
                };

        // Lâmpadas fixas — iluminação para baixo
        pointLights[numPointLights++] = {
            glm::vec3(-14.43f, 9.0f, 168.08f), glm::vec3(1.0f, 0.95f, 0.8f), 0.8f
        };
        pointLights[numPointLights++] = {
            glm::vec3(-14.43f, 9.0f, -155.17f), glm::vec3(1.0f, 0.95f, 0.8f), 0.8f
        };

        glUniform1i(g_num_point_lights_uniform, numPointLights);
        for (int i = 0; i < numPointLights; i++)
        {
            glUniform3fv(g_point_light_positions_uniform + i, 1,
                         glm::value_ptr(pointLights[i].pos));
            glUniform3fv(g_point_light_colors_uniform + i, 1,
                         glm::value_ptr(pointLights[i].color));
            glUniform1f(g_point_light_intensities_uniform + i,
                        pointLights[i].intensity);
        }

        // ── Phong Specular ─────────────────────────────────────
        glUniform3f(g_camera_pos_uniform,
            g_Player->cameraPosition.x,
            g_Player->cameraPosition.y,
            g_Player->cameraPosition.z);
        glUniform1f(g_shininess_uniform, 32.0f);
        glUniform1f(g_specular_strength_uniform, 0.5f);

        // ── Mapa ──────────────────────────────────────────────
        glUniform1f(g_ambient_intensity_uniform, 0.05f);
        glm::mat4 mapModelMatrix = Matrix_Scale(0.05f, 0.05f, 0.05f);
        DrawModel("map", mapModelMatrix);
        // O mapa não tem hitbox de bala, apenas colisão de movimento

        glUniform1f(g_ambient_intensity_uniform, 0.2f);

        // ── Inimigos ──────────────────────────────────────────
        for (auto& enemy : g_Enemies)
        {
            enemy->update(deltaTime, g_Player.get());
            enemy->draw();

            if (enemy->active)
            {
                // Atualiza bbox em tempo real (inimigos se movem)
                glm::vec3 bmin, bmax;
                GetModelBBox(enemy->modelName, bmin, bmax);

                HitboxRenderer::DrawHitbox(
                    enemy->buildModelMatrix(),
                    bmin, bmax,
                    view, projection,
                    HitboxColor::ENEMY
                );
            }
        }

        // ── Guns ──────────────────────────────────────────────
        for (auto& gun : g_Guns)
        {
            gun->update(deltaTime, g_Player.get());
            gun->draw();

            if (gun->active && !gun->consumed)
            {
                glm::mat4 gunModel =
                    Matrix_Translate(gun->position.x,
                                     gun->position.y,
                                     gun->position.z)
                    * Matrix_Rotate_Y(gun->currentAngle)
                    * gun->matrixScale;

                glm::vec3 bmin, bmax;
                GetModelBBox(gun->modelName, bmin, bmax);

                HitboxRenderer::DrawHitbox(
                    gunModel, bmin, bmax,
                    view, projection,
                    HitboxColor::WEAPON
                );
            }
        }

        // ── HealthPacks ───────────────────────────────────────
        for (auto& obj : g_HealthPacks)
        {
            obj->update(deltaTime, g_Player.get());
            obj->draw();

            if (obj->active && !obj->consumed)
            {
                glm::mat4 objModel =
                    Matrix_Translate(obj->position.x,
                                     obj->position.y,
                                     obj->position.z)
                    * Matrix_Rotate_Y(obj->currentAngle)
                    * obj->matrixScale;

                glm::vec3 bmin, bmax;
                GetModelBBox(obj->modelName, bmin, bmax);

                HitboxRenderer::DrawHitbox(
                    objModel, bmin, bmax,
                    view, projection,
                    HitboxColor::ITEM
                );
            }
        }

        // ── Armors ─────────────────────────────────────────────
        for (auto& obj : g_Armors)
        {
            obj->update(deltaTime, g_Player.get());
            obj->draw();

            if (obj->active && !obj->consumed)
            {
                glm::mat4 objModel =
                    Matrix_Translate(obj->position.x,
                                     obj->position.y,
                                     obj->position.z)
                    * Matrix_Rotate_Y(obj->currentAngle)
                    * obj->matrixScale;

                glm::vec3 bmin, bmax;
                GetModelBBox(obj->modelName, bmin, bmax);

                HitboxRenderer::DrawHitbox(
                    objModel, bmin, bmax,
                    view, projection,
                    HitboxColor::ITEM
                );
            }
        }

        // ── Laser ────────────────────────────────────────────────
        LaserRenderer::Update(deltaTime);
        LaserRenderer::Render(view, projection);

        // ── HUD estilo Doom ──────────────────────────────────────
        if (g_ShowInfoText)
        {
            float lh = TextRendering_LineHeight(window);
            float cw = TextRendering_CharWidth(window);

            // =====================================================
            // Posição base da HUD: canto inferior esquerdo
            // =====================================================
            float hudY  = -1.0f + lh * 1.5f;   // linha de fundo
            float hudY2 = hudY  + lh * 1.2f;   // linha de cima

            // =====================================================
            // HP — barra colorida com texto
            // A barra usa blocos '|' para simular preenchimento.
            // Cor: verde > 60%, amarelo > 30%, vermelho <= 30%
            // =====================================================
            const int   HP_MAX    = PLAYER_MAX_HEALTH;
            const int   hp        = g_Player->health;
            const float hpFrac    = (float)hp / HP_MAX;
            const int   BAR_LEN   = 10;  // número de blocos da barra
            int         hpFilled  = (int)(hpFrac * BAR_LEN + 0.5f);
            if (hpFilled > BAR_LEN) hpFilled = BAR_LEN;

            // Monta string da barra: [||||      ]
            std::string hpBar = "[";
            for (int i = 0; i < BAR_LEN; i++)
                hpBar += (i < hpFilled) ? '|' : ' ';
            hpBar += "]";

            // Label + número
            char hpStr[32];
            snprintf(hpStr, 32, "HP  %3d%%", (int)(hpFrac * 100));

            float hpX = -1.0f + cw * 1.5f;
            TextRendering_PrintString(window, "HP", hpX, hudY2, 1.0f);
            TextRendering_PrintString(window, hpBar, hpX, hudY, 1.0f);
            char hpNum[8];
            snprintf(hpNum, 8, "%3d", hp);
            TextRendering_PrintString(window, hpNum, hpX + cw * 13.0f, hudY, 1.0f);

            // =====================================================
            // ROSTO central — muda com o HP (estilo Doom face)
            //   > 60% HP : O_O  (normal)
            //   > 30% HP : -_-  (machucado)
            //   <= 30%HP : x_x  (crítico)
            // =====================================================
            const char* faceTop;
            const char* faceMid;
            const char* faceBot;

            if (hpFrac > 0.60f)
            {
                faceTop = " ___  ";
                faceMid = "| O O|";
                faceBot = "|  ~ |";
            }
            else if (hpFrac > 0.30f)
            {
                faceTop = " ___  ";
                faceMid = "| - -|";
                faceBot = "|  _ |";
            }
            else
            {
                faceTop = " ___  ";
                faceMid = "| x x|";
                faceBot = "|  v |";
            }

            // Centraliza o rosto na tela
            float faceX = -cw * 3.0f;
            TextRendering_PrintString(window, faceTop, faceX, hudY2 + lh * 0.5f, 1.0f);
            TextRendering_PrintString(window, faceMid, faceX, hudY2 - lh * 0.1f, 1.0f);
            TextRendering_PrintString(window, faceBot, faceX, hudY,               1.0f);

            // =====================================================
            // ARMADURA — lado direito do rosto
            // =====================================================
            const int   ARM_MAX   = 100;
            const int   arm       = g_Player->armor;
            const float armFrac   = (float)arm / ARM_MAX;
            int         armFilled = (int)(armFrac * BAR_LEN + 0.5f);
            if (armFilled > BAR_LEN) armFilled = BAR_LEN;

            std::string armBar = "[";
            for (int i = 0; i < BAR_LEN; i++)
                armBar += (i < armFilled) ? '|' : ' ';
            armBar += "]";

            float armX = cw * 7.0f;
            TextRendering_PrintString(window, "ARM", armX, hudY2, 1.0f);
            TextRendering_PrintString(window, armBar, armX, hudY, 1.0f);
            char armNum[8];
            snprintf(armNum, 8, "%3d", arm);
            TextRendering_PrintString(window, armNum, armX + cw * 13.0f, hudY, 1.0f);

            // =====================================================
            // MUNIÇÃO — canto inferior direito
            // =====================================================
            const int   AMMO_MAX   = 60;
            const int   ammo       = g_Player->ammo;
            const float ammoFrac   = std::min(1.0f, (float)ammo / AMMO_MAX);
            int         ammoFilled = (int)(ammoFrac * BAR_LEN + 0.5f);
            if (ammoFilled > BAR_LEN) ammoFilled = BAR_LEN;

            std::string ammoBar = "[";
            for (int i = 0; i < BAR_LEN; i++)
                ammoBar += (i < ammoFilled) ? '|' : ' ';
            ammoBar += "]";

            float ammoX = 1.0f - cw * 17.0f;
            TextRendering_PrintString(window, "AMMO", ammoX, hudY2, 1.0f);
            TextRendering_PrintString(window, ammoBar, ammoX, hudY, 1.0f);
            char ammoNum[8];
            snprintf(ammoNum, 8, "%3d", ammo);
            TextRendering_PrintString(window, ammoNum, ammoX + cw * 13.0f, hudY, 1.0f);

            // =====================================================
            // Mira (crosshair) — centro da tela
            // =====================================================
            if(g_Player->isFirstPerson())
                TextRendering_PrintString(window, "+", -cw * 0.5f, lh * 0.5f, 2.0f);
        }

        TextRendering_ShowFramesPerSecond(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void LoadShadersFromFiles()
{
    GLuint vertex_shader_id   = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if (g_GpuProgramID != 0)
        glDeleteProgram(g_GpuProgramID);

    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    g_model_uniform         = glGetUniformLocation(g_GpuProgramID, "model");
    g_view_uniform          = glGetUniformLocation(g_GpuProgramID, "view");
    g_projection_uniform    = glGetUniformLocation(g_GpuProgramID, "projection");
    g_object_id_uniform     = glGetUniformLocation(g_GpuProgramID, "object_id");
    g_bbox_min_uniform      = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform      = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_texture_index_uniform = glGetUniformLocation(g_GpuProgramID, "texture_index");
    g_has_texture_uniform   = glGetUniformLocation(g_GpuProgramID, "has_texture");

    g_num_point_lights_uniform        = glGetUniformLocation(g_GpuProgramID, "u_num_point_lights");
    g_point_light_positions_uniform   = glGetUniformLocation(g_GpuProgramID, "u_point_light_positions");
    g_point_light_colors_uniform      = glGetUniformLocation(g_GpuProgramID, "u_point_light_colors");
    g_point_light_intensities_uniform = glGetUniformLocation(g_GpuProgramID, "u_point_light_intensities");

    g_camera_pos_uniform        = glGetUniformLocation(g_GpuProgramID, "u_camera_pos");
    g_shininess_uniform         = glGetUniformLocation(g_GpuProgramID, "u_shininess");
    g_specular_strength_uniform = glGetUniformLocation(g_GpuProgramID, "u_specular_strength");
    g_ambient_intensity_uniform = glGetUniformLocation(g_GpuProgramID, "u_ambient_intensity");

    glUseProgram(g_GpuProgramID);
    glUseProgram(0);
}

GLuint LoadShader_Vertex(const char* filename)
{
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    LoadShader(filename, vertex_shader_id);
    return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char* filename)
{
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    LoadShader(filename, fragment_shader_id);
    return fragment_shader_id;
}

void LoadShader(const char* filename, GLuint shader_id)
{
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch (std::exception& e) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>(str.length());

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);
    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if (log_length != 0)
    {
        std::string output;
        if (!compiled_ok)
            output += "ERROR: OpenGL compilation of \"";
        else
            output += "WARNING: OpenGL compilation of \"";
        output += filename;
        output += "\".\n== Start of compilation log\n";
        output += log;
        output += "== End of compilation log\n";
        fprintf(stderr, "%s", output.c_str());
    }
    delete [] log;
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        GLchar* log = new GLchar[log_length];
        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;
        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";
        delete [] log;
        fprintf(stderr, "%s", output.c_str());
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    return program_id;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

double g_LastCursorPosX, g_LastCursorPosY;

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;

        if (g_Player && g_Player->isFirstPerson())
            g_Player->shoot(g_Enemies, g_ModelRegistry);
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        g_LeftMouseButtonPressed = false;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        g_RightMouseButtonPressed = false;

    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
        g_MiddleMouseButtonPressed = false;
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!g_Player) return;

    static double lastX = xpos, lastY = ypos;
    float dx = (float)(xpos - lastX);
    float dy = (float)(ypos - lastY);
    lastX = xpos;
    lastY = ypos;

    bool drag = g_LeftMouseButtonPressed || g_Player->isFirstPerson();
    if (drag)
        g_Player->onMouseDrag(dx, dy);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (g_Player)
        g_Player->onScroll((float)yoffset);
}

void Correcao_KeyCallback(int key, int action, int mod);

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    Correcao_KeyCallback(key, action, mod);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_SEMICOLON && action == GLFW_PRESS)
        g_Player->toggleCameraMode();

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
        g_ShowInfoText = !g_ShowInfoText;

    // =========================================================
    // NOVO: toggle de hitboxes com F1
    // =========================================================
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        HitboxRenderer::Toggle();
        fprintf(stdout, "[HitboxRenderer] Hitboxes: %s\n",
                HitboxRenderer::IsVisible() ? "ATIVADAS" : "DESATIVADAS");
        fflush(stdout);
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        glUseProgram(g_GpuProgramID);
        BindAllTextures(g_GpuProgramID);
        glUseProgram(0);
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }
}

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if (!g_ShowInfoText) return;

    static float old_seconds     = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20]      = "?? fps";
    static int   numchars        = 7;

    ellapsed_frames += 1;
    float seconds          = (float)glfwGetTime();
    float ellapsed_seconds = seconds - old_seconds;

    if (ellapsed_seconds > 1.0f)
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
        old_seconds     = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth  = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer,
        1.0f - (numchars + 1)*charwidth, 1.0f - lineheight, 1.0f);

    // Indica na tela quando hitboxes estão ativas
    if (HitboxRenderer::IsVisible())
    {
        TextRendering_PrintString(window, "[F1] Hitboxes ON",
            -1.0f + charwidth, 1.0f - 2.0f * lineheight, 1.0f);
    }
}

// =========================================================
// Helper: retorna a AABB unificada de todas as partes de um
// modelo registrado. Usado para montar a hitbox de cada objeto.
// =========================================================
static void GetModelBBox(const std::string& modelName,
                         glm::vec3& outMin, glm::vec3& outMax)
{
    // Defaults conservadores caso o modelo não seja encontrado
    outMin = glm::vec3(-1.0f, 0.0f, -1.0f);
    outMax = glm::vec3( 1.0f, 2.0f,  1.0f);

    auto it = g_ModelRegistry.find(modelName);
    if (it == g_ModelRegistry.end() || it->second.parts.empty())
        return;

    outMin = it->second.parts[0].bbox_min;
    outMax = it->second.parts[0].bbox_max;

    for (const auto& part : it->second.parts)
    {
        outMin = glm::min(outMin, part.bbox_min);
        outMax = glm::max(outMax, part.bbox_max);
    }
}
