#pragma once

#include "core/type.h"
#include "core/type_traits.h"

namespace soul::Util
{

	// From google's filament
	template<typename T>
	constexpr T trailing_zeroes(T x) noexcept {
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
	
	template<typename T>
	requires is_lambda_v<T, void(uint32)>
	void for_each_bit (uint32 value, const T& func) {
		while (value)
		{
			uint32 bit = trailing_zeroes(value);
			func(bit);
			value &= ~(1u << bit);
		}
	}

	template <typename F>
	requires is_lambda_v<F, void()>
	struct ScopeExit {
		explicit ScopeExit(F f) : f(f) {}
		ScopeExit(const ScopeExit&) = delete;
		ScopeExit& operator=(const ScopeExit&) = delete;
		ScopeExit(ScopeExit&&) = delete;
		ScopeExit& operator=(ScopeExit&&) = delete;
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

};

