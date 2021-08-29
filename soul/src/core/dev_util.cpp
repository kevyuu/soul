#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "core/dev_util.h"
#include "memory/profiler.h"

static const char* ProjectPath(const char* filepath) {
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
		printf("%s", ProjectPath(file));
		printf(":");
		printf("%d", line);
		printf("::");
		va_list argsList;
		va_start(argsList, format);
		vprintf(format, argsList);
		printf("\n");
	}
}

void soul_intern_assert(int paranoia, int line, const char* file, const char* format, ...) {
	if (paranoia <= SOUL_ASSERT_PARANOIA_LEVEL) {
		printf("%s", ProjectPath(file));
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