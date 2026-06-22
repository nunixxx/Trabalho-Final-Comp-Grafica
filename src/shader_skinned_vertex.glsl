#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal_in;
layout(location = 2) in vec2 texcoords_in;
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4  boneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneMatrices[100]; // MAX_BONES

out vec4 position_world;
out vec4 normal;
out vec2 texcoords;

void main()
{
    // Acumula a transformação ponderada de cada bone
    mat4 skinMatrix = mat4(0.0);
    for (int i = 0; i < 4; i++) {
        if (boneIDs[i] < 0) continue;
        skinMatrix += boneWeights[i] * boneMatrices[boneIDs[i]];
    }
    // Fallback: sem bones, usa identidade
    if (skinMatrix == mat4(0.0)) skinMatrix = mat4(1.0);

    vec4 skinnedPos    = skinMatrix * vec4(position, 1.0);
    vec4 skinnedNormal = skinMatrix * vec4(normal_in, 0.0);

    gl_Position   = projection * view * model * skinnedPos;
    position_world = model * skinnedPos;
    normal        = normalize(inverse(transpose(model)) * skinnedNormal);
    texcoords     = texcoords_in;
}