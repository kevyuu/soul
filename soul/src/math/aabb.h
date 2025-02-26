#pragma once

#include <limits>

#include "core/compiler.h"
#include "core/matrix.h"
#include "core/vec.h"

#include "math/matrix.h"
#include "math/vec.h"

namespace soul::math
{
  struct AABB
  {
    vec3f32 min = vec3f32(std::numeric_limits<f32>::max());
    vec3f32 max = vec3f32(std::numeric_limits<f32>::lowest());

    AABB() = default;

    AABB(const vec3f32& min, const vec3f32& max) noexcept : min{min}, max{max} {}

    [[nodiscard]]
    auto is_empty() const -> bool
    {
      return (min.x >= max.x || min.y >= max.y || min.z >= max.z);
    }

    [[nodiscard]]
    auto is_inside(const vec3f32& point) const -> bool
    {
      return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) &&
             (point.z >= min.z && point.z <= max.z);
    }

    struct Corners
    {
      static constexpr usize COUNT = 8;
      vec3f32 vertices[COUNT];
    };

    [[nodiscard]]
    auto get_corners() const -> Corners
    {
      return {
        vec3f32(min.x, min.y, min.z),
        vec3f32(min.x, min.y, max.z),
        vec3f32(min.x, max.y, min.z),
        vec3f32(min.x, max.y, max.z),
        vec3f32(max.x, min.y, min.z),
        vec3f32(max.x, min.y, max.z),
        vec3f32(max.x, max.y, min.z),
        vec3f32(max.x, max.y, max.z)};
    }

    [[nodiscard]]
    auto center() const -> vec3f32
    {
      return (min + max) / 2.0f;
    }

    [[nodiscard]]
    auto
    operator==(const AABB& rhs) const -> bool
    {
      return all(min == rhs.min) && all(max == rhs.max);
    }
  };

  SOUL_ALWAYS_INLINE auto transform(AABB aabb, const mat4f32& mat) -> AABB
  {
    vec4f32 corner_a = math::mul(mat, vec4f32(aabb.min, 1));
    vec4f32 corner_b = math::mul(mat, vec4f32(aabb.max, 1));

    return {
      math::min(corner_a.xyz(), corner_b.xyz()),
      math::max(corner_a.xyz(), corner_b.xyz()),
    };
  }

  SOUL_ALWAYS_INLINE auto combine(AABB aabb, vec3f32 point) -> AABB
  {
    return {min(aabb.min, point), max(aabb.max, point)};
  }

  SOUL_ALWAYS_INLINE auto combine(AABB x, AABB y) -> AABB
  {
    return {min(x.min, y.min), max(x.max, y.max)};
  }
} // namespace soul::math
