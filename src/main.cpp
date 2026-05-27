//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
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

// Antiga declaração de ObjModel

void LoadShadersFromFiles();
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
void LoadObjModelAsset(const char* name, const char* obj_path, const char* texture_basepath = nullptr);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);


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

struct PlayerInfo
{
    glm::vec4 position;
    glm::mat4 matrixScale;
    float yaw;
    int health;
    int armor;
    float movement_speed;
};

struct FreeCamInfo
{
    glm::vec4 position;
    float yaw;
    float pitch;
};

struct HealthPack {
    glm::vec4 position;
    bool active;
    float scale;
};

struct EnemieInfo
{
    glm::vec4 position;
    glm::mat4x4 matrixScale;
    float yaw;
    // int health;
    // float movement_speed
};

// =========================================================
// Sistema de texturas
// =========================================================
// Mapeia nome do material -> índice na unidade de textura OpenGL
std::map<std::string, int>  g_MaterialTextureIndex;
// Mapeia nome do arquivo de textura -> GLuint (texture object)
std::map<std::string, GLuint> g_LoadedTextures;
// Lista ordenada de GLuints para bind no shader (slot 0..N)
std::vector<GLuint> g_TextureSlots;

std::vector<HealthPack> healthPacks;

GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_texture_index_uniform;
GLint g_has_texture_uniform;

glm::vec4 camera_position_c;
glm::vec4 camera_view_vector;

float g_ScreenRatio = 1.0f;


bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;
bool g_MiddleMouseButtonPressed = false;

float g_CameraTheta = INITIAL_CAMERA_THETA;
float g_CameraPhi = INITIAL_CAMERA_PHI;
float g_CameraDistance = INITIAL_CAMERA_DISTANCE;

bool g_CamMode = LOOKAT;

CollisionMesh g_CollisionMesh;

PlayerInfo g_Player = {
    .position = PLAYER_INITIAL_POSITION,
    .matrixScale = SOLDIERS_SCALE,
    .yaw = PLAYER_INITIAL_YAW,
    .health = PLAYER_INITIAL_HEALTH,
    .armor = PLAYER_INITIAL_ARMOR,
    .movement_speed = PLAYER_INITIAL_SPEED
};

FreeCamInfo g_FreeCam = { g_Player.position, g_Player.yaw, 0.0f };

FreeCamInfo g_FreeCamBackup = g_FreeCam;

glm::vec4 g_LookAtTarget = g_Player.position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);

bool g_ShowInfoText = true;

GLuint g_GpuProgramID = 0;

