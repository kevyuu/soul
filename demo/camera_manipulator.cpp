#include <algorithm>
#include <cmath>

#include "camera_manipulator.h"
#include "core/math.h"

CameraManipulator::CameraManipulator(const Config& config)
	: _config(config), _minDistance(0.1f)
{

}

void CameraManipulator::setCamera(soul::Vec3f cameraPosition, soul::Vec3f cameraTarget, soul::Vec3f cameraUp) {
	_position = cameraPosition;
	_target = cameraTarget;
	_up = cameraUp;
	_distance = length(_target - _position);
}

void CameraManipulator::getCamera(soul::Vec3f* cameraPosition, soul::Vec3f* cameraTarget, soul::Vec3f* cameraUp) {
	*cameraPosition = _position;
	*cameraTarget = _target;
	*cameraUp = _up;
}

soul::Vec3f CameraManipulator::getCameraTarget() {
	return _target;
}

void CameraManipulator::setCameraTarget(soul::Vec3f target) {
	_target = target;
}

void CameraManipulator::zoom(float delta) {
	soul::Vec3f lookDir = unit(_target - _position);
	
	soul::Vec3f movement = lookDir * delta * _config.zoomSpeed;
	_position += movement;
	
	if (dot(lookDir, _target - _position) < _minDistance) {
		_position = _target - (lookDir * _minDistance);
	}
	_distance = length(_target - _position);

	_recalculateUpVector();
}

void CameraManipulator::orbit(float dx, float dy) {
	soul::Vec3f orbitDir = unit(_position - _target);
	float orbitPhi = std::asin(orbitDir.y);
	float orbitTheta = std::atan2(orbitDir.z, orbitDir.x);

	orbitPhi += (dy * _config.orbitSpeed);
	orbitTheta += (dx * _config.orbitSpeed);

	static constexpr float MAX_PHI = (soul::Fconst::PI / 2 - 0.001);

	orbitPhi = std::clamp(orbitPhi, -MAX_PHI, MAX_PHI);

	orbitDir.y = sin(orbitPhi);
	orbitDir.x = cos(orbitPhi) * cos(orbitTheta);
	orbitDir.z = cos(orbitPhi) * sin(orbitTheta);
	_position = _target + orbitDir * _distance;
	_recalculateUpVector();
}
void CameraManipulator::pan(float dx, float dy) {
	soul::Vec3f cameraDir = unit(_target - _position);
	soul::Vec3f cameraRight = unit(cross(cameraDir, _config.upAxis));

	soul::Vec3f movement = 0.001f * (float(-dx) * cameraRight + float(dy) * _up);
	_target += movement;
	_position += movement;
}

void CameraManipulator::_recalculateUpVector() {
	soul::Vec3f cameraDir = unit(_target - _position);
	soul::Vec3f cameraRight = unit(cross(cameraDir, _config.upAxis));
	_up = unit(cross(cameraRight, cameraDir));
}

soul::Mat4f CameraManipulator::getTransformMatrix() {
	return soul::mat4Inverse(soul::mat4View(_position, _target, _up));
}
