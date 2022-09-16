#include "core/dev_util.h"

#if defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#include <Windows.h>

uint32_t GetOsThreadId() {
	return GetCurrentThreadId();
}
#endif