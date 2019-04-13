layout(std140) uniform CameraData{
	mat4 _camera_projection;
	mat4 _camera_view;
	mat4 _camera_projectionView;
	mat4 _camera_invProjectionView;

	mat4 _camera_prevProjection;
	mat4 _camera_prevView;
	mat4 _camera_prevProjectionView;

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


