#include "core/dev_util.h"

#if defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#  include <Windows.h>

uint32 GetOsThreadId() { return GetCurrentThreadId(); }
#endif
