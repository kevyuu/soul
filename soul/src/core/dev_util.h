#pragma once

#define SOUL_DEVELOPMENT
#define SOUL_PROFILE_CPU_BACKEND_TRACY

#ifdef _MSC_VER
#define SOUL_DEBUG_BREAK() __debugbreak()
#else
#include <cstdlib>
#include <csignal>
#define SOUL_DEBUG_BREAK() do { raise(SIGSEGV); } while(0)
#endif //_MSC_VER

void soul_intern_log(int verbosity, int line, const char* file, const char* format, ...);

// Log
#define SOUL_LOG_VERBOSE_LEVEL SOUL_LOG_VERBOSE_INFO

#if defined(SOUL_DEVELOPMENT)
#define SOUL_LOG(verbosity, format, ...) do {soul_intern_log(verbosity, __LINE__, __FILE__, format, ##__VA_ARGS__);} while(0)
#else
#define SOUL_LOG(verbosity, format, ...) ((void) 0)
#endif // SOUL_DEVELOPMENT

#define SOUL_LOG_VERBOSE_COUNT 4
#define SOUL_LOG_VERBOSE_INFO 3
#define SOUL_LOG_VERBOSE_WARN 2
#define SOUL_LOG_VERBOSE_ERROR 1
#define SOUL_LOG_VERBOSE_FATAL 0

#define SOUL_LOG_INFO(format, ...) SOUL_LOG (SOUL_LOG_VERBOSE_INFO, format, ##__VA_ARGS__)
#define SOUL_LOG_WARN(format, ...) SOUL_LOG (SOUL_LOG_VERBOSE_WARN, format, ##__VA_ARGS__)
#define SOUL_LOG_ERROR(format, ...) SOUL_LOG(SOUL_LOG_VERBOSE_ERROR, format, ##__VA_ARGS__)
#define SOUL_LOG_FATAL(format, ...) SOUL_LOG(SOUL_LOG_VERBOSE_FATAL, format, ##__VA_ARGS__)

void soul_intern_assert(int paranoia, int line, const char* file, const char* format, ...);

// Assert
#define SOUL_ASSERT_PARANOIA_LEVEL 1

#if defined(SOUL_DEVELOPMENT)
#define SOUL_ASSERT(paranoia, test, msg, ...) do {if (!(test)) {soul_intern_assert(paranoia, __LINE__, __FILE__, \
        "Assertion failed: %s\n\n" msg, #test,  ##__VA_ARGS__); SOUL_DEBUG_BREAK();}} while (0)
#define SOUL_PANIC(paranoia, msg, ...) do{soul_intern_assert(paranoia, __LINE__, __FILE__, \
        "Panic! \n\n" msg, ##__VA_ARGS__); SOUL_DEBUG_BREAK();} while(0)
#define SOUL_NOT_IMPLEMENTED() do{soul_intern_assert(0, __LINE__, __FILE__, \
        "Not implemented yet! \n\n"); SOUL_DEBUG_BREAK();} while(0)
#else
#define SOUL_ASSERT(paranoia, test, msg, ...) ((void)0)
#define SOUL_PANIC(paranoia, msg) ((void)0)
#endif // SOUL_DEVELOPMENT

#if defined(SOUL_DEVELOPMENT)

#if defined(SOUL_PROFILE_CPU_BACKEND_TRACY)
#include <tracy/Tracy.hpp>
#include <tracy/common/TracySystem.hpp>
#include <tracy/TracyC.h>

struct FrameProfileScope {
	~FrameProfileScope() {
		FrameMark
	}
};


#define SOUL_PROFILE_FRAME() FrameProfileScope()
#define SOUL_PROFILE_ZONE() ZoneScoped
#define SOUL_PROFILE_ZONE_WITH_NAME(x) ZoneScopedN(x)
#define SOUL_PROFILE_THREAD_SET_NAME(x) do{tracy::SetThreadName(x);} while(0)

#endif // SOUL_PROFILE_CPU_BACKEND_TRACY

#else
#define SOUL_PROFILE_FRAME ((void) 0)
#define SOUL_PROFILE_ZONE ((void) 0)
#define SOUL_PROFILE_ZONE_WITH_NAME(x) ((void) 0)
#define SOUL_PROFILE_THREAD_SET_NAME(x) ((void) 0)

#endif // SOUL_DEVELOPMENT
