#pragma once

#include "core/dev_util.h"
#include "core/type.h"
#include <functional>

#define SOUL_TRAILING_ZEROES(x) ctz(x)
#define SOUL_TEMPLATE_ARG_LAMBDA(T, F) typename T, typename std::enable_if_t<std::is_convertible<T, std::function<F>>::value, T>* = nullptr
#define SOUL_TEMPLATE_ARG_TRIVIALLY_DESTRUCTIBLE(T) typename T, std::enable_if_t<std::is_trivially_destructible<T>::value, T>* = nullptr
#define SOUL_TEMPLATE_ARG_NOT_TRIVIALLY_DESTRUCTIBLE(T) typename T, std::enable_if_t<!std::is_trivially_destructible<T>::value, T>* = nullptr

namespace Soul { namespace Util {

	// From google's filament
	template<typename T>
	constexpr inline T ctz(T x) noexcept {
		static_assert(sizeof(T) <= sizeof(uint64_t), "ctz() only support up to 64 bits");
		T c = sizeof(T) * 8;
		x &= -signed(x);
		if (x) c--;
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

	template <SOUL_TEMPLATE_ARG_LAMBDA(T, void(uint32_t))>
	void ForEachBit (uint32 value, const T& func) {
		while (value)
		{
			uint32 bit = SOUL_TRAILING_ZEROES(value);
			func(bit);
			value &= ~(1u << bit);
		}
	}

	template <typename FlagSrcType, typename FlagDstType>
	FlagDstType CastFlags(FlagSrcType flags, FlagDstType mapping[]) {
		FlagDstType dstFlags = 0;
		while (flags)
		{
			uint32 bit = SOUL_TRAILING_ZEROES(flags);
			dstFlags |= mapping[bit];
			flags &= ~(1u << bit);
		}
		return dstFlags;
	}

	

}};

