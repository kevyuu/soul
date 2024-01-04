#pragma once

#include "core/floating_point.h"

namespace soul::math
{
  namespace f64const
  {
    constexpr f64 E = 2.71828182845904523536028747135266250;
    constexpr f64 LOG2_E = 1.44269504088896340735992468100189214;
    constexpr f64 LOG10_E = 0.434294481903251827651128918916605082;
    constexpr f64 LN2 = 0.693147180559945309417232121458176568;
    constexpr f64 LN10 = 2.30258509299404568401799145468436421;
    constexpr f64 PI = 3.14159265358979323846264338327950288;
    constexpr f64 PI_2 = 1.57079632679489661923132169163975144;
    constexpr f64 PI_4 = 0.785398163397448309615660845819875721;
    constexpr f64 ONE_OVER_PI = 0.318309886183790671537767526745028724;
    constexpr f64 TWO_OVER_PI = 0.636619772367581343075535053490057448;
    constexpr f64 TWO_OVER_SQRTPI = 1.12837916709551257389615890312154517;
    constexpr f64 SQRT2 = 1.41421356237309504880168872420969808;
    constexpr f64 SQRT1_2 = 0.707106781186547524400844362104849039;
    constexpr f64 TAU = 2.0 * f64const::PI;
    constexpr f64 DEG_TO_RAD = f64const::PI / 180.0;
    constexpr f64 RAD_TO_DEG = 180.0 / f64const::PI;
  } // namespace f64const

  namespace f32const
  {
    constexpr f32 E = (f32)f64const::E;
    constexpr f32 LOG2_E = (f32)f64const::LOG2_E;
    constexpr f32 LOG10_E = (f32)f64const::LOG10_E;
    constexpr f32 LN2 = (f32)f64const::LN2;
    constexpr f32 LN10 = (f32)f64const::LN10;
    constexpr f32 PI = (f32)f64const::PI;
    constexpr f32 PI_2 = (f32)f64const::PI_2;
    constexpr f32 PI_4 = (f32)f64const::PI_4;
    constexpr f32 ONE_OVER_PI = (f32)f64const::ONE_OVER_PI;
    constexpr f32 TWO_OVER_PI = (f32)f64const::TWO_OVER_PI;
    constexpr f32 TWO_OVER_SQRTPI = (f32)f64const::TWO_OVER_SQRTPI;
    constexpr f32 SQRT2 = (f32)f64const::SQRT2;
    constexpr f32 SQRT1_2 = (f32)f64const::SQRT1_2;
    constexpr f32 TAU = (f32)f64const::TAU;
    constexpr f32 DEG_TO_RAD = (f32)f64const::DEG_TO_RAD;
    constexpr f32 RAD_TO_DEG = (f32)f64const::RAD_TO_DEG;
  } // namespace f32const

} // namespace soul::math
