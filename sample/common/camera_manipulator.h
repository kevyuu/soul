#pragma once

#include "core/type.h"

class CameraManipulator {
public:
	struct Config {
		float zoom_speed;
		float orbit_speed;

		soul::vec3f up_axis;
	};

	explicit CameraManipulator(const Config& config, 
		soul::vec3f position = soul::vec3f(),
		soul::vec3f target = soul::vec3f(),
		soul::vec3f up = soul::vec3f());

	void set_camera(soul::vec3f camera_position, soul::vec3f camera_target, soul::vec3f camera_up);
	void get_camera(soul::vec3f* camera_position, soul::vec3f* camera_target, soul::vec3f* camera_up) const;
	soul::vec3f get_camera_target() const;
	void set_camera_target(soul::vec3f target);

	void zoom(float delta);
	void orbit(float dx, float dy);
	void pan(float dx, float dy);

	soul::mat4f get_view_matrix() const;
	soul::mat4f get_transform_matrix() const;

private:

	soul::vec3f position_ = soul::vec3f();
	soul::vec3f target_ = soul::vec3f();
	soul::vec3f up_ = soul::vec3f();
	float distance_ = 0.0f;

	float min_distance_;

	Config config_;

	void recalculate_up_vector();

};