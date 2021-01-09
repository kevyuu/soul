#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 aPos;

layout (set = 1, binding = 0) uniform PerLight{
	mat4 matrix;
} perLight;

layout (set = 3, binding = 0) uniform PerModel {
	mat4 matrix;
} perModel;

void main() {
	gl_Position = perLight.matrix * perModel.matrix * vec4(aPos, 1.0f);
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
