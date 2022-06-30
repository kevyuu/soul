#pragma once
#include <cstdint>

namespace soul
{
    enum class Compiler : uint8_t
    {
        MSVC,
        COUNT
    };
}

#if defined(_MSC_VER)
#include <intrin.h>
namespace soul {
    constexpr Compiler COMPILER = Compiler::MSVC;
}
#endif

#define SOUL_NODISCARD [[nodiscard]]

#if defined(_MSC_VER)
#	define SOUL_DEBUG_BREAK() __debugbreak()
#else
#	include <cstdlib>
#	include <csignal>
#	define SOUL_DEBUG_BREAK() do { raise(SIGSEGV); } while(0)
#endif //_MSC_VER

#if defined(_MSC_VER)
#	define SOUL_NOOP __noop
#else
#	define SOUL_NOOP ((void) 0) 
#endif

// compatibility with non-clang compilers...
#ifndef __has_attribute
#	define __has_attribute(x) SOUL_NOOP
#endif
#ifndef __has_builtin
#	define __has_builtin(x) SOUL_NOOP
#endif


#if __has_builtin (__builtin_expect)
#   ifdef __cplusplus
#      define SOUL_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#      define SOUL_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#   else
#      define SOUL_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#      define SOUL_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#   endif
#else
#   define SOUL_LIKELY( exp )    (!!(exp))
#   define SOUL_UNLIKELY( exp )  (!!(exp))
#endif

#if __has_attribute(noinline)
#define SOUL_NOINLINE __attribute__((noinline))
#else
#define SOUL_NOINLINE
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1900
#    define SOUL_RESTRICT __restrict
#elif (defined(__clang__) || defined(__GNUC__))
#    define SOUL_RESTRICT __restrict__
#else
#    define SOUL_RESTRICT
#endif

#if __has_attribute(maybe_unused)
#define SOUL_UNUSED [[maybe_unused]]
#define SOUL_UNUSED_IN_RELEASE [[maybe_unused]]
#elif __has_attribute(unused)
#define SOUL_UNUSED __attribute__((unused))
#define SOUL_UNUSED_IN_RELEASE __attribute__((unused))
#else
#define SOUL_UNUSED
#define SOUL_UNUSED_IN_RELEASE
#endif

#if __has_attribute(always_inline)
#define SOUL_ALWAYS_INLINE __attribute__((always_inline))
#else
#define SOUL_ALWAYS_INLINE inline
#endif

namespace soul
{
    inline size_t pop_count_16(const uint16_t val)
    {
        static_assert(COMPILER == Compiler::MSVC);
        if constexpr (COMPILER == Compiler::MSVC)
        {
            return __popcnt16(val);
        }
    }

    inline size_t pop_count_32(const uint32_t val)
    {
        static_assert(COMPILER == Compiler::MSVC);
        if constexpr (COMPILER == Compiler::MSVC)
        {
            return __popcnt(val);
        }
    }

    inline size_t pop_count_64(const uint64_t val)
    {
        static_assert(COMPILER == Compiler::MSVC);
        if constexpr (COMPILER == Compiler::MSVC)
        {
            return __popcnt64(val);
        }
    }

}