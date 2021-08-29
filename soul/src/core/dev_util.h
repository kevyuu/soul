// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#pragma once

#if defined(SOUL_ENV_DEBUG)
    #define SOUL_OPTION_VULKAN_VALIDATION_ENABLE
    #define SOUL_ASSERT_PARANOIA_LEVEL 1
	#define SOUL_OPTION_LOGGING_ENABLE
	#define SOUL_OPTION_ASSERTION_ENABLE
#else
    #define SOUL_ASSERT_PARANOIA_LEVEL 0
#endif // SOUL_ENV_DEBUG

#if defined(SOUL_ENV_RELEASE)
	#define SOUL_OPTION_LOGGING_ENABLE
	#define SOUL_OPTION_ASSERTION_ENABLE
	#define SOUL_PROFILE_CPU_BACKEND_TRACY
#endif // SOUL_ENV_RELEASE

#if defined(_MSC_VER)
    #define SOUL_DEBUG_BREAK() __debugbreak()
#else
    #include <cstdlib>
    #include <csignal>
    #define SOUL_DEBUG_BREAK() do { raise(SIGSEGV); } while(0)
#endif //_MSC_VER

void soul_intern_log(int verbosity, int line, const char* file, const char* format, ...);

// Log
#define SOUL_LOG_VERBOSE_LEVEL SOUL_LOG_VERBOSE_INFO

#if defined(SOUL_OPTION_LOGGING_ENABLE)
    #define SOUL_LOG(verbosity, format, ...) do {soul_intern_log(verbosity, __LINE__, __FILE__, format, ##__VA_ARGS__);} while(0)
#else
    #define SOUL_LOG(verbosity, format, ...) ((void) 0)
#endif // SOUL_OPTION_ASSERTION_ENABLE

// Logging
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
#if defined(SOUL_OPTION_ASSERTION_ENABLE)
    #define SOUL_ASSERT(paranoia, test, msg, ...) do {if (!(test)) {soul_intern_assert(paranoia, __LINE__, __FILE__, \
            "Assertion failed: %s\n\n" msg, #test,  ##__VA_ARGS__); SOUL_DEBUG_BREAK();}} while (0)
    #define SOUL_PANIC(paranoia, msg, ...) do{soul_intern_assert(paranoia, __LINE__, __FILE__, \
            "Panic! \n\n" msg, ##__VA_ARGS__); SOUL_DEBUG_BREAK();} while(0)
    #define SOUL_NOT_IMPLEMENTED() do{soul_intern_assert(0, __LINE__, __FILE__, \
            "Not implemented yet! \n\n"); SOUL_DEBUG_BREAK();} while(0)
#else
    #define SOUL_ASSERT(paranoia, test, msg, ...) ((void)0)
    #define SOUL_PANIC(paranoia, msg) ((void)0)
    #define SOUL_NOT_IMPLEMENTED() ((void)0)
#endif // SOUL_OPTION_ASSERTION_ENABLE

// Profiling

#if defined(SOUL_PROFILE_CPU_BACKEND_TRACY)

    #include <tracy/Tracy.hpp>
    #include <tracy/TracyC.h>

    struct FrameProfileScope {

    	FrameProfileScope() = default;
        FrameProfileScope(const FrameProfileScope&) = delete;
        FrameProfileScope& operator=(const FrameProfileScope&) = delete;
        FrameProfileScope(FrameProfileScope&&) = delete;
        FrameProfileScope& operator=(FrameProfileScope&&) = delete;
        ~FrameProfileScope() {
            FrameMark
        }
    };


    #define SOUL_PROFILE_FRAME() FrameProfileScope()
    #define SOUL_PROFILE_ZONE() ZoneScoped
    #define SOUL_PROFILE_ZONE_WITH_NAME(x) ZoneScopedN(x)
    #define SOUL_PROFILE_THREAD_SET_NAME(x) do{tracy::SetThreadName(x);} while(0)

