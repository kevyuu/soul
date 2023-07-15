#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

namespace soul
{
  using false_type = std::false_type;
  using true_type = std::true_type;

  template <typename T>
  inline bool constexpr is_const_v = std::is_const_v<T>;

  template <typename T1, typename T2>
  inline bool constexpr is_same_v = std::is_same_v<T1, T2>;

  template <typename T1, typename T2>
  concept same_as = std::is_same_v<T1, T2>;

  template <typename T>
  inline bool constexpr is_lvalue_reference_v = std::is_lvalue_reference_v<T>;

  template <typename T, typename... Args>
  using invoke_result_t = std::invoke_result_t<T, Args...>;

  template <typename T>
  using remove_cv_t = std::remove_cv_t<T>;

  template <bool... boolean_values>
  constexpr auto conjunction() -> bool
  {
    bool values[] = {boolean_values...};
    for (int i = 0; i < sizeof...(boolean_values); i++) {
      if (!values[i]) {
        return false;
      }
    }
    return true;
  }

  template <bool... boolean_values>
  inline bool constexpr conjunction_v = conjunction<boolean_values...>();

  template <bool... boolean_values>
  constexpr auto disjunction() -> bool
  {
    bool values[] = {boolean_values...};
    for (int i = 0; i < sizeof...(boolean_values); i++) {
      if (values[i]) {
        return true;
      }
    }
    return false;
  }

  template <bool... boolean_values>
  inline bool constexpr disjunction_v = disjunction<boolean_values...>();

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
    } -> same_as<remove_cv_t<T>>;
    {
      t1.clone_from(t2)
    } -> same_as<void>;
  };

  template <typename T>
  inline constexpr bool can_copy_or_clone_v = can_copy_v<T> || can_clone_v<T>;

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

  template <typename T>
  concept ts_integral = std::integral<T>;

  template <typename T>
  inline bool constexpr is_pointer_v = std::is_pointer_v<T>;

  template <typename T>
  concept ts_pointer = typeset<T> && is_pointer_v<T>;

  template <typename T, typename... Args>
  constexpr bool can_invoke_v =
    requires(const T& fn, Args&&... args) { std::invoke(fn, std::forward<Args>(args)...); };

  template <typename T, typename... Args>
  concept ts_invocable = typeset<T> && can_invoke_v<T, Args...>;

  template <typename T, typename RetType, typename... Args>
  concept ts_fn =
    typeset<T> && can_invoke_v<T, Args...> && same_as<RetType, std::invoke_result_t<T, Args...>>;

  template <typename T, typename RetType>
  concept ts_generate_fn = ts_fn<T, RetType>;

  template <typename T, typename... Args>
  constexpr bool can_multiple_invoke_v = (can_invoke_v<T, Args> && ...);

  template <typename T, typename Arg, typename... Args>
  constexpr bool has_same_invoke_result_v =
    (is_same_v<invoke_result_t<T, Arg>, invoke_result_t<T, Args>> && ...);

  template <typename T, typename Arg, typename... Args>
  constexpr bool can_visit_v =
    can_multiple_invoke_v<T, Arg, Args...> && has_same_invoke_result_v<T, Arg, Args...>;

  template <typename T, typename... Args>
  concept ts_visitor =
    typeset<T> && can_multiple_invoke_v<T, Args...> && has_same_invoke_result_v<T, Args...>;

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
  inline size_t constexpr flag_count_v = T::COUNT;

  template <typename T>
  concept ts_arithmetic = std::is_arithmetic_v<T>;

  template <typeset T>
  class Option;

  template <typeset T>
  struct is_option : false_type {
  };

  template <typename T>
  struct is_option<Option<T>> : true_type {
  };

  template <typename T>
  inline bool constexpr is_option_v = is_option<T>::value;

  template <typename T>
  concept ts_option = is_option_v<T>;

  template <typename T>
  inline bool constexpr can_compare_equality_v = std::equality_comparable<T>;

  template <typename T = void>
  struct static_assert_error {
    static bool constexpr value = false;
  };
} // namespace soul
