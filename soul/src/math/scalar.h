#pragma once

#include "core/boolean.h"
#include "core/floating_point.h"
#include "core/integer.h"
#include "core/type_traits.h"

#include <cmath>

namespace soul::math
{

  // ----------------------------------------------------------------------------
  // Basic functions
  // ----------------------------------------------------------------------------

  template <ts_arithmetic T>
  [[nodiscard]]
  constexpr auto min(T x, T y) -> T
  {
    return x < y ? x : y;
  }

  template <ts_arithmetic T>
  [[nodiscard]]
  constexpr auto max(T x, T y) -> T
  {
    return x > y ? x : y;
  }

  template <ts_arithmetic T>
  [[nodiscard]]
  constexpr auto clamp(T x, T min_, T max_) -> T
  {
    return max(min_, min(max_, x));
  }

  template <ts_signed T>
  [[nodiscard]]
  constexpr auto abs(T x) -> T
  {
    return std::abs(x);
  }

  template <ts_signed T>
  [[nodiscard]]
  constexpr auto sign(T x) -> T
  {
    return x < T(0) ? T(-1) : (x > T(0) ? T(1) : T(0));
  }

  // clang-format off

  // ----------------------------------------------------------------------------
  // Floating point checks
  // ----------------------------------------------------------------------------

  template<typename T> [[nodiscard]] auto isfinite(T x) -> bool;
  template<> [[nodiscard]] inline auto isfinite(f32 x) -> bool { return std::isfinite(x); }
  template<> [[nodiscard]] inline auto isfinite(f64 x) -> bool { return std::isfinite(x); }
  template<> [[nodiscard]] inline auto isfinite(f16 x) -> bool { return x.is_finite(); }

  template<typename T> [[nodiscard]] auto isinf(T x) -> bool;
  template<> [[nodiscard]] inline auto isinf(f32 x) -> bool { return std::isinf(x); }
  template<> [[nodiscard]] inline auto isinf(f64 x) -> bool { return std::isinf(x); }
  template<> [[nodiscard]] inline auto isinf(f16 x) -> bool { return x.is_inf(); }

  template<typename T> [[nodiscard]] auto isnan(T x) -> bool;
  template<> [[nodiscard]] inline auto isnan(f32 x) -> bool { return std::isnan(x); }
  template<> [[nodiscard]] inline auto isnan(f64 x) -> bool { return std::isnan(x); }
  template<> [[nodiscard]] inline auto isnan(f16 x) -> bool { return x.is_nan(); }

  // ----------------------------------------------------------------------------
  // Rounding
  // ----------------------------------------------------------------------------

  template<typename T> [[nodiscard]] auto floor(T x) -> T;
  template<> [[nodiscard]] inline auto floor(f32 x) -> f32 { return std::floor(x); }
  template<> [[nodiscard]] inline auto floor(f64 x) -> f64 { return std::floor(x); }

  template<typename T> [[nodiscard]] auto ceil(T x) -> T;
  template<> [[nodiscard]] inline auto ceil(f32 x) -> f32 { return std::ceil(x); }
  template<> [[nodiscard]] inline auto ceil(f64 x) -> f64 { return std::ceil(x); }

  template<typename T> [[nodiscard]] auto trunc(T x) -> T;
  template<> [[nodiscard]] inline auto trunc(f32 x) -> f32 { return std::trunc(x); }
  template<> [[nodiscard]] inline auto trunc(f64 x) -> f64 { return std::trunc(x); }

  template<typename T> [[nodiscard]] auto round(T x) -> T;
  template<> [[nodiscard]] inline auto round(f32 x) -> f32 { return std::round(x); }
  template<> [[nodiscard]] inline auto round(f64 x) -> f64 { return std::round(x); }

  // ----------------------------------------------------------------------------
  // Exponential
  // ----------------------------------------------------------------------------

  template<typename T> [[nodiscard]] auto pow(T x, T y) -> T;
  template<> [[nodiscard]] inline auto pow(f32 x, f32 y) -> f32 { return std::pow(x, y); }
  template<> [[nodiscard]] inline auto pow(f64 x, f64 y) -> f64 { return std::pow(x, y); }

