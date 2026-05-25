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

uniform sampler2D texture_map[64];
uniform int texture_index;
uniform int has_texture;

uniform vec4 camera_position;

// --- SISTEMA DE MÚLTIPLAS LUZES FIXAS ---
#define MAX_LIGHTS 10
uniform vec4 light_positions[MAX_LIGHTS];
uniform int num_lights; // Quantas luzes estão ativas de fato

out vec4 color;

void main()
{
    vec4 n = normalize(normal);
    vec4 v = normalize(camera_position - position_world);

    // Luz ambiente base para o mapa não ficar 100% escuro
    float ambient = 0.2;
    
    // Luz direcional fraca (simula a luz do sol vindo de fora)
    vec4 l2 = normalize(vec4(-1.0, -0.5, -1.0, 0.0));
    float lambert2 = max(0.0, dot(n, l2)) * 0.3;

    float total_lambert = 0.0;
    float total_specular = 0.0;
    float shininess = 64.0;
    float specular_strength = 0.6;

    // Calcula a contribuição de CADA luz fixa que você colocar no mapa
    for(int i = 0; i < num_lights; i++) {
        vec4 l = normalize(light_positions[i] - position_world);
        
        // Difusa
        float lambert = max(0.0, dot(n, l));
        total_lambert += lambert;

        // Especular (Brilho)
        if (lambert > 0.0) {
            vec4 r = reflect(-l, n);
            total_specular += pow(max(dot(r, v), 0.0), shininess) * specular_strength;
        }
    }

    // Cor da textura
    vec3 base_color;
    if (has_texture == 1) {
        base_color = texture(texture_map[texture_index], texcoords).rgb;
    } else {
        base_color = vec3(0.65, 0.60, 0.55);
    }

    // Soma a iluminação
    vec3 final_illumination = base_color * (ambient + total_lambert + lambert2);
    vec3 final_specular = vec3(1.0) * total_specular;

    final_illumination = clamp(final_illumination, 0.0, 1.5);

    color.rgb = final_illumination + final_specular;
    color.a   = 1.0;

    // Correção gamma
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));
}