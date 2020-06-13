#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec3 aTangent;

layout (set = 3, binding = 0) uniform Model {
    mat4 matrix;
} model;
layout (set = 4, binding = 0) uniform Rotation {
    mat4 matrix;
} rotation;

layout (location = 0) out VS_OUT{
    vec3 worldPosition;
    vec3 worldNormal;
    vec2 texCoord;
} vs_out;

void main() {
    gl_Position = model.matrix * vec4(aPos, 1.0f);
    vs_out.worldPosition = vec3((model.matrix * vec4(aPos, 1.0f)).xyz);
    vs_out.worldNormal = mat3(rotation.matrix) * normalize(aNormal);
    vs_out.texCoord = aTexCoord;
}