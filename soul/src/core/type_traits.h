#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

#include "core/builtins.h"

namespace soul
{
  using false_type = std::false_type;
  using true_type = std::true_type;

  struct match_any {
    match_any() = delete;
    match_any(const match_any&) = delete;
    match_any(match_any&&) noexcept = delete;
    auto operator=(const match_any&) -> match_any& = delete;
    auto operator=(match_any&&) -> match_any& = delete;
    ~match_any() = delete;
  };

  template <typename T>
  inline b8 constexpr is_const_v = std::is_const_v<T>;

  template <typename T1, typename T2>
  inline b8 constexpr is_same_v = std::is_same_v<T1, T2>;

  template <typename T1, typename T2>
  concept same_as = std::is_same_v<T1, T2>;

  template <typename T1, typename T2>
  inline b8 constexpr is_match_v =
    is_same_v<T1, T2> || is_same_v<T1, match_any> || is_same_v<T2, match_any>;

  template <typename T1, typename T2>
  inline b8 constexpr match_as = is_match_v<T1, T2>;

  template <typename T>
  inline b8 constexpr is_lvalue_reference_v = std::is_lvalue_reference_v<T>;

  template <typename T, typename... Args>
  using invoke_result_t = std::invoke_result_t<T, Args...>;

  template <typename T>
  using remove_cv_t = std::remove_cv_t<T>;

  template <b8... boolean_values>
  constexpr auto conjunction() -> b8
  {
    b8 values[] = {boolean_values...};
    for (int i = 0; i < sizeof...(boolean_values); i++) {
      if (!values[i]) {
        return false;
      }
    }
    return true;
  }

  template <b8... boolean_values>
  inline b8 constexpr conjunction_v = conjunction<boolean_values...>();

  template <b8... boolean_values>
  constexpr auto disjunction() -> b8
  {
    b8 values[] = {boolean_values...};
    for (int i = 0; i < sizeof...(boolean_values); i++) {
      if (values[i]) {
        return true;
      }
    }
    return false;
  }

  template <b8... boolean_values>
  inline b8 constexpr disjunction_v = disjunction<boolean_values...>();

  template <typename T>
  inline constexpr b8 can_default_construct_v = std::is_default_constructible_v<T>;

  template <typename T>
  inline constexpr b8 can_trivial_default_construct_v =
    std::is_trivially_default_constructible_v<T>;

  template <typename T>
  inline constexpr b8 can_nontrivial_default_construct_v =
    can_default_construct_v<T> && can_trivial_default_construct_v<T>;

  template <typename T>
  inline constexpr b8 can_copy_v = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

  template <typename T>
  inline constexpr b8 can_trivial_copy_v =
    std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T>;

  template <typename T>
  inline constexpr b8 can_nontrivial_copy_v =
    can_copy_v<T> && !std::is_trivially_copy_constructible_v<T> &&
    !std::is_trivially_copy_assignable_v<T>;

  template <typename T>
  inline constexpr b8 can_clone_v = requires(T t1, const T& t2) {
    {
      t1.clone()
    } -> same_as<remove_cv_t<T>>;
    {
      t1.clone_from(t2)
    } -> same_as<void>;
  };

  template <typename T1, typename T2>
  inline constexpr b8 assert_same_as_v = [] {
    static_assert(std::same_as<T1, T2>, "Type is not the same");
    return true;
  }();

  template <typename T>
  inline constexpr b8 can_copy_or_clone_v = can_copy_v<T> || can_clone_v<T>;

  template <typename T>
  inline constexpr b8 can_move_v = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

  template <typename T>
  inline constexpr b8 can_trivial_move_v =
    std::is_trivially_move_constructible_v<T> && std::is_trivially_copy_assignable_v<T>;

  template <typename T>
  inline constexpr b8 can_nontrivial_move_v =
    can_move_v<T> && !std::is_trivially_move_constructible_v<T> &&
    !std::is_trivially_move_assignable_v<T>;

  template <typename T>
  inline constexpr b8 can_swap_v = std::is_swappable_v<T>;

  template <typename T>
  inline constexpr b8 can_destruct_v = std::is_destructible_v<T>;

  template <typename T>
  inline constexpr b8 can_trivial_destruct_v = std::is_trivially_destructible_v<T>;

  template <typename T>
  inline constexpr b8 can_nontrivial_destruct_v =
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
  concept ts_unsigned_integral = std::unsigned_integral<T>;

  template <typename T>
  inline b8 constexpr is_pointer_v = std::is_pointer_v<T>;

  template <typename T>
  concept ts_pointer = typeset<T> && is_pointer_v<T>;

  template <typename T, typename... Args>
  constexpr b8 can_invoke_v =
    requires(const T& fn, Args&&... args) { std::invoke(fn, std::forward<Args>(args)...); };

  template <typename T, typename... Args>
  concept ts_invocable = typeset<T> && can_invoke_v<T, Args...>;

  template <typename T, typename RetType, typename... Args>
  concept ts_fn =
    typeset<T> && can_invoke_v<T, Args...> && same_as<RetType, std::invoke_result_t<T, Args...>>;

  template <typename T, typename RetType>
  concept ts_generate_fn = ts_fn<T, RetType>;

  template <typename T, typename... Args>
  constexpr b8 can_multiple_invoke_v = (can_invoke_v<T, Args> && ...);

  template <typename T, typename Arg, typename... Args>
  constexpr b8 has_same_invoke_result_v =
    (is_same_v<invoke_result_t<T, Arg>, invoke_result_t<T, Args>> && ...);

  template <typename T, typename Arg, typename... Args>
  constexpr b8 can_visit_v =
    can_multiple_invoke_v<T, Arg, Args...> && has_same_invoke_result_v<T, Arg, Args...>;

  template <typename T, typename... Args>
  concept ts_visitor =
    typeset<T> && can_multiple_invoke_v<T, Args...> && has_same_invoke_result_v<T, Args...>;

  template <typename T1, typename T2>
  inline constexpr b8 can_convert_v = std::is_convertible_v<T1, T2>;

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

  template <typename T>
  inline b8 constexpr can_compare_equality_v = std::equality_comparable<T>;

  template <typename T = void>
  struct static_assert_error {
    static b8 constexpr value = false;
  };

  template <class T, template <class...> class Template>
  struct is_specialization : std::false_type {
  };

  template <template <class...> class Template, class... Args>
  struct is_specialization<Template<Args...>, Template> : std::true_type {
  };

  template <class T, template <class...> class Template>
  inline b8 constexpr is_specialization_v = is_specialization<T, Template>::value;

  template <typename T>
  inline b8 constexpr has_unique_object_representations_v =
    std::has_unique_object_representations_v<T>;

  template <typename T>
  using remove_pointer_t = std::remove_pointer_t<T>;
} // namespace soul
