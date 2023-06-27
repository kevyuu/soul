#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

namespace soul
{

  template <typename T>
  inline constexpr bool can_default_construct_v = std::is_default_constructible_v<T>;

  template <typename T>
  inline constexpr bool can_trivial_default_construct_v =
    std::is_trivially_default_constructible_v<T>;

  template <typename T>
  inline constexpr bool can_nontrivial_default_construct_v =
    can_default_construct_v<T> && can_trivial_default_construct_v<T>;

  template <typename T>
  inline constexpr bool can_copy_v =
    std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

  template <typename T>
  inline constexpr bool can_trivial_copy_v =
    std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T>;

  template <typename T>
  inline constexpr bool can_nontrivial_copy_v =
    can_copy_v<T> && !std::is_trivially_copy_constructible_v<T> &&
    !std::is_trivially_copy_assignable_v<T>;

  template <typename T>
  inline constexpr bool can_clone_v = requires(T t1, const T& t2) {
    {
      t1.clone()
    } -> std::same_as<std::remove_cv_t<T>>;
    {
      t1.clone_from(t2)
    } -> std::same_as<void>;
  };

  template <typename T>
  inline constexpr bool can_move_v =
    std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

  template <typename T>
  inline constexpr bool can_trivial_move_v =
    std::is_trivially_move_constructible_v<T> && std::is_trivially_copy_assignable_v<T>;

  template <typename T>
  inline constexpr bool can_nontrivial_move_v =
    can_move_v<T> && !std::is_trivially_move_constructible_v<T> &&
    !std::is_trivially_move_assignable_v<T>;

  template <typename T>
  inline constexpr bool can_swap_v = std::is_swappable_v<T>;

  template <typename T>
  inline constexpr bool can_destruct_v = std::is_destructible_v<T>;

  template <typename T>
  inline constexpr bool can_trivial_destruct_v = std::is_trivially_destructible_v<T>;

  template <typename T>
  inline constexpr bool can_nontrivial_destruct_v =
    std::is_destructible_v<T> && !std::is_trivially_destructible_v<T>;

  template <typename T>
  concept ts_copy = std::is_trivially_copyable_v<T>;

  template <typename T>
  concept ts_clone = !std::is_reference_v<T> && can_clone_v<T> && can_nontrivial_move_v<T> &&
                     can_nontrivial_destruct_v<T>;

  template <typename T>
  concept ts_nontrivial_copy = !std::is_reference_v<T> && can_nontrivial_copy_v<T> &&
                               can_nontrivial_move_v<T> && can_nontrivial_destruct_v<T>;

  template <typename T>
  concept ts_move_only = !std::is_reference_v<T> && !can_copy_v<T> && !can_clone_v<T> &&
                         can_nontrivial_move_v<T> && can_nontrivial_destruct_v<T>;

  template <typename T>
  concept ts_immovable = !std::is_reference_v<T> && !can_clone_v<T> && !can_copy_v<T> &&
                         !can_move_v<T> && can_nontrivial_destruct_v<T>;

  template <typename T>
  concept typeset = ts_clone<T> || ts_copy<T> || ts_move_only<T> || ts_immovable<T>;

  template <typename T, typename... Args>
  constexpr bool can_invoke_v =
    requires(const T& fn, Args&&... args) { std::invoke(fn, std::forward<Args>(args)...); };

  template <typename T, typename... Args>
  concept ts_invocable = typeset<T> && can_invoke_v<T, Args...>;

  template <typename T, typename RetType, typename... Args>
  concept ts_fn = typeset<T> && can_invoke_v<T, Args...> &&
                  std::same_as<RetType, std::invoke_result_t<T, Args...>>;

  template <typename T, typename RetType>
  concept ts_generate_fn = ts_fn<T, RetType>;

  template <typename T1, typename T2>
  inline constexpr bool can_convert_v = std::is_convertible_v<T1, T2>;

  template <typename T1, typename T2>
  concept ts_convertible_to = typeset<T1> && can_convert_v<T1, T2>;

  // simplified from https://en.cppreference.com/w/cpp/types/is_scoped_enum
  template <typename T>
  concept ts_scoped_enum =
    std::is_enum_v<T> && (!std::convertible_to<T, int>)&&requires { sizeof(T); };

  template <typename T>
  concept ts_flag = ts_scoped_enum<T> && requires { T::COUNT; };

  template <typename T>
  concept ts_arithmetic = std::is_arithmetic_v<T>;

  template <typeset T>
  class Option;

  template <typeset T>
  struct is_option : std::false_type {
  };

  template <typename T>
  struct is_option<Option<T>> : std::true_type {
  };

  template <typename T>
  inline bool constexpr is_option_v = is_option<T>::value;

  template <typename T>
  concept ts_option = is_option_v<T>;

} // namespace soul
