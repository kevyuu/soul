#pragma once

#define DEVELOPMENT

#ifdef _MSC_VER
#define SOUL_DEBUG_BREAK() __debugbreak()
#else
#define SOUL_DEBUG_BREAK ((void)0)
#endif //_MSC_VER

// Log
#define SOUL_LOG_VERBOSE_LEVEL SOUL_LOG_VERBOSE_INFO

#if defined(DEVELOPMENT)
#define SOUL_LOG(verbosity, format, ...) do {soul_log(verbosity, __LINE__, __FILE__, format, __VA_ARGS__);} while(0)
#else
#define SOUL_LOG(verbosity, format, ...) ((void) 0)
#endif // DEVELOPMENT

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

#if defined(DEVELOPMENT)
#define SOUL_ASSERT(paranoia, test, msg, ...) do {if (!(test)) {soul_assert(paranoia, __LINE__, __FILE__, \
        "Assertion failed: %s\n\n" msg, #test,  __VA_ARGS__); SOUL_DEBUG_BREAK();}} while (0)
#else
#define SOUL_ASSERT(paranoia, test, msg, ...) ((void)0)
#endif // DEVELOPMENT

void soul_assert(int paranoia, int line, const char* file, const char* format, ...);



