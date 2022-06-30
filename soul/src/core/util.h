#pragma once

#include "core/compiler.h"
#include "core/type.h"
#include "core/type_traits.h"
#include <optional>

namespace soul::util
{

	template<std::unsigned_integral T>
	constexpr std::optional<uint32> get_first_one_bit_pos(T x) noexcept {
		if (x == 0) return std::nullopt;
		constexpr uint32 bit_count = sizeof(T) * 8;
	    static_assert(bit_count <= 64);
		uint32 n = bit_count;
		x &= -signed(x);
		if (x) --n;
		if constexpr (bit_count > 32) if (x & 0x00000000FFFFFFFF) n -= 32;
		if constexpr (bit_count > 16) if (x & 0x0000FFFF) n -= 16;
		if constexpr (bit_count > 8) if (x & 0x00FF00FF) n -= 8;
		if (x & 0x0F0F0F0F) n -= 4;
		if (x & 0x33333333) n -= 2;
		if (x & 0x55555555) n -= 1;
		return n;
	}

    template<std::unsigned_integral T>
	constexpr std::optional<uint32> get_last_one_bit_pos(T x) noexcept
	{
		constexpr uint32 bit_count = sizeof(T) * 8;
		static_assert(bit_count <= 64);
	    if (x)
	    {
			uint32 n = 0;
			if constexpr (bit_count > 32)
				if (x & 0xFFFFFFFF00000000) { n += 32; x >>= 32; }
			if constexpr (bit_count > 16)
				if (x & 0xFFFF0000) { n += 16; x >>= 16; }
			if constexpr (bit_count > 8)
				if (x & 0xFF00) { n += 8; x >>= 8; }
			if (x & 0xFFF0) { n += 4; x >>= 4; }
			if (x & 0xFFFC) { n += 2; x >>= 2; }
			if (x & 0xFFFE) { n += 1; }
			return n;
	    }
		return std::nullopt;
	}

	template<std::unsigned_integral T>
	constexpr soul_size get_one_bit_count(T x) noexcept
	{
		static_assert(sizeof(T) <= 8);
		if constexpr (std::same_as<T, uint8> || std::same_as<T, uint16>)
			return pop_count_16(x);
	    if constexpr (std::same_as<T, uint32>)
			return pop_count_32(x);
		return pop_count_64(x);
	
	}
	
	template<std::unsigned_integral Integral, typename Func>
	requires is_lambda_v<Func, void(uint32)>
	void for_each_one_bit_pos (Integral value, const Func& func) {
		while (value) {
			uint32 bit_pos = *get_first_one_bit_pos(value);
			func(bit_pos);
			value &= ~(1u << bit_pos);
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
    auto STRING_JOIN2(scope_exit_, __LINE__) = soul::util::make_scope_exit([=](){code;})

};

