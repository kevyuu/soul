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
	inline bool constexpr is_numeric_v = std::is_integral_v<T> || std::is_floating_point_v<T>;

	template<typename T>
	inline bool constexpr is_integral_v = std::is_integral_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_copyable_v = std::is_trivially_copyable_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_move_constructible_v = std::is_trivially_move_constructible_v<T>;

	template<typename T>
	inline bool constexpr is_trivially_destructible_v = std::is_trivially_destructible_v<T>;

	template<typename T>
	inline bool constexpr is_void_v = std::is_void_v<T>;

	template<typename T, typename U>
	inline bool constexpr is_same_v = std::is_same_v<T, U>;

	template<typename T>
	inline bool constexpr is_pointer_v = std::is_pointer_v<T>;

	namespace detail {
		namespace { // avoid ODR-violation
			template<class T>
			auto test_sizable(int) -> decltype(sizeof(T), std::true_type{});
			template<class>
			auto test_sizable(...)->std::false_type;

			template<class T>
			auto test_nonconvertible_to_int(int)
				-> decltype(static_cast<std::false_type(*)(int)>(nullptr)(std::declval<T>()));
			template<class>
			auto test_nonconvertible_to_int(...)->std::true_type;

			template<class T>
			constexpr bool is_scoped_enum_impl = std::conjunction_v<
				decltype(test_sizable<T>(0)),
				decltype(test_nonconvertible_to_int<T>(0))
			>;
		}
	} // namespace detail

	template<class>
	struct is_scoped_enum : std::false_type {};

	template<class E>
		requires std::is_enum_v<E>
	struct is_scoped_enum<E> : std::bool_constant<detail::is_scoped_enum_impl<E>> {};

	template<typename T>
	concept scoped_enum = is_scoped_enum<T>::value;

	template<typename T>
	concept counted_scoped_enum = scoped_enum<T> && requires
	{
		T::COUNT;
	};

	template<typename T>
	concept flag = counted_scoped_enum<T> && (static_cast<std::underlying_type_t<T>>(T::COUNT) < 64);

	template<uint64_t N>
	using min_uint = std::conditional<(N > std::numeric_limits<uint16_t>::max()),
		std::conditional_t<(N > std::numeric_limits<uint32_t>::max()), uint64_t, uint32_t>,
		std::conditional_t<(N > std::numeric_limits<uint8_t>::max()), uint16_t, uint8_t>
	>;

	template<uint64_t N>
	using min_uint_t = typename min_uint<N>::type;

}