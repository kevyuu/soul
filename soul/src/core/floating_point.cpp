#include "core/floating_point.h"

namespace soul
{
  namespace
  {
    auto overflow() -> float
    {
      volatile float f = 1e10;
      for (i32 i = 0; i < 10; ++i) {
        f *= f; // this will overflow before the for loop terminates
      }
      return f;
    }
  } // namespace

  auto Float16::Float32ToFloat16Bit(float value) -> bit_type
  {
    i32 i = std::bit_cast<i32, float>(value);

    //
    // Our floating point number, f, is represented by the bit
    // pattern in integer i.  Disassemble that bit pattern into
    // the sign, s, the exponent, e, and the significand, m.
    // Shift s into the position where it will go in the
    // resulting half number.
    // Adjust e, accounting for the different exponent bias
    // of float and half (127 versus 15).
    //

    i32 s = (i >> 16) & 0x00008000;
    i32 e = ((i >> 23) & 0x000000ff) - (127 - 15);
    i32 m = i & 0x007fffff;

    //
    // Now reassemble s, e and m into a half:
    //

    if (e <= 0) {
      if (e < -10) {
        //
        // E is less than -10.  The absolute value of f is
        // less than half_MIN (f may be a small normalized
        // float, a denormalized float or a zero).
        //
        // We convert f to a half zero.
        //

        return u16(s);
      }

      //
      // E is between -10 and 0.  F is a normalized float,
      // whose magnitude is less than __half_NRM_MIN.
      //
      // We convert f to a denormalized half.
      //

      m = (m | 0x00800000) >> (1 - e);

      //
      // Round to nearest, round "0.5" up.
      //
      // Rounding may cause the significand to overflow and make
      // our number normalized.  Because of the way a half's bits
      // are laid out, we don't have to treat this case separately;
      // the code below will handle it correctly.
      //

      if ((m & 0x00001000) != 0) {
        m += 0x00002000;
      }

      //
      // Assemble the half from s, e (zero) and m.
      //

      return u16(s | (m >> 13));
    } else if (e == 0xff - (127 - 15)) {
      if (m == 0) {
        //
        // F is an infinity; convert f to a half
        // infinity with the same sign as f.
        //

        return u16(s | 0x7c00);
      } else {
        //
        // F is a NAN; we produce a half NAN that preserves
        // the sign bit and the 10 leftmost bits of the
        // significand of f, with one exception: If the 10
        // leftmost bits are all zero, the NAN would turn
        // into an infinity, so we have to set at least one
        // bit in the significand.
        //

        m >>= 13;

        return u16(s | 0x7c00 | m | static_cast<i32>(m == 0));
      }
    } else {
      //
      // E is greater than zero.  F is a normalized float.
      // We try to convert f to a normalized half.
      //

      //
      // Round to nearest, round "0.5" up
      //

      if ((m & 0x00001000) != 0) {
        m += 0x00002000;

        if ((m & 0x00800000) != 0) {
          m = 0;  // overflow in significand,
          e += 1; // adjust exponent
        }
      }

      //
      // Handle exponent overflow
      //

      if (e > 30) {
        overflow(); // Cause a hardware floating point overflow;

        return u16(s | 0x7c00); // Return infinity with same sign as f.
      }

      //
      // Assemble the half from s, e and m.
      //

      return u16(s | (e << 10) | (m >> 13));
    }
  }

  auto Float16::Float16BitToFloat32(u16 value) -> float
  {
    i32 s = (value >> 15) & 0x00000001;
    i32 e = (value >> 10) & 0x0000001f;
    i32 m = value & 0x000003ff;

    if (e == 0) {
      if (m == 0) {
        //
        // Plus or minus zero
        //

        return std::bit_cast<float, i32>(s << 31);
      } else {
        //
        // Denormalized number -- renormalize it
        //

        while (!(m & 0x00000400)) {
          m <<= 1;
          e -= 1;
        }

        e += 1;
        m &= ~0x00000400;
      }
    } else if (e == 31) {
      if (m == 0) {
        //
        // Positive or negative infinity
        //

        return std::bit_cast<float, i32>((s << 31) | 0x7f800000);
      } else {
        //
        // Nan -- preserve sign and significand bits
        //

        return std::bit_cast<float, i32>((s << 31) | 0x7f800000 | (m << 13));
      }
    }

    //
    // Normalized number
    //

    e = e + (127 - 15);
    m = m << 13;

    //
    // Assemble s, e and m.
    //
    return std::bit_cast<float, i32>((s << 31) | (e << 23) | m);
  }
} // namespace soul
