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

out vec4 color;

#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Posição da câmera no mundo
    vec4 origin          = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // Normal interpolada e normalizada
    vec4 n = normalize(normal);

    // Duas fontes de luz direcionais para iluminar o mapa por todos os ângulos
    vec4 l1 = normalize(vec4( 1.0,  1.5,  0.5, 0.0)); // luz principal (cima-direita-frente)
    vec4 l2 = normalize(vec4(-1.0, -0.5, -1.0, 0.0)); // luz de preenchimento (baixo-esquerda-trás)

    float lambert1 = max(0.0, dot(n, l1));
    float lambert2 = max(0.0, dot(n, l2)) * 0.3; // luz de preenchimento mais fraca

    // Luz ambiente para não ficar completamente escuro
    float ambient = 0.15;

    float illumination = lambert1 + lambert2 + ambient;

    // Cor base neutra (paredes do Doom têm tons de cinza-amarronzado)
    vec3 base_color = vec3(0.65, 0.60, 0.55);

    color.rgb = base_color * illumination;
    color.a   = 1.0;

    // Correção gamma
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));
}
