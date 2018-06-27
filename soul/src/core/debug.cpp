#include "core/debug.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>

const char* _project_path(const char* filepath) {
	const char* this_file_path = __FILE__;
	return filepath + strlen(__FILE__) - strlen("core/debug.cpp");
}

void soul_log(int verbosity, int line, const char* file, const char* format, ...) {
	if (verbosity <= SOUL_LOG_VERBOSE_LEVEL) {
		printf(_project_path(file));
		printf(":");
		printf("%d", line);
		printf("::");
		va_list args_list;
		va_start(args_list, format);
		vprintf(format, args_list);
		va_end(args_list);
		printf("\n");
	}
}

void soul_assert(int paranoia, int line, const char* file, const char* format, ...) {
	if (paranoia <= SOUL_ASSERT_PARANOIA_LEVEL) {
		printf(_project_path(file));
		printf(":");
		printf("%d", line);
		printf("::");
		va_list args_list;
		va_start(args_list, format);
		vprintf(format, args_list);
		va_end(args_list);
		printf("\n");
	}
}