#pragma once

#include <functional>
#include <type_traits>
#include <limits>
#include <cstdint>

namespace soul {

	template<typename T>
	concept arithmetic = std::is_arithmetic_v<T>;

	template<typename T, typename F>
	inline bool constexpr is_lambda_v = std::is_convertible_v<T, std::function<F>>;
	
	template<typename T>
	inline bool constexpr is_trivially_copyable_v = std::is_trivially_copyable_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_move_constructible_v = std::is_trivially_move_constructible_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_destructible_v = std::is_trivially_destructible_v<T>;

	// simplified from https://en.cppreference.com/w/cpp/types/is_scoped_enum
	template<typename T>
	concept scoped_enum = std::is_enum_v<T> && (!std::convertible_to<T, int>) && requires {
		sizeof(T);
	};

	template<typename T>
	concept flag = scoped_enum<T> && requires {
		T::COUNT;
	};

	template<uint64_t N>
	using min_uint = std::conditional<(N > std::numeric_limits<uint16_t>::max()),
		std::conditional_t<(N > std::numeric_limits<uint32_t>::max()), uint64_t, uint32_t>,
		std::conditional_t<(N > std::numeric_limits<uint8_t>::max()), uint16_t, uint8_t>
	>;

	template<uint64_t N>
	using min_uint_t = typename min_uint<N>::type;

}