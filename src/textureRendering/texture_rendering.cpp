#include "texture_rendering.h"
#include "..\constants.h"
#include <glad/glad.h>
#include <string>
#include <map>
#include <stb_image.h>
#include "..\modelRendering\model_rendering.h"


extern std::map<std::string, GLuint> g_LoadedTextures;
extern std::map<std::string, int> g_MaterialTextureIndex;
extern std::vector<GLuint> g_TextureSlots;


GLuint LoadTextureImage(const char* filename)
{
    // Cache: não recarrega a mesma imagem
    auto it = g_LoadedTextures.find(std::string(filename));
    if (it != g_LoadedTextures.end())
        return it->second;

    printf("Carregando textura \"%s\"... ", filename);

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    
    // MUDANÇA 1: Usar '0' em vez de '4' para que a biblioteca detecte os canais reais
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);

    if (data == NULL)
    {
        fprintf(stderr, "AVISO: Não foi possível abrir \"%s\".\n", filename);
        g_LoadedTextures[std::string(filename)] = 0;
        return 0;
    }

    printf("OK (%dx%d, %d canais).\n", width, height, channels);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // MUDANÇA 2: Configurar o OpenGL de acordo com a quantidade de canais
    GLenum format = GL_RGB;
    if (channels == 4)
        format = GL_RGBA;
    else if (channels == 1)
        format = GL_RED;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // MUDANÇA 3: Passar o formato dinâmico aqui
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    g_LoadedTextures[std::string(filename)] = texture_id;
    return texture_id;
}

void LoadMaterialTextures(ObjModel* model, const char* basepath)
{

    std::string base(basepath ? basepath : "");

    for (size_t i = 0; i < model->materials.size(); ++i)
    {
        const tinyobj::material_t& mat = model->materials[i];
        const std::string& matname = mat.name;

        // Já processamos este material?
        if (g_MaterialTextureIndex.count(matname))
            continue;

        if (mat.diffuse_texname.empty())
        {
            // Sem textura: índice -1
            g_MaterialTextureIndex[matname] = -1;
            continue;
        }

        // Monta o caminho completo para o arquivo de textura
        std::string texpath = base + mat.diffuse_texname;

        GLuint tex_id = LoadTextureImage(texpath.c_str());

        if (tex_id == 0)
        {
            // Tenta com extensão em minúsculas caso falhe
            std::string lower = texpath;
            for (auto& c : lower) c = tolower(c);
            tex_id = LoadTextureImage(lower.c_str());
        }

        if (tex_id == 0)
        {
            g_MaterialTextureIndex[matname] = -1;
            continue;
        }

        // Atribui ao próximo slot disponível
        int slot = (int)g_TextureSlots.size();
        if (slot >= MAX_TEXTURES)
        {
            fprintf(stderr, "AVISO: Limite de %d texturas atingido. Material '%s' sem textura.\n",
                    MAX_TEXTURES, matname.c_str());
            g_MaterialTextureIndex[matname] = -1;
            continue;
        }

        g_TextureSlots.push_back(tex_id);
        g_MaterialTextureIndex[matname] = slot;
        printf("Material '%s' -> slot %d (tex_id=%u)\n", matname.c_str(), slot, tex_id);
    }
}

// =========================================================
// Ativa todas as texturas nos slots correspondentes
// Deve ser chamado após UseProgram e antes do draw loop
// =========================================================
void BindAllTextures(GLuint program_id)
{
    // Reserva a unidade 31 para o text rendering
    // Usamos unidades 0..N-1 para nossas texturas
    for (int i = 0; i < (int)g_TextureSlots.size() && i < MAX_TEXTURES; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, g_TextureSlots[i]);
    }

    // Passa o array de samplers para o shader
    // (precisa passar os índices das unidades de textura, não os IDs)
    int samplers[MAX_TEXTURES];
    for (int i = 0; i < MAX_TEXTURES; ++i)
        samplers[i] = i;

    GLint loc = glGetUniformLocation(program_id, "texture_map");
    if (loc >= 0)
        glUniform1iv(loc, MAX_TEXTURES, samplers);
}