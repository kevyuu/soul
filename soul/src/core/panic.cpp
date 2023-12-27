#include "core/panic.h"

#include "core/panic_format.h"

namespace soul
{

  void panic(const char* file_name, usize line, const char* function, const char* message)
  {
    if (message == nullptr) {
      panic_format(file_name, line, function);
    } else {
      panic_format(file_name, line, function, "{}", message);
    }
  }

  void panic_assert(
    const char* const file_name,
    const usize line,
    const char* function,
    const char* expr,
    const char* message)
  {
    if (message == nullptr) {
      panic_assert_format(file_name, line, function, expr);
    } else {
      panic_assert_format(file_name, line, function, expr, "{}", message);
    }
  }

  void panic_assert_upper_bound_check(
    const char* file_name, usize line, const char* function, usize index, usize upper_bound_index)
  {
    panic_assert_format(
      file_name,
      line,
      function,
      "index < upper_bound_index",
      "Bount check error : index = {}, upper_bound_index = {}",
      index,
      upper_bound_index);
  }

  void panic_assert_lower_bound_check(
    const char* file_name, usize line, const char* function, usize index, usize lower_bound_index)
  {
    panic_assert_format(
      file_name,
      line,
      function,
      "index >= lower_bound_index",
      "Bount check error : index = {}, lower_bound_index = {}",
      index,
      lower_bound_index);
  }

} // namespace soul
