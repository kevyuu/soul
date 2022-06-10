#pragma once

#include <numeric>

#include "core/type.h"
#include "core/type_traits.h"

#define SOUL_TRAILING_ZEROES(x) TrailingZeroes(x)

namespace soul::Util
{

	// From google's filament
	template<typename T>
	constexpr T TrailingZeroes(T x) noexcept {
		static_assert(sizeof(T) <= sizeof(uint64_t), "TrailingZeroes() only support up to 64 bits");
		T c = sizeof(T) * 8;
		x &= -signed(x);
		if (x) --c;
		if (sizeof(T) * 8 > 32) { // if() only needed to quash compiler warnings
			if (x & 0x00000000FFFFFFFF) c -= 32;
		}
		if (sizeof(T) * 8 > 16 && x & 0x0000FFFF) c -= 16;
		if (sizeof(T) * 8 > 8 && x & 0x00FF00FF) c -= 8;
		if (x & 0x0F0F0F0F) c -= 4;
		if (x & 0x33333333) c -= 2;
		if (x & 0x55555555) c -= 1;
		return c;
	}
	
	template<
		typename T
	>
	requires is_lambda_v<T, void(uint32)>
	void ForEachBit (uint32 value, const T& func) {
		while (value)
		{
			uint32 bit = SOUL_TRAILING_ZEROES(value);
			func(bit);
			value &= ~(1u << bit);
		}
	}

	template <std::integral FlagSrcType, typename FlagDstType>
	FlagDstType CastFlags(FlagSrcType flags, FlagDstType mapping[]) {
		std::remove_cv_t<FlagDstType> dstFlags = 0;
		while (flags)
		{
			uint32 bit = SOUL_TRAILING_ZEROES(flags);
			dstFlags |= mapping[bit];
			flags &= ~(1u << bit);
		}
		return dstFlags;
	}

	template <typename F>
	struct ScopeExit {
		ScopeExit(F f) : f(f) {}
		~ScopeExit() { f(); }
		F f;
	};

	template <typename F>
	ScopeExit<F> make_scope_exit(F f) {
		return ScopeExit<F>(f);
	};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SCOPE_EXIT(code) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = soul::Util::make_scope_exit([=](){code;})

#if __has_builtin(__builtin_expect)
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

};

