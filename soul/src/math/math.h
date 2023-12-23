// ReSharper disable CppInconsistentNaming
#pragma once

#include "core/aabb.h"
#include "core/builtins.h"
#include "core/compiler.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/ext/scalar_constants.hpp>

namespace soul::math
{

  namespace fconst
  {
    static constexpr auto PI = 3.14f;
  }

  template <std::integral T>
  auto fdiv(T a, T b) -> f32
  {
    return static_cast<f32>(a) / static_cast<f32>(b);
  }

  [[nodiscard]]
  inline auto floor_log2(u64 val) -> u64
  {
    u32 level = 0;
    while ((val >>= 1) != 0u) {
      ++level;
    }
    return level;
  }

  template <usize D1, usize D2, usize D3, typename T>
  SOUL_ALWAYS_INLINE auto mul(const matrix<D1, D2, T>& a, const matrix<D2, D3, T>& b) -> mat4f32
  {
    return matrix<D1, D2, T>(a.storage * b.storage);
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto translate(const mat4<T>& m, const vec3<T>& v) -> mat4<T>
  {
    return mat4<T>(glm::translate(m.storage, v.storage));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto rotate(const mat4<T>& m, T angle, const vec3<T>& axis) -> mat4<T>
  {
    return mat4<T>(glm::rotate(m.storage, angle, axis.storage));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto scale(const mat4<T>& m, const vec3<T>& scale) -> mat4<T>
  {
    return mat4<T>(glm::scale(m.storage, scale.storage));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto look_at(const vec3<T>& eye, const vec3<T>& center, const vec3<T>& up)
    -> mat4<T>
  {
    return mat4<T>(glm::lookAt(eye.storage, center.storage, up.storage));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto perspective(T fovy, T aspect, T z_near, T z_far) -> mat4<T>
  {
    T fov_rad = fovy;
    T focal_length = 1.0f / std::tan(fov_rad / 2.0f);

    T x = focal_length / aspect;
    T y = -focal_length;
    T A = z_far / (z_near - z_far);
    T B = z_near * A;

    mat4<T> result;

    result.m(0, 0) = x;
    result.m(0, 1) = 0.0f;
    result.m(0, 2) = 0.0f;
    result.m(0, 3) = 0.0f;

    result.m(1, 0) = 0.0f;
    result.m(1, 1) = y;
    result.m(1, 2) = 0.0f;
    result.m(1, 3) = 0.0f;

    result.m(2, 0) = 0.0f;
    result.m(2, 1) = 0.0f;
    result.m(2, 2) = A;
    result.m(2, 3) = B;

    result.m(3, 0) = 0.0f;
    result.m(3, 1) = 0.0f;
    result.m(3, 2) = T(-1.0);
    result.m(3, 3) = 0.0f;

    return result;
  }

  template <usize Row, usize Column, typename T>
  SOUL_ALWAYS_INLINE auto inverse(const matrix<Row, Column, T>& mat) -> matrix<Row, Column, T>
  {
    return matrix<Row, Column, T>(glm::inverse(mat.storage));
  }

  template <usize Row, usize Column, typename T>
  SOUL_ALWAYS_INLINE auto transpose(const matrix<Row, Column, T>& mat) -> matrix<Row, Column, T>
  {
    return matrix<Row, Column, T>(glm::transpose(mat.storage));
  }

  template <typename T>
  auto radians(T degrees) -> T
  {
    return glm::radians(degrees);
  }

  template <usize Dim, typename T>
  SOUL_ALWAYS_INLINE auto length(const vec<Dim, T>& x) -> T
  {
    return glm::length(x.storage);
  }

  template <usize Dim, typename T>
  SOUL_ALWAYS_INLINE auto normalize(const vec<Dim, T>& x) -> vec<Dim, T>
  {
    return vec<3, T>::FromStorage(glm::normalize(x.storage));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto cross(const vec<3, T>& x, const vec<3, T>& y) -> vec<3, T>
  {
    return vec<3, T>::FromStorage(glm::cross(x.storage, y.storage));
  }

  template <usize Dim, typename T>
  SOUL_ALWAYS_INLINE auto dot(const vec<Dim, T>& x, const vec<Dim, T>& y) -> T
  {
    return glm::dot(x.storage, y.storage);
  }

  template <usize dim, typename T>
  SOUL_ALWAYS_INLINE auto min(vec<dim, T> x, vec<dim, T> y) -> vec<dim, T>
  {
    return vec<dim, T>::FromStorage(glm::min(x.storage, y.storage));
  }

  template <usize dim, typename T>
  SOUL_ALWAYS_INLINE auto max(vec<dim, T> x, vec<dim, T> y) -> vec<dim, T>
  {
    return vec<dim, T>::FromStorage(glm::max(x.storage, y.storage));
  }

  template <usize dimension, typename T>
  constexpr auto less_than(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, bool>
  {
    return vec<dimension, b8>::FromStorage(glm::lessThan(lhs.storage, rhs.storage));
  }

  template <usize dimension, typename T>
  constexpr auto greater_than(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, bool>
  {
    return vec<dimension, b8>::FromStorage(glm::greaterThan(lhs.storage, rhs.storage));
  }

  template <usize dimension, typename T>
  constexpr auto not_equal(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, b8>
  {
    return vec<dimension, b8>::FromStorage(glm::notEqual(lhs.storage, rhs.storage));
  }

  template <usize dimension, typename T>
  constexpr auto equal(const vec<dimension, T>& lhs, const vec<dimension, T>& rhs)
    -> vec<dimension, b8>
  {
    return vec<dimension, b8>::FromStorage(glm::equal(lhs.storage, rhs.storage));
  }

  template <usize dim>
  SOUL_ALWAYS_INLINE auto any(const vec<dim, b8>& bool_vec) -> b8
  {
    return glm::any(bool_vec.storage);
  }

  template <usize dim>
  SOUL_ALWAYS_INLINE auto all(const vec<dim, b8>& bool_vec) -> b8
  {
    return glm::all(bool_vec.storage);
  }

  template <usize dim, typename T>
  SOUL_ALWAYS_INLINE auto abs(const vec<dim, T>& val) -> vec<dim, T>
  {
    return vec<dim, T>::FromStorage(glm::abs(val.storage));
  }

  SOUL_ALWAYS_INLINE auto combine(const AABB aabb, const vec3f32 point) -> AABB
  {
    return {min(aabb.min, point), max(aabb.max, point)};
  }

  SOUL_ALWAYS_INLINE auto combine(const AABB x, const AABB y) -> AABB
  {
    return {min(x.min, y.min), max(x.max, y.max)};
  }

} // namespace soul::math
