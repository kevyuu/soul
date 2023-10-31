#pragma once

#include <limits>
#include <random>

#include "core/compiler.h"
#include "core/option.h"
#include "core/type.h"
#include "core/type_traits.h"

namespace soul::util
{

  template <std::unsigned_integral T>
  constexpr auto get_first_one_bit_pos(T x) noexcept -> Option<u32>
  {
    if (x == 0) {
      return nilopt;
    }
    constexpr u32 bit_count = sizeof(T) * 8;
    static_assert(bit_count <= 64);
    if (x) {
      uint32_t n = 1;
      if constexpr (bit_count > 32) {
        if ((x & 0xFFFFFFFF) == 0) {
          n += 32;
          x >>= 32;
        }
      }
      if constexpr (bit_count > 16) {
        if ((x & 0xFFFF) == 0) {
          n += 16;
          x >>= 16;
        }
      }
      if constexpr (bit_count > 8) {
        if ((x & 0xFF) == 0) {
          n += 8;
          x >>= 8;
        }
      }
      if ((x & 0x0F) == 0) {
        n += 4;
        x >>= 4;
      }
      if ((x & 0x03) == 0) {
        n += 2;
        x >>= 2;
      }

      return Option<u32>::some(n - (static_cast<u32>(x) & 1u));
    }
    return nilopt;
  }

  template <std::unsigned_integral T>
  constexpr auto get_last_one_bit_pos(T x) noexcept -> Option<u32>
  {
    constexpr u32 bit_count = sizeof(T) * 8;
    static_assert(bit_count <= 64);
    if (x) {
      u32 n = 0;
      if constexpr (bit_count > 32) {
        if (x & 0xFFFFFFFF00000000) {
          n += 32;
          x >>= 32;
        }
      }
      if constexpr (bit_count > 16) {
        if (x & 0xFFFF0000) {
          n += 16;
          x >>= 16;
        }
      }
      if constexpr (bit_count > 8) {
        if (x & 0xFF00) {
          n += 8;
          x >>= 8;
        }
      }
      if (x & 0xFFF0) {
        n += 4;
        x >>= 4;
      }
      if (x & 0xFFFC) {
        n += 2;
        x >>= 2;
      }
      if (x & 0xFFFE) {
        n += 1;
      }
      return Option<u32>::some(n);
    }
    return nilopt;
  }

  template <std::unsigned_integral T>
  constexpr auto get_one_bit_count(T x) noexcept -> usize
  {
    if (std::is_constant_evaluated()) {
      usize count = 0;
      while (x) {
        auto bit_pos = get_first_one_bit_pos(x).unwrap();
        count++;
        x &= ~(static_cast<T>(1) << bit_pos);
      }
      return count;
    }
    // ReSharper disable once CppUnreachableCode
    static_assert(sizeof(T) <= 8);
    if constexpr (std::same_as<T, u8> || std::same_as<T, u16>) {
      return pop_count_16(x);
    }
    if constexpr (std::same_as<T, u32>) {
      return pop_count_32(x);
    }
    return pop_count_64(x);
  }

  template <std::unsigned_integral Integral, ts_fn<void, u32> Fn>
  auto for_each_one_bit_pos(Integral value, const Fn& func) -> void
  {
    while (value) {
      auto bit_pos = get_first_one_bit_pos(value).unwrap();
      func(bit_pos);
      value &= ~(static_cast<Integral>(1) << bit_pos);
    }
  }

  inline auto next_power_of_two(usize i) -> usize
  {
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i |= i >> 32;
    ++i;
    return i;
  }

  template <ts_fn<void> Fn>
  struct ScopeExit {
    explicit ScopeExit(Fn f) : f(f) {}

    ScopeExit(const ScopeExit&) = delete;
    auto operator=(const ScopeExit&) -> ScopeExit& = delete;
    ScopeExit(ScopeExit&&) = delete;
    auto operator=(ScopeExit&&) -> ScopeExit& = delete;
    ~ScopeExit() { f(); }
    Fn f;
  };

