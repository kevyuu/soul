#pragma once

#include "core/type.h"

class CameraManipulator
{
public:
  struct Config {
    float zoom_speed;
    float orbit_speed;

    soul::vec3f up_axis;
  };

  explicit CameraManipulator(
    const Config& config,
    soul::vec3f position = soul::vec3f(),
    soul::vec3f target = soul::vec3f(),
    soul::vec3f up = soul::vec3f());

  auto set_camera(soul::vec3f camera_position, soul::vec3f camera_target, soul::vec3f camera_up)
    -> void;
  auto get_camera(
    soul::vec3f* camera_position, soul::vec3f* camera_target, soul::vec3f* camera_up) const -> void;
  auto get_position() -> soul::vec3f;
  auto get_camera_target() const -> soul::vec3f;
  auto set_camera_target(soul::vec3f target) -> void;

  auto zoom(float delta) -> void;
  auto orbit(float dx, float dy) -> void;
  auto pan(float dx, float dy) -> void;

  auto get_view_matrix() const -> soul::mat4f;
  auto get_transform_matrix() const -> soul::mat4f;

private:
  soul::vec3f position_ = soul::vec3f();
  soul::vec3f target_ = soul::vec3f();
  soul::vec3f up_ = soul::vec3f();
  float distance_ = 0.0f;

  float min_distance_;

  Config config_;

  auto recalculate_up_vector() -> void;
};
