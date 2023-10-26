#pragma once

#include <concepts>
#include "core/array.h"
#include "core/builtins.h"
#include "core/compiler.h"
#include "core/span.h"
#include "core/type_traits.h"
#include "core/util.h"

namespace soul
{

  [[nodiscard]]
  constexpr auto hash_wy_bytes(Span<const byte*, usize> bytes) -> u64
  {
    constexpr auto SECRETS = Array{
      UINT64_C(0xa0761d6478bd642f),
      UINT64_C(0xe7037ed1a0b428db),
      UINT64_C(0x8ebc6af09c88c6e3),
      UINT64_C(0x589965cc75374cc3),
    };

    auto mix = [](u64 a, u64 b) -> u64 {
      util::mul128(&a, &b);
      return a ^ b;
    };

    auto r3 = [](const byte* p, usize k) -> u64 {
      return (static_cast<u64>(p[0]) << 16U) | (static_cast<u64>(p[k >> 1U]) << 8U) | p[k - 1];
    };

    const byte* p = bytes.data().unwrap();
    u64 seed = SECRETS[0];
    u64 a{};
    u64 b{};
    usize len = bytes.size();
    if (SOUL_LIKELY(len <= 16)) {
      if (SOUL_LIKELY(len >= 4)) {
        a = (util::unaligned_load32(p) << 32U) | util::unaligned_load32(p + ((len >> 3U) << 2U));
        b = (util::unaligned_load32(p + len - 4) << 32U) |
            util::unaligned_load32(p + len - 4 - ((len >> 3U) << 2U));
      } else if (SOUL_LIKELY(len > 0)) {
        a = r3(p, len);
        b = 0;
      } else {
        a = 0;
        b = 0;
      }
    } else {
      size_t i = len;
      if (SOUL_UNLIKELY(i > 48)) {
        u64 see1 = seed;
        u64 see2 = seed;
        do {
          seed = mix(util::unaligned_load64(p) ^ SECRETS[1], util::unaligned_load64(p + 8) ^ seed);
          see1 =
            mix(util::unaligned_load64(p + 16) ^ SECRETS[2], util::unaligned_load64(p + 24) ^ see1);
          see2 =
            mix(util::unaligned_load64(p + 32) ^ SECRETS[3], util::unaligned_load64(p + 40) ^ see2);
          p += 48;
          i -= 48;
        } while (SOUL_LIKELY(i > 48));
        seed ^= see1 ^ see2;
      }
      while (SOUL_UNLIKELY(i > 16)) {
        seed = mix(util::unaligned_load64(p) ^ SECRETS[1], util::unaligned_load64(p + 8) ^ seed);
        i -= 16;
        p += 16;
      }
      a = util::unaligned_load64(p + i - 16);
      b = util::unaligned_load64(p + i - 8);
    }

    return mix(SECRETS[1] ^ len, mix(a ^ SECRETS[1], b ^ seed));
  }

  class Hasher;

  template <typename T>
  struct HashTrait;

  template <typename T>
  inline constexpr b8 impl_hash_trait_v = requires(Hasher& h, const T& val) {
    {
      soul::HashTrait<T>::combine(h, val)
    } -> same_as<void>;
  };

  class Hasher
  {
    u64 state_;
    static constexpr auto kMul = 0x9ddfea08eb382d69ULL;

    [[nodiscard]]
    static constexpr auto Mix(u64 a, u64 b) -> u64
    {
      u64 v1 = a + b;
      u64 v2 = kMul;
      util::mul128(&v1, &v2);
      return v1 ^ v2;
    }

    static constexpr auto Read4To8(const byte* p, usize len) -> u64
    {
      u32 low_mem = util::unaligned_load32(p);
      u32 high_mem = util::unaligned_load32(p + len - 4);
      u32 most_significant, least_significant; // NOLINT
      if constexpr (get_endianess() == Endianess::LITTLE) {
        most_significant = high_mem;
        least_significant = low_mem;
      } else {
        most_significant = low_mem;
        least_significant = high_mem;
      }
      return (static_cast<uint64_t>(most_significant) << (len - 4) * 8) | least_significant;
    }

