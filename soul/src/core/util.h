#pragma once

#include <optional>
#include <random>

#include "core/compiler.h"
#include "core/cstring.h"
#include "core/type.h"
#include "core/type_traits.h"

namespace soul::util
{

  template <std::unsigned_integral T>
  constexpr auto get_first_one_bit_pos(T x) noexcept -> std::optional<uint32>
  {
    if (x == 0) {
      return std::nullopt;
    }
    constexpr uint32 bit_count = sizeof(T) * 8;
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

      return (n - (static_cast<uint32>(x) & 1u));
    }
    return std::nullopt;
  }

  template <std::unsigned_integral T>
  constexpr auto get_last_one_bit_pos(T x) noexcept -> std::optional<uint32>
  {
    constexpr uint32 bit_count = sizeof(T) * 8;
    static_assert(bit_count <= 64);
    if (x) {
      uint32 n = 0;
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
  constexpr auto get_one_bit_count(T x) noexcept -> soul_size
  {
    if (std::is_constant_evaluated()) {
      soul_size count = 0;
      while (x) {
        auto bit_pos = *get_first_one_bit_pos(x);
        count++;
        x &= ~(static_cast<T>(1) << bit_pos);
      }
      return count;
    }
    // ReSharper disable once CppUnreachableCode
    static_assert(sizeof(T) <= 8);
    if constexpr (std::same_as<T, uint8> || std::same_as<T, uint16>) {
      return pop_count_16(x);
    }
    if constexpr (std::same_as<T, uint32>) {
      return pop_count_32(x);
    }
    return pop_count_64(x);
  }

  template <std::unsigned_integral Integral, typename Func>
    requires is_lambda_v<Func, void(uint32)>
  auto for_each_one_bit_pos(Integral value, const Func& func) -> void
  {
    while (value) {
      auto bit_pos = *get_first_one_bit_pos(value);
      func(bit_pos);
      value &= ~(static_cast<Integral>(1) << bit_pos);
    }
  }

  template <typename F>
    requires is_lambda_v<F, void()>
  struct ScopeExit {
    explicit ScopeExit(F f) : f(f) {}

    ScopeExit(const ScopeExit&) = delete;
    auto operator=(const ScopeExit&) -> ScopeExit& = delete;
    ScopeExit(ScopeExit&&) = delete;
    auto operator=(ScopeExit&&) -> ScopeExit& = delete;
    ~ScopeExit() { f(); }
    F f;
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

    CString string(fsize, allocator);
    fread(string.data(), 1, fsize, file);

    return string;
  }
  // NOLINTEND(cert-err33-c)

  constexpr auto hash_fnv1_bytes(
    const uint8* data, const soul_size size, const uint64 initial = 0xcbf29ce484222325ull) -> uint64
  {
    auto hash = initial;
    for (uint32 i = 0; i < size; i++) {
      hash = (hash * 0x100000001b3ull) ^ data[i];
    }
    return hash;
  }

  template <typename T>
  constexpr auto hash_fnv1(const T* data, const uint64 initial = 0xcbf29ce484222325ull) -> uint64
  {
    return hash_fnv1_bytes(reinterpret_cast<const uint8*>(data), sizeof(data), initial);
  }

  template <arithmetic T>
  auto get_random_number(T min, T max) -> T
  {
    thread_local std::random_device rd;
    thread_local std::mt19937 mt(rd());
    if constexpr (std::is_floating_point_v<T>) {
      std::uniform_real_distribution<T> distribution(min, max);
      return distribution(mt);
    } else {
      std::uniform_int_distribution<T> distribution(min, max);
      return distribution(mt);
    }
  }

  [[nodiscard]] inline auto get_random_color() -> vec3f
  {
    return {
      get_random_number<float>(0.0f, 1.0f),
      get_random_number<float>(0.0f, 1.0f),
      get_random_number<float>(0.0f, 1.0f)};
  }

  template <std::integral Integral>
  constexpr auto align_up(Integral x, soul_size a) noexcept -> Integral
  {
    return Integral((x + (Integral(a) - 1)) & ~Integral(a - 1));
  }
}; // namespace soul::util
