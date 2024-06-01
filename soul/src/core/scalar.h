#pragma once

#include "boolean.h"
#include "floating_point.h"
#include "integer.h"

namespace soul
{
  // ----------------------------------------------------------------------------
  // Boolean reductions
  // ----------------------------------------------------------------------------

  template <typename T>
  [[nodiscard]]
  constexpr auto any(T x) -> bool
  {
    return x != T(0);
  }

  template <typename T>
  [[nodiscard]]
  constexpr auto all(T x) -> bool
  {
    return x != T(0);
  }

  // ----------------------------------------------------------------------------
  // Trigonometry reductions
  // ----------------------------------------------------------------------------

  template <typename T>
  [[nodiscard]]
  auto radians(T x) noexcept -> T;

  template <>
  [[nodiscard]]
  inline auto radians(f32 x) noexcept -> f32
  {
    return x * 0.01745329251994329576923690768489f;
  }

  template <>
  [[nodiscard]]
  inline auto radians(f64 x) noexcept -> f64
  {
    return x * 0.01745329251994329576923690768489;
  }

  template <typename T>
  [[nodiscard]]
  T degrees(T x) noexcept;

  template <>
  [[nodiscard]]
  inline auto degrees(f32 x) noexcept -> f32
  {
    return x * 57.295779513082320876798154814105f;
  }

  template <>
  [[nodiscard]]
  inline auto degrees(f64 x) noexcept -> f64
  {
    return x * 57.295779513082320876798154814105;
  }

  template <typename T>
  [[nodiscard]]
  auto sin(T x) noexcept -> T
  {
    return std::sin(x);
  }

  template <typename T>
  [[nodiscard]]
  auto cos(T x) noexcept -> T
  {
    return std::cos(x);
  }

  template <typename T>
  [[nodiscard]]
  auto tan(T x) noexcept -> T
  {
    return std::tan(x);
  }

  template <typename T>
  [[nodiscard]]
  auto asin(T x) noexcept -> T
  {
    return std::asin(x);
  }

  template <typename T>
  [[nodiscard]]
  auto acos(T x) noexcept -> T
  {
    return std::acos(x);
  }

  template <typename T>
  [[nodiscard]]
  auto atan(T x) noexcept -> T
  {
    return std::atan(x);
  }

  template <typename T>
  [[nodiscard]]
  auto atan2(T y, T x) noexcept -> T
  {
    return std::atan2(y, x);
  }

  template <typename T>
  [[nodiscard]]
  auto sinh(T x) noexcept -> T
  {
    return std::sinh(x);
  }

  template <typename T>
  [[nodiscard]]
  auto cosh(T x) noexcept -> T
  {
    return std::cosh(x);
  }

  template <typename T>
  [[nodiscard]]
  auto tanh(T x) noexcept -> T
  {
    return std::tanh(x);
  }

} // namespace soul
