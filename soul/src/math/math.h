// ReSharper disable CppInconsistentNaming
#pragma once

#include "core/compiler.h"
#include "core/type.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace soul::math
{

  namespace fconst
  {
    static constexpr auto PI = 3.14f;
  }

  template <std::integral T>
  auto fdiv(T a, T b) -> float
  {
    return static_cast<float>(a) / static_cast<float>(b);
  }

  template <usize D1, usize D2, usize D3, typename T>
  SOUL_ALWAYS_INLINE auto mul(const matrix<D1, D2, T>& a, const matrix<D2, D3, T>& b) -> mat4f
  {
    return mat4f(a.mat * b.mat);
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto translate(const matrix4x4<T>& m, const vec3<T>& v) -> matrix4x4<T>
  {
    return matrix4x4<T>(glm::translate(m.mat, v));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto rotate(const matrix4x4<T>& m, T angle, const vec3<T>& axis)
    -> matrix4x4<T>
  {
    return matrix4x4<T>(glm::rotate(m.mat, angle, axis));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto scale(const matrix4x4<T>& m, const vec3<T>& scale) -> matrix4x4<T>
  {
    return matrix4x4<T>(glm::scale(m.mat, scale));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto look_at(const vec3<T>& eye, const vec3<T>& center, const vec3<T>& up)
    -> matrix4x4<T>
  {
    return mat4f(glm::lookAt(eye, center, up));
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto perspective(T fovy, T aspect, T z_near, T z_far) -> matrix4x4<T>
  {
    return mat4f(glm::perspective(fovy, aspect, z_near, z_far));
  }

  template <usize Row, usize Column, typename T>
  SOUL_ALWAYS_INLINE auto inverse(const matrix<Row, Column, T>& mat) -> matrix<Row, Column, T>
  {
    return mat4f(glm::inverse(mat.mat));
  }

  template <usize Row, usize Column, typename T>
  SOUL_ALWAYS_INLINE auto transpose(const matrix<Row, Column, T>& mat) -> matrix<Row, Column, T>
  {
    return mat4f(glm::transpose(mat.mat));
  }

  template <typename T>
  auto radians(T degrees) -> T
  {
    return glm::radians(degrees);
  }

  template <usize Dim, typename T>
  SOUL_ALWAYS_INLINE auto normalize(const vec<Dim, T>& x) -> vec<Dim, T>
  {
    return glm::normalize(x);
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto cross(const vec<3, T>& x, const vec<3, T>& y) -> vec<3, T>
  {
    return glm::cross(x, y);
  }

  template <usize Dim, typename T>
  SOUL_ALWAYS_INLINE auto dot(const vec<Dim, T>& x, const vec<Dim, T>& y) -> T
  {
    return glm::dot(x, y);
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto min(T x, T y) -> T
  {
    return glm::min(x, y);
  }

  template <typename T>
  SOUL_ALWAYS_INLINE auto max(T x, T y) -> T
  {
    return glm::max(x, y);
  }

  SOUL_ALWAYS_INLINE auto combine(const AABB aabb, const vec3f point) -> AABB
  {
    return {min(aabb.min, point), max(aabb.max, point)};
  }

  SOUL_ALWAYS_INLINE auto combine(const AABB x, const AABB y) -> AABB
  {
    return {min(x.min, y.min), max(x.max, y.max)};
  }

} // namespace soul::math
