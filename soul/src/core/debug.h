#pragma once

#define SOUL_DEVELOPMENT
#define SOUL_PROFILE_BACKEND_NSIGHT

#ifdef _MSC_VER
#define SOUL_DEBUG_BREAK() __debugbreak()
#else
#define SOUL_DEBUG_BREAK ((void)0)
#endif //_MSC_VER

// Log
#define SOUL_LOG_VERBOSE_LEVEL SOUL_LOG_VERBOSE_INFO

#if defined(SOUL_DEVELOPMENT)
#define SOUL_LOG(verbosity, format, ...) do {soul_log(verbosity, __LINE__, __FILE__, format, __VA_ARGS__);} while(0)
#else
#define SOUL_LOG(verbosity, format, ...) ((void) 0)
#endif // SOUL_DEVELOPMENT

#define SOUL_LOG_VERBOSE_INFO 4
#define SOUL_LOG_VERBOSE_WARN 3
#define SOUL_LOG_VERBOSE_ERROR 2
#define SOUL_LOG_VERBOSE_FATAL 1

#define SOUL_LOG_INFO(format, ...) SOUL_LOG (SOUL_LOG_VERBOSE_INFO, format, __VA_ARGS__)
#define SOUL_LOG_WARN(format, ...) SOUL_LOG (SOUL_LOG_VERBOSE_WARN, format, __VA_ARGS__)
#define SOUL_LOG_ERROR(format, ...) SOUL_LOG(SOUL_LOG_VERBOSE_ERROR, format, __VA_ARGS__)
#define SOUL_LOG_FATAL(format, ...) SOUL_LOG(SOUL_LOG_VERBOSE_FATAL, format, __VA_ARGS__)

void soul_log(int verbosity, int line, const char* file, const char* format, ...);

// Assert
#define SOUL_ASSERT_PARANOIA_LEVEL 1

#if defined(SOUL_DEVELOPMENT)
#define SOUL_ASSERT(paranoia, test, msg, ...) do {if (!(test)) {soul_assert(paranoia, __LINE__, __FILE__, \
        "Assertion failed: %s\n\n" msg, #test,  __VA_ARGS__); SOUL_DEBUG_BREAK();}} while (0)
#else
#define SOUL_ASSERT(paranoia, test, msg, ...) ((void)0)
#endif // SOUL_DEVELOPMENT

void soul_assert(int paranoia, int line, const char* file, const char* format, ...);

#if defined(SOUL_DEVELOPMENT)

#define SOUL_PROFILE_RANGE_PUSH(msg) ((void)0)
#define SOUL_PROFILE_RANGE_POP(msg) ((void)0)

#if defined(SOUL_PROFILE_BACKEND_NSIGHT)
#include <nvToolsExt.h>
#define SOUL_PROFILE_RANGE_PUSH(name) do {nvtxRangePush(name);} while(0)
#define SOUL_PROFILE_RANGE_POP() do {nvtxRangePop();} while(0)
#endif // SOUL_PROFILE_BACKEND_NSIGHT

#endif // SOUL_DEVELOPMENT
