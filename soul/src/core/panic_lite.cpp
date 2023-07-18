#include "core/panic_lite.h"

#include "core/panic.h"

namespace soul
{

  void panic_lite(const char* file_name, usize line, const char* function, const char* message)
  {
    if (message == nullptr) {
      panic(file_name, line, function);
    } else {
      panic(file_name, line, function, "{}", message);
    }
  }

  void panic_assert_lite(
    const char* const file_name,
    const usize line,
    const char* function,
    const char* expr,
    const char* message)
  {
    if (message == nullptr) {
      panic_assert(file_name, line, function, expr);
    } else {
      panic_assert(file_name, line, function, expr, "{}", message);
    }
  }

} // namespace soul
