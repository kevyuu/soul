#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aBinormal;
layout (location = 4) in vec3 aTangent;

layout (set = 3, binding = 0) uniform PerModel {
	mat4 modelMat;
} perModel;

layout (location = 0) out VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

layout(set = 0, binding = 0, std140) uniform CameraData{
	mat4 _camera_projection;
	mat4 _camera_view;
	mat4 _camera_projectionView;
	mat4 _camera_invProjectionView;

	vec3 _camera_cameraPosition;
	float _camera_exposure;
};

mat4 camera_getProjectionMat() {
	return _camera_projection;
}

mat4 camera_getViewMat() {
	return _camera_view;
}

mat3 camera_getViewRotMat() {
	return mat3(_camera_view);
}

mat4 camera_getProjectionViewMat() {
	return _camera_projectionView;
}

mat4 camera_getInvProjectionViewMat() {
	return _camera_invProjectionView;
}

vec3 camera_getPosition() {
	return _camera_cameraPosition;
}

float camera_getExposure() {
	return _camera_exposure;
}

void main() {
	gl_Position = camera_getProjectionViewMat() * perModel.modelMat * vec4(aPos, 1.0f);
	gl_Position.y = -gl_Position.y;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	vs_out.worldPosition = vec3((perModel.modelMat * vec4(aPos, 1.0f)).xyz);
	mat3 rotMat = mat3(transpose(inverse(perModel.modelMat)));
	vs_out.worldNormal = rotMat * aNormal;
	vs_out.worldTangent = rotMat * aTangent;
	vs_out.worldBinormal = rotMat * aBinormal;
	vs_out.texCoord = aTexCoord;
}