#elif defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
    #include "core/type.h"
    #include <nvToolsExt.h>
    struct NVTXScope {
        NVTXScope(const char* name) {
            nvtxRangePush(name);
        }

        ~NVTXScope() {
            nvtxRangePop();
        }
    };

    uint32_t GetOsThreadId();

    #define SOUL_PROFILE_FRAME() NVTXScope("Frame")
    #define SOUL_PROFILE_ZONE() NVTXScope(__FUNCTION__)
    #define SOUL_PROFILE_ZONE_WITH_NAME(x) NVTXScope(x)
    #define SOUL_PROFILE_THREAD_SET_NAME(x) do{nvtxNameOsThread(GetOsThreadId(), x);} while(0)
    
#else

    #define SOUL_PROFILE_FRAME ((void) 0)
    #define SOUL_PROFILE_ZONE ((void) 0)
    #define SOUL_PROFILE_ZONE_WITH_NAME(x) ((void) 0)
    #define SOUL_PROFILE_THREAD_SET_NAME(x) ((void) 0)

#endif // SOUL_PROFILE_CPU_BACKEND

#if defined(SOUL_MEMPROFILE_CPU_BACKEND_SOUL_PROFILER)
	#include "core/type.h"
	struct MemProfile {

		struct Scope {
			Scope();
			~Scope();
		};

		static void RegisterAllocator(const char* name);
		static void UnregisterAllocator(const char* name);
		static void RegisterAllocation(const char* name, const char* tag, const void* addr, uint32 size);
		static void RegisterDeallocation(const char* name, const void* addr, uint32 size);
		static void Snapshot(const char* name);
	};

	#define SOUL_MEMPROFILE_REGISTER_ALLOCATOR(x) do { MemProfile::RegisterAllocator(x);} while(0)
	#define SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(x) do { MemProfile::UnregisterAllocator(x);} while(0)
	#define SOUL_MEMPROFILE_REGISTER_ALLOCATION(allocatorName, tag, addr, size) do { MemProfile::RegisterAllocation(allocatorName, tag, addr, size);} while(0)
	#define SOUL_MEMPROFILE_REGISTER_DEALLOCATION(allocatorName, addr, size) do{ MemProfile::RegisterDeallocation(allocatorName, addr, size); } while(0)
	#define SOUL_MEMPROFILE_SNAPSHOT(x) do{ MemProfile::Snapshot(x); } while(0)
	#define SOUL_MEMPROFILE_FRAME() MemProfile::Scope()
#elif defined(SOUL_MEMPROFILE_CPU_BACKEND_TRACY)

	#if !defined(SOUL_PROFILE_CPU_BACKEND_TRACY)
		static_assert(false, "Please enable tracy cpu profiler to use the memory profiler");
	#endif

	#include <tracy/Tracy.hpp>
	#include "core/type.h"
	#define SOUL_MEMPROFILE_REGISTER_ALLOCATOR(x) do {} while(0)
	#define SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(x) do {} while(0)
	#define SOUL_MEMPROFILE_REGISTER_ALLOCATION(allocatorName, tag, addr, size) do { TracyAllocNS(addr, size, 10, allocatorName); } while(0)
	#define SOUL_MEMPROFILE_REGISTER_DEALLOCATION(allocatorName, addr, size) do{ TracyFreeNS(addr, 10, allocatorName); } while(0)
	#define SOUL_MEMPROFILE_SNAPSHOT(x) do{} while(0)
	#define SOUL_MEMPROFILE_FRAME() do{} while(0)

#else

	#define SOUL_MEMPROFILE_REGISTER_ALLOCATOR(x) ((void) 0)
	#define SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(x) ((void) 0)
	#define SOUL_MEMPROFILE_REGISTER_ALLOCATION(allocatorName, tag, addr, size) ((void) 0)
	#define SOUL_MEMPROFILE_REGISTER_DEALLOCATION(allocatorName, addr, size) ((void) 0)
	#define SOUL_MEMPROFILE_SNAPSHOT(x) ((void) 0)
	#define SOUL_MEMPROFILE_FRAME() ((void) 0)

#endif // SOUL_MEMPROFILE_CPU_BACKEND
