#include <algorithm>
#include <cmath>

#include "camera_manipulator.h"
#include "core/math.h"

CameraManipulator::CameraManipulator(const Config& config)
	: _config(config), _minDistance(0.1f)
{

}

void CameraManipulator::setCamera(Soul::Vec3f cameraPosition, Soul::Vec3f cameraTarget, Soul::Vec3f cameraUp) {
	_position = cameraPosition;
	_target = cameraTarget;
	_up = cameraUp;
	_distance = length(_target - _position);
}

void CameraManipulator::getCamera(Soul::Vec3f* cameraPosition, Soul::Vec3f* cameraTarget, Soul::Vec3f* cameraUp) {
	*cameraPosition = _position;
	*cameraTarget = _target;
	*cameraUp = _up;
}

Soul::Vec3f CameraManipulator::getCameraTarget() {
	return _target;
}

void CameraManipulator::setCameraTarget(Soul::Vec3f target) {
	_target = target;
}

void CameraManipulator::zoom(float delta) {
	Soul::Vec3f lookDir = unit(_target - _position);
	
	Soul::Vec3f movement = lookDir * delta * _config.zoomSpeed;
	_position += movement;
	
	if (dot(lookDir, _target - _position) < _minDistance) {
		_position = _target - (lookDir * _minDistance);
	}
	_distance = length(_target - _position);

	_recalculateUpVector();
}

void CameraManipulator::orbit(int dx, int dy) {
	Soul::Vec3f orbitDir = unit(_position - _target);
	float orbitPhi = std::asin(orbitDir.y);
	float orbitTheta = std::atan2(orbitDir.z, orbitDir.x);

	orbitPhi += (dy * _config.orbitSpeed);
	orbitTheta += (dx * _config.orbitSpeed);

	static constexpr float MAX_PHI = (Soul::FCONST::PI / 2 - 0.001);

	orbitPhi = std::clamp(orbitPhi, -MAX_PHI, MAX_PHI);

	orbitDir.y = sin(orbitPhi);
	orbitDir.x = cos(orbitPhi) * cos(orbitTheta);
	orbitDir.z = cos(orbitPhi) * sin(orbitTheta);
	_position = _target + orbitDir * _distance;
	_recalculateUpVector();
}

void CameraManipulator::pan(int dx, int dy) {
	Soul::Vec3f cameraDir = unit(_target - _position);
	Soul::Vec3f cameraRight = unit(cross(cameraDir, _config.upAxis));

	Soul::Vec3f movement = 0.001f * (-dx * cameraRight + dy * _up);
	_target += movement;
	_position += movement;
}

void CameraManipulator::_recalculateUpVector() {
	Soul::Vec3f cameraDir = unit(_target - _position);
	Soul::Vec3f cameraRight = unit(cross(cameraDir, _config.upAxis));
	_up = unit(cross(cameraRight, cameraDir));
}

Soul::Mat4f CameraManipulator::getTransformMatrix() {
	return Soul::mat4Inverse(Soul::mat4View(_position, _target, _up));
}
