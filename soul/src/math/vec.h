#pragma once

#include "core/scalar.h"
#include "core/type_traits.h"
#include "core/vec.h"

#include "math/scalar.h"

namespace soul::math
{

  /// min
  template <ts_arithmetic T, u8 N>
  [[nodiscard]]
  constexpr auto min(const Vec<T, N>& x, const Vec<T, N>& y) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{min(x.x, y.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{min(x.x, y.x), min(x.y, y.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{min(x.x, y.x), min(x.y, y.y), min(x.z, y.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{min(x.x, y.x), min(x.y, y.y), min(x.z, y.z), min(x.w, y.w)};
    }
  }

  /// max
  template <ts_arithmetic T, u8 N>
  [[nodiscard]]
  constexpr auto max(const Vec<T, N>& x, const Vec<T, N>& y) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{max(x.x, y.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{max(x.x, y.x), max(x.y, y.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{max(x.x, y.x), max(x.y, y.y), max(x.z, y.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{max(x.x, y.x), max(x.y, y.y), max(x.z, y.z), max(x.w, y.w)};
    }
  }

  /// clamp
  template <ts_arithmetic T, u8 N>
  [[nodiscard]]
  constexpr auto clamp(const Vec<T, N>& x, const Vec<T, N>& min_vec, const Vec<T, N>& max_vec)
    -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{clamp(x.x, min_vec.x, max_vec.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{clamp(x.x, min_vec.x, max_vec.x), clamp(x.y, min_vec.y, max_vec.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{
        clamp(x.x, min_vec.x, max_vec.x),
        clamp(x.y, min_vec.y, max_vec.y),
        clamp(x.z, min_vec.z, max_vec.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{
        clamp(x.x, min_vec.x, max_vec.x),
        clamp(x.y, min_vec.y, max_vec.y),
        clamp(x.z, min_vec.z, max_vec.z),
        clamp(x.w, min_vec.w, max_vec.w)};
    }
  }

  /// abs
  template <ts_signed T, u8 N>
  [[nodiscard]]
  constexpr auto abs(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{abs(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{abs(x.x), abs(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{abs(x.x), abs(x.y), abs(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{abs(x.x), abs(x.y), abs(x.z), abs(x.w)};
    }
  }

  /// sign
  template <ts_signed T, u8 N>
  [[nodiscard]]
  constexpr auto sign(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{sign(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{sign(x.x), sign(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{sign(x.x), sign(x.y), sign(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{sign(x.x), sign(x.y), sign(x.z), sign(x.w)};
    }
  }

  // ----------------------------------------------------------------------------
  // Floating point checks
  // ----------------------------------------------------------------------------

  /// isfinite
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto isfinite(const Vec<T, N>& x) -> Vec<bool, N>
  {
    if constexpr (N == 1) {
      return Vec<bool, N>{isfinite(x.x)};
    } else if constexpr (N == 2) {
      return Vec<bool, N>{isfinite(x.x), isfinite(x.y)};
    } else if constexpr (N == 3) {
      return Vec<bool, N>{isfinite(x.x), isfinite(x.y), isfinite(x.z)};
    } else if constexpr (N == 4) {
      return Vec<bool, N>{isfinite(x.x), isfinite(x.y), isfinite(x.z), isfinite(x.w)};
    }
  }

  /// isinf
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto isinf(const Vec<T, N>& x) -> Vec<bool, N>
  {
    if constexpr (N == 1) {
      return Vec<bool, N>{isinf(x.x)};
    } else if constexpr (N == 2) {
      return Vec<bool, N>{isinf(x.x), isinf(x.y)};
    } else if constexpr (N == 3) {
      return Vec<bool, N>{isinf(x.x), isinf(x.y), isinf(x.z)};
    } else if constexpr (N == 4) {
      return Vec<bool, N>{isinf(x.x), isinf(x.y), isinf(x.z), isinf(x.w)};
    }
  }

  /// isnan
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto isnan(const Vec<T, N>& x) -> Vec<bool, N>
  {
    if constexpr (N == 1) {
      return Vec<bool, N>{isnan(x.x)};
    } else if constexpr (N == 2) {
      return Vec<bool, N>{isnan(x.x), isnan(x.y)};
    } else if constexpr (N == 3) {
      return Vec<bool, N>{isnan(x.x), isnan(x.y), isnan(x.z)};
    } else if constexpr (N == 4) {
      return Vec<bool, N>{isnan(x.x), isnan(x.y), isnan(x.z), isnan(x.w)};
    }
  }

  // ----------------------------------------------------------------------------
  // Rounding
  // ----------------------------------------------------------------------------

  /// floor
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto floor(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{floor(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{floor(x.x), floor(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{floor(x.x), floor(x.y), floor(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{floor(x.x), floor(x.y), floor(x.z), floor(x.w)};
    }
  }

  /// ceil
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto ceil(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{ceil(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{ceil(x.x), ceil(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{ceil(x.x), ceil(x.y), ceil(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{ceil(x.x), ceil(x.y), ceil(x.z), ceil(x.w)};
    }
  }

  /// trunc
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto trunc(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{trunc(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{trunc(x.x), trunc(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{trunc(x.x), trunc(x.y), trunc(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{trunc(x.x), trunc(x.y), trunc(x.z), trunc(x.w)};
    }
  }

  /// round
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto round(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{round(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{round(x.x), round(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{round(x.x), round(x.y), round(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{round(x.x), round(x.y), round(x.z), round(x.w)};
    }
  }

  // ----------------------------------------------------------------------------
  // Exponential
  // ----------------------------------------------------------------------------

  /// pow
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto pow(const Vec<T, N>& x, const Vec<T, N>& y) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{pow(x.x, y.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{pow(x.x, y.x), pow(x.y, y.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{pow(x.x, y.x), pow(x.y, y.y), pow(x.z, y.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{pow(x.x, y.x), pow(x.y, y.y), pow(x.z, y.z), pow(x.w, y.w)};
    }
  }

  /// sqrt
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto sqrt(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{sqrt(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{sqrt(x.x), sqrt(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{sqrt(x.x), sqrt(x.y), sqrt(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{sqrt(x.x), sqrt(x.y), sqrt(x.z), sqrt(x.w)};
    }
  }

  /// rsqrt
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto rsqrt(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{rsqrt(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{rsqrt(x.x), rsqrt(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{rsqrt(x.x), rsqrt(x.y), rsqrt(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{rsqrt(x.x), rsqrt(x.y), rsqrt(x.z), rsqrt(x.w)};
    }
  }

  /// exp
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto exp(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{exp(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{exp(x.x), exp(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{exp(x.x), exp(x.y), exp(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{exp(x.x), exp(x.y), exp(x.z), exp(x.w)};
    }
  }

  /// exp2
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto exp2(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{exp2(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{exp2(x.x), exp2(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{exp2(x.x), exp2(x.y), exp2(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{exp2(x.x), exp2(x.y), exp2(x.z), exp2(x.w)};
    }
  }

  /// log
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto log(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{log(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{log(x.x), log(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{log(x.x), log(x.y), log(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{log(x.x), log(x.y), log(x.z), log(x.w)};
    }
  }

  /// log2
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto log2(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{log2(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{log2(x.x), log2(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{log2(x.x), log2(x.y), log2(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{log2(x.x), log2(x.y), log2(x.z), log2(x.w)};
    }
  }

  /// log10
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto log10(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{log10(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{log10(x.x), log10(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{log10(x.x), log10(x.y), log10(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{log10(x.x), log10(x.y), log10(x.z), log10(x.w)};
    }
  }

  // ----------------------------------------------------------------------------
  // Trigonometry
  // ----------------------------------------------------------------------------

  /// radians
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto radians(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{radians(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{radians(x.x), radians(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{radians(x.x), radians(x.y), radians(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{radians(x.x), radians(x.y), radians(x.z), radians(x.w)};
    }
  }

  /// degrees
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto degrees(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{degrees(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{degrees(x.x), degrees(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{degrees(x.x), degrees(x.y), degrees(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{degrees(x.x), degrees(x.y), degrees(x.z), degrees(x.w)};
    }
  }

  /// sin
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto sin(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{sin(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{sin(x.x), sin(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{sin(x.x), sin(x.y), sin(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{sin(x.x), sin(x.y), sin(x.z), sin(x.w)};
    }
  }

  /// cos
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto cos(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{cos(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{cos(x.x), cos(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{cos(x.x), cos(x.y), cos(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{cos(x.x), cos(x.y), cos(x.z), cos(x.w)};
    }
  }

  /// tan
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto tan(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{tan(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{tan(x.x), tan(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{tan(x.x), tan(x.y), tan(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{tan(x.x), tan(x.y), tan(x.z), tan(x.w)};
    }
  }

  /// asin
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto asin(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{asin(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{asin(x.x), asin(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{asin(x.x), asin(x.y), asin(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{asin(x.x), asin(x.y), asin(x.z), asin(x.w)};
    }
  }

  /// acos
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto acos(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{acos(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{acos(x.x), acos(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{acos(x.x), acos(x.y), acos(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{acos(x.x), acos(x.y), acos(x.z), acos(x.w)};
    }
  }

  /// atan
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto atan(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{atan(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{atan(x.x), atan(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{atan(x.x), atan(x.y), atan(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{atan(x.x), atan(x.y), atan(x.z), atan(x.w)};
    }
  }

  /// atan2
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto atan2(const Vec<T, N>& y, const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{atan2(y.x, x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{atan2(y.x, x.x), atan2(y.y, x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{atan2(y.x, x.x), atan2(y.y, x.y), atan2(y.z, x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{atan2(y.x, x.x), atan2(y.y, x.y), atan2(y.z, x.z), atan2(y.w, x.w)};
    }
  }

  /// sinh
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto sinh(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{sinh(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{sinh(x.x), sinh(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{sinh(x.x), sinh(x.y), sinh(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{sinh(x.x), sinh(x.y), sinh(x.z), sinh(x.w)};
    }
  }

  /// cosh
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto cosh(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{cosh(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{cosh(x.x), cosh(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{cosh(x.x), cosh(x.y), cosh(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{cosh(x.x), cosh(x.y), cosh(x.z), cosh(x.w)};
    }
  }

  /// tanh
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto tanh(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{tanh(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{tanh(x.x), tanh(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{tanh(x.x), tanh(x.y), tanh(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{tanh(x.x), tanh(x.y), tanh(x.z), tanh(x.w)};
    }
  }

  // ----------------------------------------------------------------------------
  // Misc
  // ----------------------------------------------------------------------------

  /// fmod
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto fmod(const Vec<T, N>& x, const Vec<T, N>& y) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{fmod(x.x, y.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{fmod(x.x, y.x), fmod(x.y, y.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{fmod(x.x, y.x), fmod(x.y, y.y), fmod(x.z, y.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{fmod(x.x, y.x), fmod(x.y, y.y), fmod(x.z, y.z), fmod(x.w, y.w)};
    }
  }

  /// frac
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto frac(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{frac(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{frac(x.x), frac(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{frac(x.x), frac(x.y), frac(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{frac(x.x), frac(x.y), frac(x.z), frac(x.w)};
    }
  }

  /// lerp
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto lerp(const Vec<T, N>& x, const Vec<T, N>& y, const Vec<T, N>& s) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{lerp(x.x, y.x, s.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{lerp(x.x, y.x, s.x), lerp(x.y, y.y, s.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{lerp(x.x, y.x, s.x), lerp(x.y, y.y, s.y), lerp(x.z, y.z, s.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{
        lerp(x.x, y.x, s.x), lerp(x.y, y.y, s.y), lerp(x.z, y.z, s.z), lerp(x.w, y.w, s.w)};
    }
  }

  /// rcp
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto rcp(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{rcp(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{rcp(x.x), rcp(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{rcp(x.x), rcp(x.y), rcp(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{rcp(x.x), rcp(x.y), rcp(x.z), rcp(x.w)};
    }
  }

  /// saturate
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto saturate(const Vec<T, N>& x) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{saturate(x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{saturate(x.x), saturate(x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{saturate(x.x), saturate(x.y), saturate(x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{saturate(x.x), saturate(x.y), saturate(x.z), saturate(x.w)};
    }
  }

  /// smoothstep
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto smoothstep(const Vec<T, N>& min_, const Vec<T, N>& max_, const Vec<T, N>& x)
    -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{smoothstep(min_.x, max_.x, x.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{smoothstep(min_.x, max_.x, x.x), smoothstep(min_.y, max_.y, x.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{
        smoothstep(min_.x, max_.x, x.x),
        smoothstep(min_.y, max_.y, x.y),
        smoothstep(min_.z, max_.z, x.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{
        smoothstep(min_.x, max_.x, x.x),
        smoothstep(min_.y, max_.y, x.y),
        smoothstep(min_.z, max_.z, x.z),
        smoothstep(min_.w, max_.w, x.w)};
    }
  }

  /// step
  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto step(const Vec<T, N>& x, const Vec<T, N>& y) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{step(x.x, y.x)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{step(x.x, y.x), step(x.y, y.y)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{step(x.x, y.x), step(x.y, y.y), step(x.z, y.z)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{step(x.x, y.x), step(x.y, y.y), step(x.z, y.z), step(x.w, y.w)};
    }
  }

  template <ts_floating_point T, u8 N>
  [[nodiscard]]
  constexpr auto lerp(const Vec<T, N>& a, const Vec<T, N>& b, const T& s) -> Vec<T, N>
  {
    if constexpr (N == 1) {
      return Vec<T, N>{lerp(a.x, b.x, s)};
    } else if constexpr (N == 2) {
      return Vec<T, N>{lerp(a.x, b.x, s), lerp(a.y, b.y, s)};
    } else if constexpr (N == 3) {
      return Vec<T, N>{lerp(a.x, b.x, s), lerp(a.y, b.y, s), lerp(a.z, b.z, s)};
    } else if constexpr (N == 4) {
      return Vec<T, N>{lerp(a.x, b.x, s), lerp(a.y, b.y, s), lerp(a.z, b.z, s), lerp(a.w, b.w, s)};
    }
  }

  /// dot
  template <typename T, u8 N>
  [[nodiscard]]
  constexpr auto dot(const Vec<T, N>& lhs, const Vec<T, N>& rhs) -> T
  {
    T result = lhs.x * rhs.x;
    if constexpr (N >= 2) {
      result += lhs.y * rhs.y;
    }
    if constexpr (N >= 3) {
      result += lhs.z * rhs.z;
    }
    if constexpr (N >= 4) {
      result += lhs.w * rhs.w;
    }
    return result;
  }

  /// cross
  template <typename T>
  [[nodiscard]]
  constexpr auto cross(const Vec<T, 3>& lhs, const Vec<T, 3>& rhs) -> Vec<T, 3>
  {
    return Vec<T, 3>(
      lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
  }

  /// length
  template <typename T, u8 N>
  [[nodiscard]]
  constexpr auto length(const Vec<T, N>& v) -> T
  {
    return sqrt(dot(v, v));
  }

  /// normalize
  template <typename T, u8 N>
  [[nodiscard]]
  constexpr auto normalize(const Vec<T, N>& v) -> Vec<T, N>
  {
    return v * rsqrt(dot(v, v));
  }

  /// reflect
  template <typename T, u8 N>
  [[nodiscard]]
  constexpr auto reflect(const Vec<T, N>& v, const Vec<T, N>& n) -> Vec<T, N>
  {
    return v - T(2) * dot(v, n) * n;
  }
} // namespace soul::math
