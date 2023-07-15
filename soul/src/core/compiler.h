#pragma once

#include <cstdint>

namespace soul
{
  enum class Compiler : uint8_t { MSVC, COUNT };
}

#if defined(_MSC_VER)
#  include <intrin.h>

namespace soul
{
  constexpr Compiler COMPILER = Compiler::MSVC;
}
#endif

#if defined(_MSC_VER)
#  define SOUL_DEBUG_BREAK() __debugbreak()
#else
#  include <csignal>
#  include <cstdlib>
#  define SOUL_DEBUG_BREAK()                                                                       \
    do {                                                                                           \
      raise(SIGSEGV);                                                                              \
    } while (0)
#endif //_MSC_VER

#define SOUL_NOOP ((void)0)

// compatibility with non-clang compilers...
#ifndef __has_attribute
#  define __has_attribute(x) SOUL_NOOP
#endif
#ifndef __has_builtin
#  define __has_builtin(x) SOUL_NOOP
#endif

#if __has_builtin(__builtin_expect)
#  ifdef __cplusplus
#    define SOUL_LIKELY(exp) (__builtin_expect(!!(exp), true))
#    define SOUL_UNLIKELY(exp) (__builtin_expect(!!(exp), false))
#  else
#    define SOUL_LIKELY(exp) (__builtin_expect(!!(exp), 1))
#    define SOUL_UNLIKELY(exp) (__builtin_expect(!!(exp), 0))
#  endif
#else
#  define SOUL_LIKELY(exp) (!!(exp))
#  define SOUL_UNLIKELY(exp) (!!(exp))
#endif

#if __has_attribute(noinline)
#  define SOUL_NOINLINE __attribute__((noinline))
#else
#  define SOUL_NOINLINE
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1900
#  define SOUL_RESTRICT __restrict
#elif (defined(__clang__) || defined(__GNUC__))
#  define SOUL_RESTRICT __restrict__
#else
#  define SOUL_RESTRICT
#endif

#if __has_attribute(maybe_unused)
#  define SOUL_UNUSED [[maybe_unused]]
#  define SOUL_UNUSED_IN_RELEASE [[maybe_unused]]
#elif __has_attribute(unused)
#  define SOUL_UNUSED __attribute__((unused))
#  define SOUL_UNUSED_IN_RELEASE __attribute__((unused))
#else
#  define SOUL_UNUSED
#  define SOUL_UNUSED_IN_RELEASE
#endif

#if __has_attribute(always_inline)
#  define SOUL_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#  define SOUL_ALWAYS_INLINE __forceinline
#endif

#if defined(_MSC_VER)
#  define SOUL_FORMAT_STRING _In_z_ _Printf_format_string_
#endif

#if __clang__
#  define SOUL_NO_UNIQUE_ADDRESS
#elif _MSC_VER >= 1929 // VS2019 v16.10 and later (_MSC_FULL_VER >= 192829913 for VS 2019 v16.9)
#  define SOUL_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#  define SOUL_NO_UNIQUE_ADDRESS
#endif

namespace soul
{

  inline auto pop_count_16(const uint16_t val) -> size_t
  {
    static_assert(COMPILER == Compiler::MSVC);
    if constexpr (COMPILER == Compiler::MSVC) {
      return __popcnt16(val);
    }
  }

  inline auto pop_count_32(const uint32_t val) -> size_t
  {
    static_assert(COMPILER == Compiler::MSVC);
    if constexpr (COMPILER == Compiler::MSVC) {
      return __popcnt(val);
    }
  }

  inline auto pop_count_64(const uint64_t val) -> size_t
  {
    static_assert(COMPILER == Compiler::MSVC);
    if constexpr (COMPILER == Compiler::MSVC) {
      return __popcnt64(val);
    }
  }

  [[noreturn]]
  inline void unreachable()
  {
    // Uses compiler specific extensions if possible.
    // Even if no extension is used, undefined behavior is still raised by
    // an empty function body and the noreturn attribute.
#ifdef __GNUC__ // GCC, Clang, ICC
    __builtin_unreachable();
#elifdef _MSC_VER // MSVC
    __assume(false);
#endif
  }

} // namespace soul
