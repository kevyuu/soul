#pragma once

#include "core/vec.h"

namespace soul
{
  struct AABB {
    vec3f min = vec3f::Fill(std::numeric_limits<f32>::max());
    vec3f max = vec3f::Fill(std::numeric_limits<f32>::lowest());

    AABB() = default;

    AABB(const vec3f& min, const vec3f& max) noexcept : min{min}, max{max} {}

    [[nodiscard]]
    auto is_empty() const -> bool
    {
      return (min.x >= max.x || min.y >= max.y || min.z >= max.z);
    }

    [[nodiscard]]
    auto is_inside(const vec3f& point) const -> bool
    {
      return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) &&
             (point.z >= min.z && point.z <= max.z);
    }

    struct Corners {
      static constexpr usize COUNT = 8;
      vec3f vertices[COUNT];
    };

    [[nodiscard]]
    auto get_corners() const -> Corners
    {
      return {
        vec3f(min.x, min.y, min.z),
        vec3f(min.x, min.y, max.z),
        vec3f(min.x, max.y, min.z),
        vec3f(min.x, max.y, max.z),
        vec3f(max.x, min.y, min.z),
        vec3f(max.x, min.y, max.z),
        vec3f(max.x, max.y, min.z),
        vec3f(max.x, max.y, max.z)};
    }

    [[nodiscard]]
    auto center() const -> vec3f
    {
      return (min + max) / 2.0f;
    }
  };
} // namespace soul
