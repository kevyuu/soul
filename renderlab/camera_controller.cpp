#include <algorithm>
#include <cmath>

#include "camera_controller.h"
#include "core/matrix.h"
#include "math/math.h"
#include "math/quaternion.h"
#include "type.h"

using namespace soul;

namespace renderlab
{
  OrbitCameraController::OrbitCameraController(
    const Config& config, const mat4f32& camera_model_mat, const vec3f32& target)
      : position_(camera_model_mat.col(3).xyz()),
        target_(target),
        distance_(math::length(target_ - position_)),
        min_distance_(0.1f),
        config_(config)
  {
  }

  auto OrbitCameraController::zoom(float delta) -> void
  {
    const vec3f32 look_dir = math::normalize(target_ - position_);

    const vec3f32 movement = look_dir * delta * config_.zoom_speed;
    position_ += movement;

    if (math::dot(look_dir, target_ - position_) < min_distance_)
    {
      position_ = target_ - (look_dir * min_distance_);
    }
    distance_ = math::length(target_ - position_);
  }

  auto OrbitCameraController::orbit(vec2f32 delta) -> void
  {
    vec3f32 orbit_dir = math::normalize(position_ - target_);
    auto orbit_phi    = std::asin(orbit_dir.y);
    auto orbit_theta  = std::atan2(orbit_dir.z, orbit_dir.x);

    orbit_phi += (delta.y * config_.orbit_speed);
    orbit_theta += (delta.x * config_.orbit_speed);

    static constexpr auto MAX_PHI = (math::f32const::PI / 2 - 0.001f);

    orbit_phi = std::clamp(orbit_phi, -MAX_PHI, MAX_PHI);

    orbit_dir.y = sin(orbit_phi);
    orbit_dir.x = cos(orbit_phi) * cos(orbit_theta);
    orbit_dir.z = cos(orbit_phi) * sin(orbit_theta);
    position_   = target_ + orbit_dir * distance_;
  }

  auto OrbitCameraController::get_view_matrix() const -> mat4f32
  {
    return math::look_at(position_, target_, config_.up_axis);
  }

  auto OrbitCameraController::get_model_matrix() const -> mat4f32
  {
    return math::inverse(math::look_at(position_, target_, config_.up_axis));
  }

  FlightCameraController::FlightCameraController(
    const Config& config, const mat4f32& camera_model_mat)
      : model_mat_(camera_model_mat), config_(config)
  {
  }

  void FlightCameraController::zoom(f32 delta)
  {
    auto camera_transform         = CameraTransform::FromModelMat(model_mat_);
    const auto camera_forward_dir = camera_transform.target - camera_transform.position;

    camera_transform.target += camera_forward_dir * delta;
    camera_transform.position += camera_forward_dir * delta;

    model_mat_ = math::inverse(
      math::look_at(camera_transform.position, camera_transform.target, vec3f32(0, 1, 0)));
  }

  void FlightCameraController::pan(vec2f32 delta)
  {
    delta *= config_.pan_speed;
    auto camera_transform = CameraTransform::FromModelMat(model_mat_);
    const auto camera_dir = camera_transform.target - camera_transform.position;
    const auto new_camera_dir =
      camera_dir + math::mul(model_mat_, vec4f32(delta.x, -delta.y, 0.0f, 0.0f)).xyz();

    const auto new_target = camera_transform.position + new_camera_dir;

    model_mat_ =
      math::inverse(math::look_at(camera_transform.position, new_target, vec3f32(0, 1, 0)));
  }

  auto FlightCameraController::get_model_matrix() const -> mat4f32
  {
    return model_mat_;
  }

  MapCameraController::MapCameraController(const Config& config, const mat4f32& camera_model_mat)
      : position_(camera_model_mat.col(3).xyz()),
        camera_dir_(-camera_model_mat.col(2).xyz()),
        camera_up_(camera_model_mat.col(1).xyz()),
        camera_right_(camera_model_mat.col(0).xyz()),
        config_(config)
  {
  }

  void MapCameraController::zoom(f32 delta)
  {
    position_ = position_ + delta * config_.zoom_speed * camera_dir_;
  }

  void MapCameraController::pan(vec2f32 delta)
  {
    position_ = position_ + (-delta.x * camera_right_ * config_.pan_speed) +
                (delta.y * camera_up_ * config_.pan_speed);
  }

  auto MapCameraController::get_model_matrix() const -> mat4f32
  {
    return math::inverse(math::look_at(position_, position_ + camera_dir_, camera_up_));
  }

} // namespace renderlab
