#include "core/dev_util.h"

#include <cstdlib>

#if defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#include <Windows.h>

uint32_t GetOsThreadId() {
	return GetCurrentThreadId();
}
#endif

void soul_exit_failure(const int line, const char* const file, _In_z_ _Printf_format_string_ const char* const format, ...) {
    printf("EXIT_FAILURE:%s", project_path(file));
    printf(":");
    printf("%d", line);
    printf("::");
    va_list args_list;
    va_start(args_list, format);
    vprintf(format, args_list);  // NOLINT(clang-diagnostic-format-nonliteral)
    va_end(args_list);
    printf("\n");
    exit(-1);
}