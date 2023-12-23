#pragma once

#include "core/type_traits.h"
#include "core/vec.h"

namespace soul
{
  template <ts_arithmetic T>
  struct quat {
    union {
      struct {
        T x, y, z, w;
      }; // NOLINT(clang-diagnostic-nested-anon-types)
      vec4<T> xyzw;
      vec3<T> xyz;
      vec2<T> xy;

      struct {
        // NOLINT(clang-diagnostic-nested-anon-types)
        vec3<T> g;
        T real;
      };

      T mem[4];
    };

    constexpr quat() noexcept : x(0), y(0), z(0), w(1) {}

    constexpr quat(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

    constexpr quat(vec3<T> xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    [[nodiscard]]
    constexpr static auto FromData(const T* val) -> quat
    {
      return quat(val[0], val[1], val[2], val[3]);
    };
  };

  using quatf = quat<f32>;
} // namespace soul
