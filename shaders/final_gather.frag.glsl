#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput renderMap1;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput renderMap2;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput renderMap3;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput renderMap4;

layout (location = 0) out vec4 finalColor;

void main() {

    vec3 albedo = subpassLoad(renderMap1).rgb;
    vec3 specularOutput = subpassLoad(renderMap2).rgb;
    vec3 diffuseOutput= subpassLoad(renderMap4).rgb;

    vec3 directColor = specularOutput + diffuseOutput;

    finalColor = vec4(directColor, 1.0f);
}