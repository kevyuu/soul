#pragma once

#include <functional>
#include <type_traits>

#define SOUL_REQUIRE(...) std::enable_if_t<(__VA_ARGS__)>* = nullptr

namespace Soul {

	template<bool T>
	using require = std::enable_if_t<T>;

	template<typename T, typename F>
	inline bool constexpr is_lambda_v = std::is_convertible_v<T, std::function<F>>;
	
	template<typename T>
	inline bool constexpr is_numeric_v = std::is_integral_v<T> || std::is_floating_point_v<T>;

	template<typename T>
	inline bool constexpr is_integral_v = std::is_integral_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_copyable_v = std::is_trivially_copyable_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_move_constructible_v = std::is_trivially_move_constructible_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_destructible_v = std::is_trivially_destructible_v<T>;
}