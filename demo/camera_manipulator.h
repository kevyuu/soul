#pragma once

#include "core/type.h"

class CameraManipulator {
public:
	struct Config {
		float zoomSpeed;
		float orbitSpeed;
		
		soul::Vec3f upAxis;
	};

	CameraManipulator(const Config& config);

	void setCamera(soul::Vec3f cameraPosition, soul::Vec3f cameraTarget, soul::Vec3f cameraUp);
	void getCamera(soul::Vec3f* cameraPosition, soul::Vec3f* cameraTarget, soul::Vec3f* cameraUp);
	soul::Vec3f getCameraTarget();
	void setCameraTarget(soul::Vec3f target);

	void zoom(float delta);
	void orbit(float dx, float dy);
	void pan(float dx, float dy);

	soul::Mat4f getTransformMatrix();

private:
	
	soul::Vec3f _position;
	soul::Vec3f _target;
	soul::Vec3f _up;
	float _distance;
	
	float _minDistance;

	Config _config;

	void _recalculateUpVector();

};