#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aBinormal;
layout (location = 4) in vec3 aTangent;

uniform mat4 model;
uniform mat4 shadowMatrix;

void main() {
	gl_Position = shadowMatrix * model * vec4(aPos, 1.0f);
}