    // Reads 1 to 3 bytes from p. Zero pads to fill uint32_t.
    static constexpr auto Read1To3(const unsigned char* p, size_t len) -> u64
    {
      // The trick used by this implementation is to avoid branches if possible.
      byte mem0 = p[0];
      byte mem1 = p[len / 2];
      byte mem2 = p[len - 1];

      byte significant0, significant1, significant2; // NOLINT
      if constexpr (get_endianess() == Endianess::LITTLE) {
        significant2 = mem2;
        significant1 = mem1;
        significant0 = mem0;
      } else {
        significant2 = mem0;
        significant1 = len == 2 ? mem0 : mem1;
        significant0 = mem2;
      }
      return static_cast<uint32_t>(
        significant0 |                    //
        (significant1 << (len / 2 * 8)) | //
        (significant2 << ((len - 1) * 8)));
    }

  public:
    constexpr explicit Hasher(u64 seed = 0x9E3779B97F4A7C15ULL) : state_(seed) {}

    constexpr void combine_u64(u64 val) { state_ = Mix(state_, val); }

    constexpr void combine_bytes(Span<const byte*> bytes)
    {
      const byte* data = bytes.data();
      const u64 size = bytes.size_in_bytes();
      if (size > 8) {
        state_ = Mix(state_, hash_wy_bytes({data, size}));
      } else if (size >= 4) {
        state_ = Mix(state_, Read4To8(data, size));
      } else if (size > 0) {
        state_ = Mix(state_, Read1To3(data, size));
      }
    }

    template <typename T>
    constexpr void combine_span(Span<const T*> span)
    {
      if constexpr (has_unique_object_representations_v<T>) {
        const T* data = span.data();
        const usize size = span.size_in_bytes();
        combine_bytes({reinterpret_cast<const byte*>(data), size});
      } else {
        combine_u64(span.size());
        for (const T& val : span) {
          state_ = HashTrait<T>::combine(*this, val);
        }
      }
    }

    template <typename T>
      requires(impl_hash_trait_v<T>)
    constexpr void combine(const T& val)
    {
      HashTrait<T>::combine(*this, val);
    }

    template <typename T, typename... Ts>
      requires(impl_hash_trait_v<T> && conjunction_v<impl_hash_trait_v<Ts>...>)
    constexpr void combine(const T& val, const Ts&... args)
    {
      combine(val);
      (combine(args), ...);
    }

    constexpr auto finish() -> u64 { return state_; }
  };

  template <typename T>
    requires(has_unique_object_representations_v<T>)
  struct HashTrait<T> {
    static constexpr void combine(Hasher& hasher, const T& val)
    {
      if constexpr (ts_integral<T>) {
        hasher.combine_u64(static_cast<u64>(val));
      } else {
        hasher.combine_bytes({reinterpret_cast<const byte*>(val), sizeof(val)});
      }
    }
  };

  template <>
  struct HashTrait<f32> {
    static constexpr void combine(Hasher& hasher, f32 val)
    {
      u64 val64 = val == 0 ? 0 : std::bit_cast<u32>(val);
      hasher.combine_u64(val64);
    }
  };

  template <>
  struct HashTrait<f64> {
    static constexpr void combine(Hasher& hasher, f64 val)
    {
      u64 val64 = val == 0 ? 0 : std::bit_cast<u64>(val);
      hasher.combine_u64(val64);
    }
  };

  template <>
  struct HashTrait<b8> {
    static constexpr void combine(Hasher& hasher, b8 val)
    {
      u64 val64 = val ? 1 : 0;
      hasher.combine_u64(val64);
    }
  };

  template <typename T>
    requires(impl_hash_trait_v<T>)
  constexpr auto hash(const T& val) -> u64
  {
    Hasher hasher;
    hasher.combine(val);
    return hasher.finish();
  }

  template <typename T>
  constexpr auto hash_span(Span<T> span) -> u64
  {
    Hasher hasher;
    hasher.combine_span(span);
    return hasher.finish();
  }
} // namespace soul
