#pragma once

#include "core/matrix.h"
#include "core/type.h"
#include "math/quaternion.h"
#include "runtime/data.h"
#include "type.h"

using namespace soul;

namespace renderlab
{

  class OrbitCameraController
  {
  public:
    struct Config
    {
      f32 zoom_speed  = 1.0f;
      f32 orbit_speed = 1.0f;
      vec3f32 up_axis = vec3f32(0, 1, 0);
    };

  private:
    vec3f32 position_;
    vec3f32 target_;
    f32 distance_ = 0.0f;
    f32 min_distance_;
    Config config_;

  public:
    explicit OrbitCameraController(
      const Config& config, const mat4f32& camera_model_mat, const vec3f32& target);

    [[nodiscard]]
    auto camera_transform() const -> CameraTransform;

    void zoom(f32 delta);

    void orbit(vec2f32 delta);

    [[nodiscard]]
    auto get_view_matrix() const -> mat4f32;

    [[nodiscard]]
    auto get_model_matrix() const -> mat4f32;
  };

  class FlightCameraController
  {
  public:
    struct Config
    {
      f32 zoom_speed  = 1.0f;
      f32 pan_speed   = 1.0f;
      vec3f32 up_axis = vec3f32(0, 1, 0);
    };

  private:
    mat4f32 model_mat_;
    Config config_;

  public:
    explicit FlightCameraController(const Config& config, const mat4f32& camera_model_mat);

    void zoom(f32 delta);

    void pan(vec2f32 delta);

    [[nodiscard]]
    auto get_model_matrix() const -> mat4f32;
  };

  class MapCameraController
  {
  public:
    struct Config
    {
      f32 zoom_speed = 1.0f;
      f32 pan_speed  = 1.0f;
    };

  private:
    vec3f32 position_;
    vec3f32 camera_dir_;
    vec3f32 camera_up_;
    vec3f32 camera_right_;
    Config config_;

  public:
    explicit MapCameraController(const Config& config, const mat4f32& camera_model_mat);

    void zoom(f32 delta);

    void pan(vec2f32 delta);

    [[nodiscard]]
    auto get_model_matrix() const -> mat4f32;
  };
} // namespace renderlab
