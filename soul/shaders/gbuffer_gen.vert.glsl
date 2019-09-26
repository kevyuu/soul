#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#pragma shader_stage(vertex)

#include "camera.lib.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aBinormal;
layout (location = 4) in vec3 aTangent;

layout (set = 4, binding = 0) uniform PerModel {
	mat4 modelMat;
} perModel;

layout (location = 0) out VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

void main() {
	gl_Position = camera_getProjectionViewMat() * perModel.modelMat * vec4(aPos, 1.0f);
	vs_out.worldPosition = vec3((perModel.modelMat * vec4(aPos, 1.0f)).xyz);
	mat3 rotMat = mat3(transpose(inverse(perModel.modelMat)));
	vs_out.worldNormal = rotMat * aNormal;
	vs_out.worldTangent = rotMat * aTangent;
	vs_out.worldBinormal = rotMat * aBinormal;
	vs_out.texCoord = aTexCoord;
}