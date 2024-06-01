#pragma once

#include <type_traits>

#include "boolean.h"
#include "integer.h"

namespace soul
{
  constexpr auto relative_from_project_path(const char* filepath) -> const char*
  {
    return filepath + sizeof(SOUL_PROJECT_SOURCE_DIR) - 1;
  }

  void panic(
    const char* file_name, usize line, const char* function, const char* message = nullptr);

  void panic_assert(
    const char* file_name,
    usize line,
    const char* function,
    const char* expr,
    const char* message = nullptr);

  void panic_assert_upper_bound_check(
    const char* file_name, usize line, const char* function, usize index, usize upper_bound_index);

  void panic_assert_lower_bound_check(
    const char* file_name, usize line, const char* function, usize index, usize lower_bound_index);

} // namespace soul

#if defined(SOUL_ASSERT_ENABLE)
#  define SOUL_ASSERT(paranoia, test, ...)                                                         \
    do                                                                                             \
    {                                                                                              \
      if (!std::is_constant_evaluated() && !(test) && (paranoia) <= SOUL_ASSERT_PARANOIA_LEVEL)    \
      {                                                                                            \
        soul::panic_assert(__FILE__, __LINE__, __FUNCTION__, #test, ##__VA_ARGS__);                \
      }                                                                                            \
    }                                                                                              \
    while (0)
#  define SOUL_PANIC(message)                                                                      \
    do                                                                                             \
    {                                                                                              \
      if (!std::is_constant_evaluated())                                                           \
      {                                                                                            \
        soul::panic(__FILE__, __LINE__, __FUNCTION__, message);                                    \
      }                                                                                            \
    }                                                                                              \
    while (0)
#  define SOUL_NOT_IMPLEMENTED()                                                                   \
    do                                                                                             \
    {                                                                                              \
      if (!std::is_constant_evaluated())                                                           \
      {                                                                                            \
        soul::panic(__FILE__, __LINE__, __FUNCTION__, "Not implemented yet! \n");                  \
      }                                                                                            \
    }                                                                                              \
    while (0)
#  define SOUL_ASSERT_UPPER_BOUND_CHECK(index, upper_bound_index)                                  \
    do                                                                                             \
    {                                                                                              \
      if (!std::is_constant_evaluated() && ((index) >= (upper_bound_index)))                       \
      {                                                                                            \
        soul::panic_assert_upper_bound_check(                                                      \
          __FILE__, __LINE__, __FUNCTION__, index, upper_bound_index);                             \
      }                                                                                            \
    }                                                                                              \
    while (0)
#  define SOUL_ASSERT_LOWER_BOUND_CHECK(index, lower_bound_index)                                  \
    do                                                                                             \
    {                                                                                              \
      if (!std::is_constant_evaluated() && ((index) < (lower_bound_index)))                        \
      {                                                                                            \
        soul::panic_assert_lower_bound_check(                                                      \
          __FILE__, __LINE__, __FUNCTION__, index, lower_bound_index);                             \
      }                                                                                            \
    }                                                                                              \
    while (0)
#else
#  define SOUL_ASSERT(paranoia, test, ...)                        ((void)0)
#  define SOUL_PANIC(...)                                         ((void)0)
#  define SOUL_NOT_IMPLEMENTED()                                  ((void)0)
#  define SOUL_ASSERT_UPPER_BOUND_CHECK(index, upper_bound_index) ((void)0)
#  define SOUL_ASSERT_LOWER_BOUND_CHECK(index, lower_bound_index) ((void)0)
#endif
