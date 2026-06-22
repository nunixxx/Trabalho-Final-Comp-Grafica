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

#define MAX_POINT_LIGHTS 16

uniform int   u_num_point_lights;
uniform vec3  u_point_light_positions[MAX_POINT_LIGHTS];
uniform vec3  u_point_light_colors[MAX_POINT_LIGHTS];
uniform float u_point_light_intensities[MAX_POINT_LIGHTS];

uniform vec3  u_camera_pos;
uniform float u_shininess;
uniform float u_specular_strength;
uniform float u_ambient_intensity;

void main()
{
    vec3 N = normalize(normal).xyz;
    vec3 V = normalize(u_camera_pos - position_world.xyz);

    // Duas fontes de luz direcionais
    vec3 L1 = normalize(vec3( 1.0,  1.5,  0.5));
    vec3 L2 = normalize(vec3(-1.0, -0.5, -1.0));

    float NdotL1 = max(0.0, dot(N, L1));
    float NdotL2 = max(0.0, dot(N, L2));

    float ambient = u_ambient_intensity;
    vec3 diffuse  = NdotL1 * vec3(1.0) + NdotL2 * 0.3 * vec3(1.0);
    vec3 specular = vec3(0.0);

    // Especular das direcionais
    if (NdotL1 > 0.0)
    {
        vec3 R = reflect(-L1, N);
        specular += pow(max(dot(R, V), 0.0), u_shininess) * vec3(1.0) * u_specular_strength;
    }
    if (NdotL2 > 0.0)
    {
        vec3 R = reflect(-L2, N);
        specular += pow(max(dot(R, V), 0.0), u_shininess) * 0.3 * vec3(1.0) * u_specular_strength;
    }

    // Point lights dos objetos da cena
    for (int i = 0; i < u_num_point_lights; i++)
    {
        vec3 light_vec = position_world.xyz - u_point_light_positions[i];
        float dist = length(light_vec);
        float atten = 1.0 / (1.0 + 0.03 * dist * dist);
        vec3 L = normalize(light_vec);
        float NdotL = max(0.0, dot(N, L));

        diffuse += NdotL * u_point_light_colors[i] * u_point_light_intensities[i] * atten;

        if (NdotL > 0.0)
        {
            vec3 R = reflect(-L, N);
            specular += pow(max(dot(R, V), 0.0), u_shininess)
                        * u_point_light_colors[i] * u_point_light_intensities[i] * atten
                        * u_specular_strength;
        }
    }

    vec3 illumination = vec3(ambient) + diffuse + specular;
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