  template <typename F>
  auto make_scope_exit(F f) -> ScopeExit<F>
  {
    return ScopeExit<F>(f);
  };

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1##arg2
#define SCOPE_EXIT(code)                                                                           \
  auto STRING_JOIN2(scope_exit_, __LINE__) = soul::util::make_scope_exit([=]() { code; })

  constexpr auto hash_fnv1_bytes(
    const u8* data, const usize size, const u64 initial = 0xcbf29ce484222325ull) -> u64
  {
    auto hash = initial;
    for (u32 i = 0; i < size; i++) {
      hash = (hash * 0x100000001b3ull) ^ data[i];
    }
    return hash;
  }

  template <typename T>
  constexpr auto hash_fnv1(const T* data, const u64 initial = 0xcbf29ce484222325ull) -> u64
  {
    return hash_fnv1_bytes(reinterpret_cast<const u8*>(data), sizeof(data), initial);
  }

  [[nodiscard]]
  inline auto get_random_u32() -> u32
  {
    thread_local u32 x = 123456789;
    thread_local u32 y = 362436069;
    thread_local u32 z = 521288629;

    x ^= x << 16u;
    x ^= x >> 5u;
    x ^= x << 1u;

    u32 t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return z;
  }

  [[nodiscard]]
  inline auto get_random_color() -> vec3f
  {
    auto get_random_float = []() -> f32 {
      return f32(get_random_u32()) / f32(std::numeric_limits<u32>::max());
    };
    return {get_random_float(), get_random_float(), get_random_float()};
  }

  template <std::integral Integral>
  constexpr auto align_up(Integral x, usize a) noexcept -> Integral
  {
    return Integral((x + (Integral(a) - 1)) & ~Integral(a - 1));
  }

  // All leading zero will not be count as digit, '0' will have 1 digit count
  constexpr auto digit_count(usize val, usize base = 10) -> usize
  {
    if (val == 0) {
      return 1;
    }
    usize number_of_digits = 0;

    while (val != 0) {
      number_of_digits++;
      val /= base;
    }
    return number_of_digits;
  }

  constexpr void mul128_nonbuiltin(u64* a, u64* b)
  {
    u64 ha = *a >> 32U;
    u64 hb = *b >> 32U;
    u64 la = static_cast<u32>(*a);
    u64 lb = static_cast<u32>(*b);
    u64 hi{};
    u64 lo{};
    u64 rh = ha * hb;
    u64 rm0 = ha * lb;
    u64 rm1 = hb * la;
    u64 rl = la * lb;
    u64 t = rl + (rm0 << 32U);
    auto c = static_cast<u64>(t < rl);
    lo = t + (rm1 << 32U);
    c += static_cast<u64>(lo < t);
    hi = rh + (rm0 >> 32U) + (rm1 >> 32U) + c;
    *a = lo;
    *b = hi;
  }

  constexpr void mul128(u64* a, u64* b)
  {
    if (std::is_constant_evaluated()) {
      mul128_nonbuiltin(a, b);
    } else {
#if defined(__SIZEOF_INT128__)
      __uint128_t r = *a;
      r *= *b;
      *a = static_cast<u64>(r);
      *b = static_cast<u64>(r >> 64U);
#elif defined(_MSC_VER) && defined(_M_X64)
      *a = _umul128(*a, *b, b);
#else
      mul128_nonbuiltin(a, b);
#endif
    }
  }

  [[nodiscard]]
  constexpr auto unaligned_load32(const byte* p) -> u64
  {
    const byte val[4] = {p[0], p[1], p[2], p[3]};
    return std::bit_cast<u32>(val);
  }

  [[nodiscard]]
  constexpr auto unaligned_load64(const byte* p) -> u64
  {
    const byte val[8] = {p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]};
    return std::bit_cast<u64>(val);
  }

}; // namespace soul::util