  template<typename T> [[nodiscard]] auto sqrt(T x) -> T;
  template<> [[nodiscard]] inline auto sqrt(f32 x) -> f32 { return std::sqrt(x); }
  template<> [[nodiscard]] inline auto sqrt(f64 x) -> f64 { return std::sqrt(x); }

  template<typename T> [[nodiscard]] auto rsqrt(T x) -> T;
  template<> [[nodiscard]] inline auto rsqrt(f32 x) -> f32 { return 1.0_f32 / std::sqrt(x); }
  template<> [[nodiscard]] inline auto rsqrt(f64 x) -> f64 { return 1.0_f64 / std::sqrt(x); }

  template<typename T> [[nodiscard]] auto exp(T x) -> T;
  template<> [[nodiscard]] inline auto exp(f32 x) -> f32 { return std::exp(x); }
  template<> [[nodiscard]] inline auto exp(f64 x) -> f64 { return std::exp(x); }
  template<> [[nodiscard]] inline auto exp(f16 x) -> f16 { return f16(std::exp(f32(x))); }

  template<typename T> [[nodiscard]] auto exp2(T x) -> T;
  template<> [[nodiscard]] inline auto exp2(f32 x) -> f32 { return std::exp2(x); }
  template<> [[nodiscard]] inline auto exp2(f64 x) -> f64 { return std::exp2(x); }
  template<> [[nodiscard]] inline auto exp2(f16 x) -> f16 { return f16(std::exp2(f32(x))); }

  template<typename T> [[nodiscard]] auto log(T x) -> T;
  template<> [[nodiscard]] inline auto log(f32 x) -> f32 { return std::log(x); }
  template<> [[nodiscard]] inline auto log(f64 x) -> f64 { return std::log(x); }
  template<> [[nodiscard]] inline auto log(f16 x) -> f16 { return f16(std::log(f32(x))); }

  template<typename T> [[nodiscard]] auto log2(T x) -> T;
  template<> [[nodiscard]] inline auto log2(f32 x) -> f32 { return std::log2(x); }
  template<> [[nodiscard]] inline auto log2(f64 x) -> f64 { return std::log2(x); }

  template<typename T> [[nodiscard]] auto log10(T x) -> T;
  template<> [[nodiscard]] inline auto log10(f32 x) -> f32 { return std::log10(x); }
  template<> [[nodiscard]] inline auto log10(f64 x) -> f64 { return std::log10(x); }

  // ----------------------------------------------------------------------------
  // Trigonometry
  // ----------------------------------------------------------------------------

  template<typename T> [[nodiscard]] auto radians(T x) -> T;
  template<> [[nodiscard]] inline auto radians(f32 x) -> f32 { return x * 0.01745329251994329576923690768489_f32; }
  template<> [[nodiscard]] inline auto radians(f64 x) -> f64 { return x * 0.01745329251994329576923690768489_f64; }

  template<typename T> [[nodiscard]] auto degrees(T x) -> T;
  template<> [[nodiscard]] inline auto degrees(f32 x) -> f32 { return x * 57.295779513082320876798154814105_f32; }
  template<> [[nodiscard]] inline auto degrees(f64 x) -> f64 { return x * 57.295779513082320876798154814105_f64; }

  template<typename T> [[nodiscard]] auto sin(T x) -> T;
  template<> [[nodiscard]] inline auto sin(f32 x) -> f32 { return std::sin(x); }
  template<> [[nodiscard]] inline auto sin(f64 x) -> f64 { return std::sin(x); }

  template<typename T> [[nodiscard]] auto cos(T x) -> T;
  template<> [[nodiscard]] inline auto cos(f32 x) -> f32 { return std::cos(x); }
  template<> [[nodiscard]] inline auto cos(f64 x) -> f64 { return std::cos(x); }

  template<typename T> [[nodiscard]] auto tan(T x) -> T;
  template<> [[nodiscard]] inline auto tan(f32 x) -> f32 { return std::tan(x); }
  template<> [[nodiscard]] inline auto tan(f64 x) -> f64 { return std::tan(x); }

  template<typename T> [[nodiscard]] auto asin(T x) -> T;
  template<> [[nodiscard]] inline auto asin(f32 x) -> f32 { return std::asin(x); }
  template<> [[nodiscard]] inline auto asin(f64 x) -> f64 { return std::asin(x); }

