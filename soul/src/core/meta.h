#pragma once

#include "core/type_traits.h"

namespace soul
{
  struct MetaSentinel {
    MetaSentinel() = delete;
    MetaSentinel(const MetaSentinel& other) = delete;
    MetaSentinel(MetaSentinel&& other) = delete;
    auto operator=(const MetaSentinel& other) -> MetaSentinel& = delete;
    auto operator=(MetaSentinel&& other) -> MetaSentinel& = delete;
    ~MetaSentinel() = delete;
  };

  namespace impl
  {

    template <typename T_to_find, typename T_first, typename... T_remaining>
    inline constexpr auto get_type_index(size_t I) -> size_t
    {
      constexpr bool found = is_same_v<T_to_find, T_first>;
      constexpr bool last_element = sizeof...(T_remaining) == 0;
      static_assert(found || !last_element);
      if constexpr (found) {
        return I;
      } else if constexpr (!last_element) {
        return get_type_index<T_to_find, T_remaining...>(I + 1);
      }
    }

    template <size_t I, typename T_first = MetaSentinel, typename... T_remaining>
    struct get_type_at {
      using type = std::conditional_t<
        I == 0 || is_same_v<T_first, MetaSentinel>,
        std::type_identity<T_first>, // recursion base
        get_type_at<I - 1, T_remaining...>>::type;
    };

    template <typename T_to_count, typename T_first, typename... T_remaining>
    inline constexpr auto get_type_count() -> int
    {
      constexpr int count = std::is_same_v<T_to_count, T_first> ? 1 : 0;
      if constexpr (sizeof...(T_remaining) == 0) {
        return count;
      } else {
        return count + get_type_count<T_to_count, T_remaining...>();
      }
    }

    template <typename T_first, typename... T_remaining>
    inline constexpr auto has_duplicate_type() -> bool
    {
      if constexpr (sizeof...(T_remaining) == 0) {
        return false;
      } else if constexpr (get_type_count<T_first, T_remaining...>() > 0) {
        return true;
      } else {
        return has_duplicate_type<T_remaining...>();
      }
    }

  } // namespace impl

  template <typename T_to_find, typename... T_list>
  inline constexpr int get_type_index_v = impl::get_type_index<T_to_find, T_list...>(0);

  template <size_t I, typename... T_list>
  using get_type_at_t = typename impl::get_type_at<I, T_list...>::type;

  template <typename T_to_count, typename... T_list>
  inline constexpr int get_type_count_v = impl::get_type_count<T_to_count, T_list...>();

  template <typename... T_list>
  inline constexpr bool has_duplicate_type_v = impl::has_duplicate_type<T_list...>();

} // namespace soul
