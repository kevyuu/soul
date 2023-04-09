#include <algorithm>
#include <cmath>

#include "camera_manipulator.h"
#include "math/math.h"

CameraManipulator::CameraManipulator(
  const Config& config, soul::vec3f position, soul::vec3f target, soul::vec3f up)
    : position_(position),
      target_(target),
      up_(up),
      distance_(length(target_ - position_)),
      min_distance_(0.1f),
      config_(config)
{
}

auto CameraManipulator::set_camera(
  const soul::vec3f camera_position, const soul::vec3f camera_target, const soul::vec3f camera_up)
  -> void
{
  position_ = camera_position;
  target_ = camera_target;
  up_ = camera_up;
  distance_ = length(target_ - position_);
}

auto CameraManipulator::get_camera(
  soul::vec3f* camera_position, soul::vec3f* camera_target, soul::vec3f* camera_up) const -> void
{
  *camera_position = position_;
  *camera_target = target_;
  *camera_up = up_;
}

auto CameraManipulator::get_camera_target() const -> soul::vec3f { return target_; }

auto CameraManipulator::set_camera_target(soul::vec3f target) -> void { target_ = target; }

auto CameraManipulator::zoom(float delta) -> void
{
  const soul::vec3f look_dir = soul::math::normalize(target_ - position_);

  const soul::vec3f movement = look_dir * delta * config_.zoom_speed;
  position_ += movement;

  if (dot(look_dir, target_ - position_) < min_distance_) {
    position_ = target_ - (look_dir * min_distance_);
  }
  distance_ = length(target_ - position_);

  recalculate_up_vector();
}

auto CameraManipulator::orbit(float dx, float dy) -> void
{
  soul::vec3f orbit_dir = soul::math::normalize(position_ - target_);
  auto orbit_phi = std::asin(orbit_dir.y);
  auto orbit_theta = std::atan2(orbit_dir.z, orbit_dir.x);

  orbit_phi += (dy * config_.orbit_speed);
  orbit_theta += (dx * config_.orbit_speed);

  static constexpr auto MAX_PHI = (soul::math::fconst::PI / 2 - 0.001f);

  orbit_phi = std::clamp(orbit_phi, -MAX_PHI, MAX_PHI);

  orbit_dir.y = sin(orbit_phi);
  orbit_dir.x = cos(orbit_phi) * cos(orbit_theta);
  orbit_dir.z = cos(orbit_phi) * sin(orbit_theta);
  position_ = target_ + orbit_dir * distance_;
  recalculate_up_vector();
}

auto CameraManipulator::pan(float dx, float dy) -> void
{
  const soul::vec3f camera_dir = soul::math::normalize(target_ - position_);
  const soul::vec3f camera_right = soul::math::normalize(cross(camera_dir, config_.up_axis));

  const soul::vec3f movement = 0.001f * (-dx * camera_right + dy * up_);
  target_ += movement;
  position_ += movement;
}

auto CameraManipulator::recalculate_up_vector() -> void
{
  const soul::vec3f camera_dir = soul::math::normalize(target_ - position_);
  const soul::vec3f camera_right = soul::math::normalize(cross(camera_dir, config_.up_axis));
  up_ = soul::math::normalize(cross(camera_right, camera_dir));
}

auto CameraManipulator::get_view_matrix() const -> soul::mat4f
{
  return soul::math::look_at(position_, target_, up_);
}

auto CameraManipulator::get_transform_matrix() const -> soul::mat4f
{
  return soul::math::inverse(soul::math::look_at(position_, target_, up_));
}
