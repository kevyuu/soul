#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#pragma shader_stage(vertex)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aBinormal;
layout (location = 4) in vec3 aTangent;

layout (set = 1, binding = 0) uniform PerLight{
	mat4 matrix;
} perLight;

layout (set = 3, binding = 0) uniform PerModel {
	mat4 matrix;
} perModel;

void main() {
	gl_Position = perLight.matrix * perModel.matrix * vec4(aPos, 1.0f);
}
