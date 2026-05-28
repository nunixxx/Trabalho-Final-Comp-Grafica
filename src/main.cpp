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

#include "classes/enemy/enemy.h"
#include "classes/player/player.h"
#include "classes/objects/health_pack.h"
#include "classes/objects/world_object.h"
#include "classes/objects/gun.h"

void LoadShadersFromFiles();
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
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

// Instancia de Objetos
std::unique_ptr<Player> g_Player;
std::vector<std::unique_ptr<Enemy>> g_Enemies;
std::vector<std::unique_ptr<HealthPack>> g_HealthPacks;
std::vector<std::unique_ptr<Gun>> g_Guns;
// =========================================================
// Sistema de texturas
// =========================================================
// Mapeia nome do material -> índice na unidade de textura OpenGL
std::map<std::string, int>  g_MaterialTextureIndex;
// Mapeia nome do arquivo de textura -> GLuint (texture object)
std::map<std::string, GLuint> g_LoadedTextures;
// Lista ordenada de GLuints para bind no shader (slot 0..N)
std::vector<GLuint> g_TextureSlots;

GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_texture_index_uniform;
GLint g_has_texture_uniform;

float g_ScreenRatio = 1.0f;

bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;
bool g_MiddleMouseButtonPressed = false;

CollisionMesh g_CollisionMesh;

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

    // Adiciona dois health packs de teste na cena:
    g_HealthPacks.push_back(std::make_unique<HealthPack>(glm::vec4( 5.0f, -1.0f,  5.0f, 1.0f), 0.5f));
    g_HealthPacks.push_back(std::make_unique<HealthPack>(glm::vec4(-5.0f, -1.0f, -5.0f, 1.0f), 0.5f));

    // Adiciona duas armas de teste na cena:
    g_Guns.push_back(std::make_unique<ShotGun>(glm::vec4( -5.0f, 1.0f, -10.0f, 1.0f), 0.05f, 10, 50));
    g_Guns.push_back(std::make_unique<Pistol>(glm::vec4(-10.0f, -1.0f, -10.0f, 1.0f), 2.0f, 15, 50));

    // Inimigo com rota manual (4 pontos de controle explícitos)
    g_Enemies.push_back(std::make_unique<Enemy>(
        glm::vec4(-0.0f, 0.0f, -15.0f, 1.0f),  // posição inicial
        PI / 4.0f,                                // yaw inicial
        std::array<glm::vec4, 4>{
            glm::vec4(-15.0f, 0.0f, -10.0f, 1.0f),
            glm::vec4( -5.0f, 0.0f, -10.0f, 1.0f),
            glm::vec4( -5.0f, 0.0f,  -5.0f, 1.0f),
            glm::vec4(-15.0f, 0.0f,  -5.0f, 1.0f)
        }
    ));

    // Inimigo com rota gerada automaticamente (construtor simples)
    g_Enemies.push_back(std::make_unique<Enemy>(
        glm::vec4(-10.0f, 0.0f, -15.0f, 1.0f),  // posição inicial
        0.0f,                                     // yaw inicial
        4.0f                                     // raio do patrol
    ));

    g_Player = std::make_unique<Player>(
        window,
        &g_CollisionMesh,
        PLAYER_INITIAL_POSITION,
        PLAYER_INITIAL_YAW
    );
    
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

        // Calcula deltaTime uma vez por frame
        static float lastTime = (float)glfwGetTime();
        float currentTime = (float)glfwGetTime();
        float deltaTime   = currentTime - lastTime;
        lastTime          = currentTime;

        // Atualiza o player (input + movimento + câmera)
        g_Player->update(deltaTime);

        // Monta view/projection com os vetores expostos pelo Player
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

        // Renderiza o personagem (em FreeCam não desenha nada — já tratado dentro do Player)
        g_Player->draw();

        glUniformMatrix4fv(g_view_uniform,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        // =========================================================
        // Renderiza o mapa Doom
        // =========================================================
        glm::mat4 mapModelMatrix = Matrix_Scale(0.05f, 0.05f, 0.05f);
        DrawModel("map", mapModelMatrix); // <-- Renderização direta e rápida

        for (auto& enemy : g_Enemies)
        {
            enemy->update(deltaTime, g_Player.get());
            enemy->draw();
        }

        for (auto& gun : g_Guns)
        {
            gun->update(deltaTime, g_Player.get());
            gun->draw();
        }

        for (auto& obj : g_HealthPacks)
        {
            obj->update(deltaTime, g_Player.get());
            obj->draw();
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
    if (!g_Player) return;

    static double lastX = xpos, lastY = ypos;
    float dx = (float)(xpos - lastX);
    float dy = (float)(ypos - lastY);
    lastX = xpos;
    lastY = ypos;

    // Só processa drag em LookAt (botão esquerdo), FreeCam sempre processa
    bool drag = g_LeftMouseButtonPressed || g_Player->isFreeCam();
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