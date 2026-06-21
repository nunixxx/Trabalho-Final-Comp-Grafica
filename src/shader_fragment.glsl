#version 330 core

in vec4 position_world;
in vec4 normal;
in vec4 position_model;
in vec2 texcoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int object_id;

uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Array de texturas (suporta até 128 materiais diferentes)
uniform sampler2D texture_map[128];
uniform int texture_index; // índice da textura do objeto atual
uniform int has_texture;   // 1 se tem textura, 0 se não tem

out vec4 color;

#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Normal interpolada e normalizada
    vec4 n = normalize(normal);

    // Duas fontes de luz direcionais
    vec4 l1 = normalize(vec4( 1.0,  1.5,  0.5, 0.0)); // luz principal (cima-direita-frente)
    vec4 l2 = normalize(vec4(-1.0, -0.5, -1.0, 0.0)); // luz de preenchimento

    float lambert1 = max(0.0, dot(n, l1));
    float lambert2 = max(0.0, dot(n, l2)) * 0.3;

    float ambient = 0.2;
    float illumination = lambert1 + lambert2 + ambient;
    // Clamp para não estourar
    illumination = clamp(illumination, 0.0, 1.5);

    vec3 base_color;

    if (has_texture == 1)
    {
        // Amostra a textura usando as coordenadas UV vindas do OBJ
        vec4 tex_color = texture(texture_map[texture_index], texcoords);
        base_color = tex_color.rgb;
    }
    else
    {
        // Fallback: cor cinza-amarronzada estilo Doom
        base_color = vec3(0.65, 0.60, 0.55);
    }

    color.rgb = base_color * illumination;
    color.a   = 1.0;

    // Correção gamma
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));
}
