#pragma once

#include "core/type.h"

class CameraManipulator {
public:
	struct Config {
		float zoomSpeed;
		float orbitSpeed;
		
		Soul::Vec3f upAxis;
	};

	CameraManipulator(const Config& config);

	void setCamera(Soul::Vec3f cameraPosition, Soul::Vec3f cameraTarget, Soul::Vec3f cameraUp);
	void getCamera(Soul::Vec3f* cameraPosition, Soul::Vec3f* cameraTarget, Soul::Vec3f* cameraUp);
	Soul::Vec3f getCameraTarget();
	void setCameraTarget(Soul::Vec3f target);

	void zoom(float delta);
	void orbit(int dx, int dy);
	void pan(int dx, int dy);

	Soul::Mat4f getTransformMatrix();

private:
	
	Soul::Vec3f _position;
	Soul::Vec3f _target;
	Soul::Vec3f _up;
	float _distance;
	
	float _minDistance;

	Config _config;

	void _recalculateUpVector();

};