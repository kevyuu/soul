#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

namespace soul
{

  template <typename T>
  concept arithmetic = std::is_arithmetic_v<T>;

  template <typename T>
  concept trivially_copy_constructible = std::is_trivially_copy_constructible_v<T>;
  template <typename T>
  concept untrivially_copy_constructible =
    std::is_copy_constructible_v<T> && !std::is_trivially_copy_constructible_v<T>;
  template <typename T>
  concept trivially_copy_assignable = std::is_trivially_copy_assignable_v<T>;
  template <typename T>
  concept untrivially_copy_assinable =
    std::is_copy_assignable_v<T> && !std::is_trivially_copy_assignable_v<T>;
  template <typename T>
  concept copyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

  template <typename T>
  concept trivially_move_constructible = std::is_trivially_move_constructible_v<T>;
  template <typename T>
  concept untrivially_move_constructible =
    std::is_move_constructible_v<T> && !std::is_trivially_move_constructible_v<T>;
  template <typename T>
  concept trivially_move_assignable = std::is_trivially_move_assignable_v<T>;
  template <typename T>
  concept untrivially_move_assignable =
    std::is_move_assignable_v<T> && !std::is_trivially_move_assignable_v<T>;
  template <typename T>
  concept untrivially_movable = untrivially_move_constructible<T> && untrivially_move_assignable<T>;

  template <typename T>
  concept trivially_destructible = std::is_trivially_destructible_v<T>;
  template <typename T>
  concept untrivially_destructible =
    std::is_destructible_v<T> && !std::is_trivially_destructible_v<T>;

  template <typename T>
  concept ts_copy = std::is_trivially_copyable_v<T>;

  template <typename T>
  concept ts_clone = requires(T t1, const T& t2) {
    {
      t1.clone()
    } -> std::same_as<std::remove_cv_t<T>>;
    {
      t1.clone_from(t2)
    } -> std::same_as<void>;
  } && std::movable<T> && std::is_destructible_v<T>;

  template <typename T>
  concept ts_move_only =
    !ts_clone<T> && !std::is_copy_constructible_v<T> && !std::is_copy_assignable_v<T> &&
    untrivially_movable<T> && untrivially_destructible<T>;

  template <typename T>
  concept ts_immovable = !ts_clone<T> && !std::is_copy_constructible_v<T> &&
                         !std::is_copy_assignable_v<T> && !std::is_move_constructible_v<T> &&
                         !std::is_move_assignable_v<T> && untrivially_destructible<T>;

  template <typename T>
  concept typeset = ts_clone<T> || ts_copy<T> || ts_move_only<T> || ts_immovable<T>;

  template <typename T, typename RetType, typename... Args>
  concept callable =
    std::regular_invocable<T, Args...> && std::same_as<RetType, std::invoke_result_t<T, Args...>>;

  template <typename T, typename RetType>
  concept ts_generate_fn = callable<T, RetType>;

  template <typename T, typename F>
  inline bool constexpr is_lambda_v = std::is_convertible_v<T, std::function<F>>;

  template <typename T>
  inline bool constexpr is_trivially_copyable_v = std::is_trivially_copyable_v<T>;

  template <typename T>
  inline bool constexpr is_trivially_move_constructible_v =
    std::is_trivially_move_constructible_v<T>;

  template <typename T>
  inline bool constexpr is_trivially_destructible_v = std::is_trivially_destructible_v<T>;

  // simplified from https://en.cppreference.com/w/cpp/types/is_scoped_enum
  template <typename T>
  concept scoped_enum =
    std::is_enum_v<T> && (!std::convertible_to<T, int>)&&requires { sizeof(T); };

  template <typename T>
  concept flag = scoped_enum<T> && requires { T::COUNT; };

  template <uint64_t N>
  using min_uint = std::conditional<
    (N > std::numeric_limits<uint16_t>::max()),
    std::conditional_t<(N > std::numeric_limits<uint32_t>::max()), uint64_t, uint32_t>,
    std::conditional_t<(N > std::numeric_limits<uint8_t>::max()), uint16_t, uint8_t>>;

  template <uint64_t N>
  using min_uint_t = typename min_uint<N>::type;

  template <typename T>
  class Option;

  template <typename T>
  struct is_option : std::false_type {
  };

  template <typename T>
  struct is_option<Option<T>> : std::true_type {
  };

  template <typename T>
  inline bool constexpr is_option_v = is_option<T>::value;

  template <typename T>
  concept c_option = is_option_v<T>;

} // namespace soul
