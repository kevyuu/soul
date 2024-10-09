#pragma once

#include "core/boolean.h"
#include "core/integer.h"

#include <bit>
#include <limits>

namespace soul
{
  struct Float16
  {

    static constexpr u32 SIGN_BIT_COUNT     = 1;
    static constexpr u32 EXPONENT_BIT_COUNT = 5;
    static constexpr u32 MANTISSA_BIT_COUNT = 10;

    static constexpr u32 SIGN_SHIFT     = EXPONENT_BIT_COUNT + MANTISSA_BIT_COUNT;
    static constexpr u32 EXPONENT_SHIFT = MANTISSA_BIT_COUNT;
    static constexpr u32 MANTISSA_SHIFT = 0;

    static constexpr u32 UNSHIFTED_SIGN_MASK     = ((1u << SIGN_BIT_COUNT) - 1u);
    static constexpr u32 UNSHIFTED_EXPONENT_MASK = ((1u << EXPONENT_BIT_COUNT) - 1u);
    static constexpr u32 UNSHIFTED_MANTISSA_MASK = ((1u << MANTISSA_BIT_COUNT) - 1u);

    static constexpr u32 SIGN_MASK     = UNSHIFTED_SIGN_MASK << SIGN_SHIFT;
    static constexpr u32 EXPONENT_MASK = UNSHIFTED_EXPONENT_MASK << EXPONENT_SHIFT;
    static constexpr u32 MANTISSA_MASK = UNSHIFTED_MANTISSA_MASK << MANTISSA_SHIFT;

    using bit_type = u16;
    static_assert(sizeof(bit_type) == 2);

    Float16() = default;

    Float16(u32 sign, u32 exponent, u32 fraction)
        : bits_(
            ((sign & UNSHIFTED_SIGN_MASK) << SIGN_SHIFT) |
            ((exponent & UNSHIFTED_EXPONENT_MASK) << EXPONENT_SHIFT) |
            ((fraction & UNSHIFTED_MANTISSA_MASK) << MANTISSA_SHIFT))
    {
    }

    static auto Float32ToFloat16Bit(float value) -> bit_type;
    static auto Float16BitToFloat32(bit_type) -> float;

    explicit Float16(float value) : bits_(Float32ToFloat16Bit(value)) {}

    template <typename T>
    explicit Float16(T value) : bits_(Float32ToFloat16Bit(static_cast<float>(value)))
    {
    }

    explicit operator float() const
    {
      return Float16BitToFloat32(bits_);
    }

    auto operator==(const Float16 other) const -> bool
    {
      return bits_ == other.bits_;
    }

    auto operator!=(const Float16 other) const -> bool
    {
      return bits_ != other.bits_;
    }

    auto operator<(const Float16 other) const -> bool
    {
      return static_cast<float>(*this) < static_cast<float>(other);
    }

    auto operator<=(const Float16 other) const -> bool
    {
      return static_cast<float>(*this) <= static_cast<float>(other);
    }

    auto operator>(const Float16 other) const -> bool
    {
      return static_cast<float>(*this) > static_cast<float>(other);
    }

    auto operator>=(const Float16 other) const -> bool
    {
      return static_cast<float>(*this) >= static_cast<float>(other);
    }

    auto operator+() const -> Float16
    {
      return *this;
    }

    auto operator-() const -> Float16
    {
      return std::bit_cast<Float16, bit_type>(bits_ ^ 0x8000);
    }

    auto operator+(const Float16 other) const -> Float16
    {
      return Float16(static_cast<float>(*this) + static_cast<float>(other));
    }

    auto operator-(const Float16 other) const -> Float16
    {
      return Float16(static_cast<float>(*this) - static_cast<float>(other));
    }

    auto operator*(const Float16 other) const -> Float16
    {
      return Float16(static_cast<float>(*this) * static_cast<float>(other));
    }

    auto operator/(const Float16 other) const -> Float16
    {
      return Float16(static_cast<float>(*this) / static_cast<float>(other));
    }

    auto operator+=(const Float16 other) -> Float16
    {
      return *this = *this + other;
    }

    auto operator-=(const Float16 other) -> Float16
    {
      return *this = *this - other;
    }

    auto operator*=(const Float16 other) -> Float16
    {
      return *this = *this * other;
    }

    auto operator/=(const Float16 other) -> Float16
    {
      return *this = *this / other;
    }

    [[nodiscard]]
    constexpr auto is_finite() const noexcept -> bool
    {
      return exponent() < 31;
    }

    [[nodiscard]]
    constexpr auto is_inf() const noexcept -> bool
    {
      return exponent() == 31 && mantissa() == 0;
    }

    [[nodiscard]]
    constexpr auto is_nan() const noexcept -> bool
    {
      return exponent() == 31 && mantissa() != 0;
    }

    [[nodiscard]]
    constexpr auto is_normalized() const noexcept -> bool
    {
      return exponent() > 0 && exponent() < 31;
    }

    [[nodiscard]]
    constexpr auto is_denormalized() const noexcept -> bool
    {
      return exponent() == 0 && mantissa() != 0;
    }

  private:
    [[nodiscard]]
    constexpr auto mantissa() const noexcept -> bit_type
    {
      return bits_ & 0x3ff;
    }

    [[nodiscard]]
    constexpr auto exponent() const noexcept -> bit_type
    {
      return (bits_ >> 10) & 0x001f;
    }

    bit_type bits_;
  };

  inline namespace builtin
  {
    using f16 = Float16;
    static_assert(sizeof(f16) == 2);

    using f32 = float;
    static_assert(sizeof(f32) == 4);

    using f64 = double;
    static_assert(sizeof(f64) == 8);
  } // namespace builtin

  inline namespace literal
  {
    inline auto operator""_f16(long double value) -> f16
    {
      return f16(static_cast<float>(value));
    }

    inline auto operator""_f32(long double value) -> f32
    {
      return static_cast<f32>(value);
    }

    inline auto operator""_f64(long double value) -> f64
    {
      return static_cast<f64>(value);
    }
  } // namespace literal

} // namespace soul

namespace std
{

  template <>
  class numeric_limits<soul::f16>
  {

  public:
    static constexpr bool is_specialized = true;

    static constexpr auto min() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x0200);
    }

    static constexpr auto max() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x7bff);
    }

    static constexpr auto lowest() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0xfbff);
    }

    static constexpr int digits      = 11;
    static constexpr int digits10    = 3;
    static constexpr bool is_signed  = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact   = false;
    static constexpr int radix       = 2;

    static constexpr auto epsilon() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x1200);
    }

    static constexpr auto round_error() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x3c00);
    }

    static constexpr int min_exponent       = -13;
    static constexpr int min_exponent10     = -4;
    static constexpr int max_exponent       = 16;
    static constexpr int max_exponent10     = 4;
    static constexpr bool has_infinity      = true;
    static constexpr bool has_quiet_NaN     = true;
    static constexpr bool has_signaling_NaN = true;
    static constexpr bool has_denorm_loss   = false;

    static constexpr auto infinity() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x7c00);
    }

    static constexpr auto quiet_NaN() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x7fff);
    }

    static constexpr auto signaling_NaN() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0x7dff);
    }

    static constexpr auto denorm_min() noexcept -> soul::f16
    {
      return std::bit_cast<soul::f16, soul::u16>(0);
    }

    static constexpr bool is_iec559                = false;
    static constexpr bool is_bounded               = false;
    static constexpr bool is_modulo                = false;
    static constexpr bool traps                    = false;
    static constexpr bool tinyness_before          = false;
    static constexpr float_round_style round_style = round_to_nearest;
  };
} // namespace std
