#pragma once

#include <limits>
#include <optional>
#include <random>

#include "core/compiler.h"
#include "core/cstring.h"
#include "core/type.h"
#include "core/type_traits.h"

namespace soul::util
{

  template <std::unsigned_integral T>
  constexpr auto get_first_one_bit_pos(T x) noexcept -> std::optional<u32>
  {
    if (x == 0) {
      return std::nullopt;
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

      return (n - (static_cast<u32>(x) & 1u));
    }
    return std::nullopt;
  }

  template <std::unsigned_integral T>
  constexpr auto get_last_one_bit_pos(T x) noexcept -> std::optional<u32>
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
      return n;
    }
    return std::nullopt;
  }

  template <std::unsigned_integral T>
  constexpr auto get_one_bit_count(T x) noexcept -> usize
  {
    if (std::is_constant_evaluated()) {
      usize count = 0;
      while (x) {
        auto bit_pos = *get_first_one_bit_pos(x);
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
      auto bit_pos = *get_first_one_bit_pos(value);
      func(bit_pos);
      value &= ~(static_cast<Integral>(1) << bit_pos);
    }
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

  // NOLINTBEGIN(cert-err33-c)
  inline auto load_file(const char* filepath, memory::Allocator& allocator) -> CString
  {
    FILE* file = nullptr;
    const auto err = fopen_s(&file, filepath, "rb");
    if (err != 0) {
      SOUL_PANIC("Fail to open file %s", filepath);
    }
    SCOPE_EXIT(fclose(file));

    fseek(file, 0, SEEK_END);
    const auto fsize = ftell(file);
    fseek(file, 0, SEEK_SET); /* same as rewind(f); */

    auto string = CString::with_size(fsize, &allocator);
    fread(string.data(), 1, fsize, file);

    return string;
  }
  // NOLINTEND(cert-err33-c)

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
}; // namespace soul::util