// O registro que guarda a geometria pronta para ser instanciada
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

    //#ifdef __APPLE__
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //#endif

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

    /*
    printf("Lendo assets.csv...\n");
    LoadPathsCSV(PATH_CSV);

    for (const auto& pair : g_PathsRegistry) 
    {
        // pair.first é a chave (ex: "soldier", "map")
        // pair.second é a struct ModelPaths com os dados
        const std::string& name = pair.first;
        const ModelPaths& paths = pair.second;

        printf("\n--- Construindo asset: %s ---\n", name.c_str());
        
        // Pega o caminho do OBJ
        ObjModel tempModel(paths.GetModelPath().c_str());
        ComputeNormals(&tempModel);
        
        // Pega o caminho da textura
        if(paths.GetUseTexture()){
                LoadMaterialTextures(&tempModel, paths.GetTexturePath().c_str());
        }
        
        // Salva no registro de renderização usando o nome do objeto
        g_ModelRegistry[name] = BuildModelAsset(&tempModel); 
    }

    */

    LoadModelsFromCSV(PATH_CSV);

    printf("\nTodos os modelos foram carregados com sucesso!\n\n");

    // Adiciona dois health packs de teste na cena:
    healthPacks.push_back({glm::vec4( 5.0f, -1.0f,  5.0f, 1.0f), true, 0.5f}); // Posição 1
    healthPacks.push_back({glm::vec4(-5.0f, -1.0f, -5.0f, 1.0f), true, 0.5f}); // Posição 2

    g_CollisionMesh = BuildCollisionMesh(g_ModelRegistry["map"], 0.05f);
    
    TextRendering_Init();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Ativa todas as texturas (uma vez, antes do loop)
    glUseProgram(g_GpuProgramID);
    BindAllTextures(g_GpuProgramID);
    glUseProgram(0);

    while (!glfwWindowShouldClose(window))
    {

        
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_GpuProgramID);

        if (g_CamMode == LOOKAT)
        {
            float r = g_CameraDistance;
            float y = r * sin(g_CameraPhi);

            float behindAngle = g_Player.yaw + PI;  // direção de trás do soldado
            float z = r * cos(g_CameraPhi) * cos(behindAngle);
            float x = r * cos(g_CameraPhi) * sin(behindAngle);

            glm::vec4 lookat = g_LookAtTarget;
            camera_position_c  = lookat + glm::vec4(x, y, z, 0.0f);
            camera_view_vector = lookat - camera_position_c;

            glm::vec3 front(
                cos(g_Player.yaw),
                0.0f,
                sin(g_Player.yaw)
            );
            glm::vec3 right = glm::vec3(front.z, 0.0f, -front.x); // perpendicular no plano XZ

            glm::vec3 desiredPos(g_Player.position.x,
                                g_Player.position.y,
                                g_Player.position.z);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) desiredPos += right * g_Player.movement_speed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) desiredPos -= right * g_Player.movement_speed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) desiredPos -= front * g_Player.movement_speed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) desiredPos += front * g_Player.movement_speed;

            // Resolução de colisão por triângulos (substitui o AABB antigo)
            glm::vec3 resolvedPos = ResolvePlayerCollision(
                g_CollisionMesh,
                desiredPos,
                PLAYER_RADIUS,
                PLAYER_HEIGHT
            );

            g_Player.position = glm::vec4(resolvedPos.x, resolvedPos.y, resolvedPos.z, 1.0f);

            // Atualiza alvo da câmera LookAt
            g_LookAtTarget = g_Player.position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);

            glm::mat4 soldierModelMatrix =
                Matrix_Translate(g_Player.position.x, g_Player.position.y, g_Player.position.z)
                * Matrix_Rotate_Y(g_Player.yaw)
                * SOLDIERS_SCALE;

            DrawModel("soldier", soldierModelMatrix); // <-- Desenha o soldado usando a mesma função
        }
        if(g_CamMode == FREECAM)
        {
            glm::vec4 front;
            front.x = cos(g_FreeCam.pitch) * cos(g_FreeCam.yaw);
            front.y = sin(g_FreeCam.pitch);
            front.z = cos(g_FreeCam.pitch) * sin(g_FreeCam.yaw);
            front.w = 0.0f;

            camera_position_c  = g_FreeCam.position;
            camera_view_vector = front;

            glm::vec4 right = crossproduct(front, glm::vec4(0, 1, 0, 0));
            right = normalize(right);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                g_FreeCam.position += front * CAMERA_SPEED;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                g_FreeCam.position -= front * CAMERA_SPEED;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                g_FreeCam.position -= right * CAMERA_SPEED;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                g_FreeCam.position += right * CAMERA_SPEED;
        }

        glm::mat4 view = Matrix_Camera_View(
            camera_position_c,
            camera_view_vector,
            CAMERA_UP_VECTOR
        );

        float field_of_view = PI / 3.0f;
        glm::mat4 projection = Matrix_Perspective(field_of_view, g_ScreenRatio, NEARPLANE, FARPLANE);

        glUniformMatrix4fv(g_view_uniform,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        // =========================================================
        // Renderiza o mapa Doom
        // =========================================================
        glm::mat4 mapModelMatrix = Matrix_Scale(0.05f, 0.05f, 0.05f);
        DrawModel("map", mapModelMatrix); // <-- Renderização direta e rápida

        // =========================================================
        // Renderiza o inimigo
        glm::mat4 enemieModelMatrix =
                Matrix_Translate(-15.0f, 0.0f, -10.0f)
                * Matrix_Rotate_Y(PI / 4.0f)
                * SOLDIERS_SCALE;
        DrawModel("enemie", enemieModelMatrix); // <-- Renderização direta e rápida

        glm::mat4 enemieGunMatrix =
                Matrix_Translate(-10.0f, 0.0f, -10.0f)
                * Matrix_Rotate_Y(PI / 4.0f)
                * Matrix_Scale(2.0f, 2.0f, 2.0f);
        DrawModel("enemieGun", enemieGunMatrix); // <-- Renderização direta e rápida

        glm::mat4 shotgunMatrix =
                Matrix_Translate(-5.0f, 0.0f, -10.0f)
                * Matrix_Rotate_Y(PI / 4.0f)
                * Matrix_Scale(0.05f, 0.05f, 0.05f);
        DrawModel("shotgun", shotgunMatrix); // <-- Renderização direta e rápida

        float current_time = (float)glfwGetTime();
        float healthpack_angle = current_time * 1.5f; // Multiplicador define a velocidade do giro

        for (size_t i = 0; i < healthPacks.size(); i++)
        {
            if (healthPacks[i].active)
            {
                glm::mat4 hpModelMatrix = 
                      Matrix_Translate(healthPacks[i].position.x, healthPacks[i].position.y, healthPacks[i].position.z)
                    * Matrix_Rotate_Y(healthpack_angle)
                    * Matrix_Scale(healthPacks[i].scale, healthPacks[i].scale, healthPacks[i].scale);

                DrawModel("healthpack", hpModelMatrix); 
            }
        }

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
    if (g_LeftMouseButtonPressed && g_CamMode == LOOKAT)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        //g_CameraTheta -= 0.01f * dx;
        g_Player.yaw -= 0.01f * dx;   // mouse gira o soldado
        g_CameraPhi   += 0.01f * dy;

        float phimax =  PI / 2;
        float phimin = -phimax;
        if (g_CameraPhi > phimax) g_CameraPhi = phimax;
        if (g_CameraPhi < phimin) g_CameraPhi = phimin;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_CamMode == FREECAM)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_FreeCam.yaw += 0.003f * dx;
        g_FreeCam.pitch -= 0.003f * dy;

        float limit = PI / 2.0f - 0.01f;
        if (g_FreeCam.pitch >  limit) g_FreeCam.pitch =  limit;
        if (g_FreeCam.pitch < -limit) g_FreeCam.pitch = -limit;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_CameraDistance -= 0.5f * yoffset;
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void Correcao_KeyCallback(int key, int action, int mod);

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    Correcao_KeyCallback(key, action, mod);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_SEMICOLON && action == GLFW_PRESS)
    {
        g_CamMode = !g_CamMode;
        if (g_CamMode == FREECAM)
        {
            // LookAt → FreeCam: restaura o estado salvo anteriormente
            g_FreeCam.position = g_FreeCamBackup.position;
            g_FreeCam.yaw      = g_FreeCamBackup.yaw;
            g_FreeCam.pitch    = g_FreeCamBackup.pitch;

            glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            // FreeCam → LookAt: salva o estado atual da FreeCam
            g_FreeCamBackup.position = g_FreeCam.position;
            g_FreeCamBackup.yaw      = g_FreeCam.yaw;
            g_FreeCamBackup.pitch    = g_FreeCam.pitch;

            // Spawn do personagem na posição atual da FreeCam
            g_Player.position = g_FreeCam.position;
            g_Player.yaw      = g_FreeCam.yaw;

            printf("\n[POSICAO ATUAL]\n");
            printf("g_PlayerSpawnPosition = glm::vec4(%.2ff, %.2ff, %.2ff, 1.0f);\n", 
                g_Player.position.x, g_Player.position.y, g_Player.position.z);
            printf("g_PlayerSpawnYaw      = %.2ff;\n\n", g_Player.yaw);
            fflush(stdout);

            // Herda posição da FreeCam como novo alvo da LookAt
            glm::vec4 front;
            front.x = cos(g_FreeCam.pitch) * cos(g_FreeCam.yaw);
            front.y = sin(g_FreeCam.pitch);
            front.z = cos(g_FreeCam.pitch) * sin(g_FreeCam.yaw);
            front.w = 0.0f;
            front   = normalize(front);

            // O ponto que a câmera estava "olhando" vira o novo lookat_target
            float orbit_dist = 4.0f;
            glm::vec4 lookat_target = g_FreeCam.position + front * orbit_dist;

            // Recalcula theta/phi/distance
            glm::vec4 diff = g_FreeCam.position - lookat_target;
            g_LookAtTarget = g_Player.position + glm::vec4(0.0f, 0.8f, 0.0f, 0.0f);
            g_CameraDistance = norm(diff);

            if (g_CameraDistance < 0.01f) g_CameraDistance = orbit_dist;

            g_CameraPhi   = asin(diff.y / g_CameraDistance);
            g_CameraTheta = atan2(diff.x, diff.z);

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
        g_ShowInfoText = !g_ShowInfoText;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        // Re-bind texturas após recarregar shaders
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

    static float old_seconds    = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20]     = "?? fps";
    static int   numchars       = 7;

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
    TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1)*charwidth, 1.0f - lineheight, 1.0f);
}