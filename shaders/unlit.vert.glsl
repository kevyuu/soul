#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

struct SunLight {
	vec3 direction;
	vec3 color;
};

struct Camera {
	mat4 projection;
	mat4 view;
	mat4 projectionView;
	mat4 invProjectionView;

	vec3 position;
};

layout(set = 0, binding = 0, std140) uniform SceneData{
	Camera camera;
	SunLight light;
	uint sunlightCount;
};

layout (set = 1, binding = 0) uniform PerModel {
	mat4 modelMat;
} perModel;

layout (location = 0) out VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
} vs_out;

mat4 camera_getProjectionMat() {
	return camera.projection;
}

mat4 camera_getViewMat() {
	return camera.view;
}

mat4 camera_getProjectionViewMat() {
	return camera.projectionView;
}

mat4 camera_getInvProjectionViewMat() {
	return camera.invProjectionView;
}

vec3 camera_getPosition() {
	return camera.position;
}

void main() {
	gl_Position = camera_getProjectionViewMat() * perModel.modelMat * vec4(aPos, 1.0f);
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	vs_out.worldPosition = vec3((perModel.modelMat * vec4(aPos, 1.0f)).xyz);

	mat3 rotMat = mat3(transpose(inverse(perModel.modelMat)));
	vs_out.worldNormal = normalize(rotMat * aNormal);

}