// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#pragma once

#include "core/compiler.h"
#include "core/format.h"
#include "panic.h"

namespace soul::impl
{
  constexpr size_t PANIC_OUTPUT_MAX_LENGTH = 5096;
  auto output_panic_message(const char* str, usize size) -> void;
} // namespace soul::impl

namespace soul
{
  template <typename... Args>
  auto panic_format(
    const char* const file_name,
    const size_t line,
    const char* function,
    std::format_string<Args...> fmt = "No panic message",
    Args&&... args) -> void
  {
    char str[impl::PANIC_OUTPUT_MAX_LENGTH];
    auto result = std::format_to_n(
      str,
      std::size(str) - 1,
      "Panic in {}::{}\nin file: {}\nMessage: ",
      function,
      line,
      relative_from_project_path(file_name));
    result = std::format_to_n(
      result.out, std::size(str) - 1 - result.size, std::move(fmt), std::forward<Args>(args)...);

    impl::output_panic_message(str, result.out - str);
    SOUL_DEBUG_BREAK();
  }

  template <typename... Args>
  auto panic_assert_format(
    const char* const file_name,
    const size_t line,
    const char* function,
    const char* const expr,
    std::format_string<Args...> fmt = "No assert message",
    Args&&... args) -> void
  {
    char str[impl::PANIC_OUTPUT_MAX_LENGTH];
    auto result = std::format_to_n(
      str,
      std::size(str) - 1,
      "Assertion failed in {}::{}\nin file: {}\nExpression: ({})\nMessage: ",
      function,
      line,
      relative_from_project_path(file_name),
      expr);
    result = std::format_to_n(
      result.out, std::size(str) - 1 - result.size, std::move(fmt), std::forward<Args>(args)...);

    impl::output_panic_message(str, result.out - str);
    SOUL_DEBUG_BREAK();
  }
} // namespace soul

#if defined(SOUL_ASSERT_ENABLE)
#  define SOUL_ASSERT_FORMAT(paranoia, test, ...)                                                  \
    do                                                                                             \
    {                                                                                              \
      if (!(test) && paranoia <= SOUL_ASSERT_PARANOIA_LEVEL)                                       \
      {                                                                                            \
        soul::panic_assert_format(__FILE__, __LINE__, __FUNCTION__, #test, ##__VA_ARGS__);         \
      }                                                                                            \
    }                                                                                              \
    while (0)
#  define SOUL_PANIC_FORMAT(...)                                                                   \
    do                                                                                             \
    {                                                                                              \
      soul::panic_format(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                         \
    }                                                                                              \
    while (0)
#else
#  define SOUL_ASSERT_FORMAT(paranoia, test, ...) ((void)0)
#  define SOUL_PANIC_FORMAT(...)                  ((void)0)
#endif
