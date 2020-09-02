#include "core/dev_util.h"
#include "memory/profiler.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>

const char* _project_path(const char* filepath) {
	const char* this_file_path = __FILE__;
	return filepath + strlen(__FILE__) - strlen("core/debug.cpp");
}

static const char* LOG_PREFIX[] = {
	"FATAL",
	"ERROR",
	"WARN",
	"INFO",
};

void soul_intern_log(int verbosity, int line, const char* file, const char* format, ...) {
	if (verbosity <= SOUL_LOG_VERBOSE_LEVEL) {
		printf("%s", LOG_PREFIX[verbosity]);
		printf(":");
		printf("%s", _project_path(file));
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

void soul_intern_assert(int paranoia, int line, const char* file, const char* format, ...) {
	if (paranoia <= SOUL_ASSERT_PARANOIA_LEVEL) {
		printf("%s", _project_path(file));
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

#if defined(SOUL_MEMPROFILE_CPU_BACKEND_SOUL_PROFILER)

Soul::Memory::Profiler* _GetProfiler() {
	SOUL_NOT_IMPLEMENTED();
	return nullptr;
}

void MemProfile::RegisterAllocator(const char *name) {
	_GetProfiler()->registerAllocator(name);
}

void MemProfile::UnregisterAllocator(const char *name) {
	_GetProfiler()->unregisterAllocator(name);
}

void MemProfile::RegisterAllocation(const char *name, const char *tag, const void *addr, uint32 size) {
	_GetProfiler()->registerAllocation(name, tag, addr, size);
}

void MemProfile::RegisterDeallocation(const char *name, const void *addr, uint32 size) {
	_GetProfiler()->registerDeallocation(name, addr, size);
}

void MemProfile::Snapshot(const char* name) {
	_GetProfiler()->snapshot(name);
}

MemProfile::Scope::Scope() {
	_GetProfiler()->beginFrame();
}

MemProfile::Scope::~Scope() {
	_GetProfiler()->endFrame();
}
#endif

#if defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#include <Windows.h>

uint32_t GetOsThreadId() {
	return GetCurrentThreadId();
}
#endif