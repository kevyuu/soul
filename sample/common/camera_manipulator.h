#pragma once

#include "core/type.h"

class CameraManipulator
{
public:
  struct Config {
    float zoom_speed;
    float orbit_speed;

    soul::vec3f32 up_axis;
  };

  explicit CameraManipulator(
    const Config& config,
    soul::vec3f32 position = soul::vec3f32(),
    soul::vec3f32 target = soul::vec3f32(),
    soul::vec3f32 up = soul::vec3f32());

  auto set_camera(soul::vec3f32 camera_position, soul::vec3f32 camera_target, soul::vec3f32 camera_up)
    -> void;
  auto get_camera(
    soul::vec3f32* camera_position, soul::vec3f32* camera_target, soul::vec3f32* camera_up) const -> void;
  auto get_position() -> soul::vec3f32;
  auto get_camera_target() const -> soul::vec3f32;
  auto set_camera_target(soul::vec3f32 target) -> void;

  auto zoom(float delta) -> void;
  auto orbit(float dx, float dy) -> void;
  auto pan(float dx, float dy) -> void;

  auto get_view_matrix() const -> soul::mat4f32;
  auto get_transform_matrix() const -> soul::mat4f32;

private:
  soul::vec3f32 position_ = soul::vec3f32();
  soul::vec3f32 target_ = soul::vec3f32();
  soul::vec3f32 up_ = soul::vec3f32();
  float distance_ = 0.0f;

  float min_distance_;

  Config config_;

  auto recalculate_up_vector() -> void;
};
