#pragma once

#include "builtins.h"

/**
 * Panic is used in a lot of places. The usage doesn't necessarily need to
 * pass a formatted string which requires template. panic_lite is used to
 * avoid circular dependency. This file should not include other headers
 * beside builtins.h
 **/
namespace soul
{
  void panic_lite(const char* file_name, usize line, const char* function, const char* message);

  void panic_assert_lite(
    const char* file_name, usize line, const char* function, const char* expr, const char* message);
} // namespace soul

#if defined(SOUL_ASSERT_ENABLE)
#  define SOUL_ASSERT_LITE(paranoia, test, message)                                                \
    do {                                                                                           \
      if (!(test) && (paranoia) <= SOUL_ASSERT_PARANOIA_LEVEL) {                                   \
        soul::panic_assert_lite(__FILE__, __LINE__, __FUNCTION__, #test, message);                 \
      }                                                                                            \
    } while (0)
#  define SOUL_PANIC_LITE(message)                                                                 \
    do {                                                                                           \
      soul::panic_lite(__FILE__, __LINE__, __FUNCTION__, message);                                 \
    } while (0)
#  define SOUL_NOT_IMPLEMENTED()                                                                   \
    do {                                                                                           \
      soul::panic(__FILE__, __LINE__, __FUNCTION__, "Not implemented yet! \n");                    \
    } while (0)
#else
#  define SOUL_ASSERT_LITE(paranoia, test, ...) ((void)0)
#  define SOUL_PANIC_LITE(...) ((void)0)
#  define SOUL_NOT_IMPLEMENTED() ((void)0)
#endif
