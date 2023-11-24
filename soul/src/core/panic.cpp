#include <iostream>
#include <string_view>

#include "panic.h"

#if defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#  include <Windows.h>

uint32 GetOsThreadId() { return GetCurrentThreadId(); }
#endif

namespace soul::impl
{
  auto output_panic_message(const char* str, usize size) -> void
  {
    std::cerr << std::string_view(str, size) << std::endl;
  }
} // namespace soul::impl