  template<typename T> [[nodiscard]] auto acos(T x) -> T;
  template<> [[nodiscard]] inline auto acos(f32 x) -> f32 { return std::acos(x); }
  template<> [[nodiscard]] inline auto acos(f64 x) -> f64 { return std::acos(x); }

  template<typename T> [[nodiscard]] auto atan(T x) -> T;
  template<> [[nodiscard]] inline auto atan(f32 x) -> f32 { return std::atan(x); }
  template<> [[nodiscard]] inline auto atan(f64 x) -> f64 { return std::atan(x); }

  template<typename T> [[nodiscard]] auto atan2(T y, T x) -> T;
  template<> [[nodiscard]] inline auto atan2(f32 y, f32 x) -> f32 { return std::atan2(y, x); }
  template<> [[nodiscard]] inline auto atan2(f64 y, f64 x) -> f64 { return std::atan2(y, x); }

  template<typename T> [[nodiscard]] auto sinh(T x) -> T;
  template<> [[nodiscard]] inline auto sinh(f32 x) -> f32 { return std::sinh(x); }
  template<> [[nodiscard]] inline auto sinh(f64 x) -> f64 { return std::sinh(x); }

  template<typename T> [[nodiscard]] auto cosh(T x) -> T;
  template<> [[nodiscard]] inline auto cosh(f32 x) -> f32 { return std::cosh(x); }
  template<> [[nodiscard]] inline auto cosh(f64 x) -> f64 { return std::cosh(x); }

  template<typename T> [[nodiscard]] auto tanh(T x) -> T;
  template<> [[nodiscard]] inline auto tanh(f32 x) -> f32 { return std::tanh(x); }
  template<> [[nodiscard]] inline auto tanh(f64 x) -> f64 { return std::tanh(x); }

  // ----------------------------------------------------------------------------
  // Misc
  // ----------------------------------------------------------------------------

  template<typename T> [[nodiscard]] auto fmod(T x, T y) -> T;
  template<> [[nodiscard]] inline auto fmod(f32 x, f32 y) -> f32 { return std::fmod(x, y); }
  template<> [[nodiscard]] inline auto fmod(f64 x, f64 y) -> f64 { return std::fmod(x, y); }

  template<typename T> [[nodiscard]] auto frac(T x) -> T;
  template<> [[nodiscard]] inline auto frac(f32 x) -> f32 { return x - floor(x); }
  template<> [[nodiscard]] inline auto frac(f64 x) -> f64 { return x - floor(x); }

  template<typename T> [[nodiscard]] auto lerp(T x, T y, T s) -> T;
  template<> [[nodiscard]] inline auto lerp(f32 x, f32 y, f32 s) -> f32 { return (1.f - s) * x + s * y; }
  template<> [[nodiscard]] inline auto lerp(f64 x, f64 y, f64 s) -> f64 { return (1.0 - s) * x + s * y; }

  template<typename T> [[nodiscard]] auto rcp(T x) -> T;
  template<> [[nodiscard]] inline auto rcp(f32 x) -> f32 { return 1.0_f32 / x; }
  template<> [[nodiscard]] inline auto rcp(f64 x) -> f64 { return 1.0_f64 / x; }

  template<typename T> [[nodiscard]] auto saturate(T x) -> T;
  template<> [[nodiscard]] inline auto saturate(f32 x) -> f32 { return max(0.0_f32, min(1.f, x)); }
  template<> [[nodiscard]] inline auto saturate(f64 x) -> f64 { return max(0.0_f64, min(1.0, x)); }

  template<typename T> [[nodiscard]] auto step(T x, T y) -> T;
  template<> [[nodiscard]] inline auto step(f32 x, f32 y) -> f32 { return x >= y ? 1.0_f32 : 0._f32; }
  template<> [[nodiscard]] inline auto step(f64 x, f64 y) -> f64 { return x >= y ? 1.0_f64 : 0.0_f64; }

  // clang-format on

  template <ts_floating_point T>
  [[nodiscard]]
  auto smoothstep(T min_val, T max_val, T x) -> T
  {
    x = saturate((x - min_val) / (max_val - min_val));
    return x * x * (T(3) - T(2) * x);
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
} // namespace soul::math
