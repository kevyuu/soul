#include <iostream>

#include "core/dev_util.h"

#if defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#  include <Windows.h>

uint32 GetOsThreadId() { return GetCurrentThreadId(); }
#endif

namespace soul::impl
{
  auto output_panic_message(std::string_view str) -> void { std::cerr << str << std::endl; }
} // namespace soul::impl
