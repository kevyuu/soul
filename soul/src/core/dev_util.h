// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "compiler.h"

namespace soul
{
  constexpr auto relative_from_project_path(const char* filepath) -> const char*
  {
    return filepath + sizeof(SOUL_PROJECT_SOURCE_DIR) - 1;
  }
} // namespace soul

static auto project_path(const char* filepath) -> const char*
{
  return filepath + strlen(__FILE__) - strlen("core/debug.cpp");
}

// Assertion
constexpr auto soul_intern_assert(
  const int paranoia,
  const int line,
  const char* const file,
  _In_z_ _Printf_format_string_ const char* const format,
  ...) -> void
{
  if (paranoia <= SOUL_ASSERT_PARANOIA_LEVEL) {
    printf("%s", project_path(file));
    printf(":");
    printf("%d", line);
    printf("::");
    va_list args_list;
    va_start(args_list, format);
    vprintf(format, args_list); // NOLINT(clang-diagnostic-format-nonliteral)
    va_end(args_list);
    printf("\n");
  }
}

#if defined(SOUL_ASSERT_ENABLE)
#  define SOUL_ASSERT(paranoia, test, msg, ...)                                                    \
    do {                                                                                           \
      if (!(test)) {                                                                               \
        soul_intern_assert(                                                                        \
          paranoia, __LINE__, __FILE__, "Assertion failed: %s\n\n" msg, #test, ##__VA_ARGS__);     \
        SOUL_DEBUG_BREAK();                                                                        \
      }                                                                                            \
    } while (0)
#  define SOUL_PANIC(msg, ...)                                                                     \
    do {                                                                                           \
      soul_intern_assert(0, __LINE__, __FILE__, "Panic! \n\n" msg, ##__VA_ARGS__);                 \
      SOUL_DEBUG_BREAK();                                                                          \
    } while (0)
#  define SOUL_NOT_IMPLEMENTED()                                                                   \
    do {                                                                                           \
      soul_intern_assert(0, __LINE__, __FILE__, "Not implemented yet! \n\n");                      \
      SOUL_DEBUG_BREAK();                                                                          \
    } while (0)
#else
#  define SOUL_ASSERT(paranoia, test, msg, ...) ((void)0)
#  define SOUL_PANIC(paranoia, msg) ((void)0)
#  define SOUL_NOT_IMPLEMENTED() ((void)0)
#endif

// Profiling
#if defined(SOUL_PROFILE_CPU_BACKEND_TRACY)

#  include <Tracy.hpp>
#  include <TracyC.h>

struct FrameProfileScope {
  FrameProfileScope() = default;
  FrameProfileScope(const FrameProfileScope&) = delete;
  FrameProfileScope& operator=(const FrameProfileScope&) = delete;
  FrameProfileScope(FrameProfileScope&&) = delete;
  FrameProfileScope& operator=(FrameProfileScope&&) = delete;
  ~FrameProfileScope() { FrameMark }
};

#  define SOUL_PROFILE_FRAME() FrameProfileScope()
#  define SOUL_PROFILE_ZONE() ZoneScoped(void) 0
#  define SOUL_PROFILE_ZONE_WITH_NAME(x) ZoneScopedN(x)(void) 0
#  define SOUL_PROFILE_THREAD_SET_NAME(x)                                                          \
    do {                                                                                           \
      tracy::SetThreadName(x);                                                                     \
    } while (0)
#  define SOUL_LOCKABLE(type, varname) TracyLockable(type, varname)
#  define SOUL_SHARED_LOCKABLE(type, varname) TracySharedLockable(type, varname)

#elif defined(SOUL_PROFILE_CPU_BACKEND_NVTX)
#  include <nvToolsExt.h>
#  include "core/type.h"
struct NVTXScope {
  NVTXScope(const char* name) { nvtxRangePush(name); }

  ~NVTXScope() { nvtxRangePop(); }
};

uint32_t GetOsThreadId();

#  define SOUL_PROFILE_FRAME() NVTXScope nvtx_scope("Frame")
#  define SOUL_PROFILE_ZONE() NVTXScope nvtx_scope(__FUNCTION__)
#  define SOUL_PROFILE_ZONE_WITH_NAME(x) NVTXScope(x)
#  define SOUL_PROFILE_THREAD_SET_NAME(x)                                                          \
    do {                                                                                           \
      nvtxNameOsThread(GetOsThreadId(), x);                                                        \
    } while (0)
#  define SOUL_LOCKABLE(type, varname) type varname
#  define SOUL_SHARED_LOCKABLE(type, varname) type varname

#else

#  define SOUL_PROFILE_FRAME SOUL_NOOP
#  define SOUL_PROFILE_ZONE SOUL_NOOP
#  define SOUL_PROFILE_ZONE_WITH_NAME(x) SOUL_NOOP
#  define SOUL_PROFILE_THREAD_SET_NAME(x) SOUL_NOOP
#  define SOUL_LOCKABLE(type, var_name) type var_name
#  define SOUL_SHARED_LOCKABLE(type, var_name) type var_name

#endif // SOUL_PROFILE_CPU_BACKEND

#if defined(SOUL_MEMPROFILE_CPU_BACKEND_SOUL_PROFILER)
#  include "core/type.h"
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

#  define SOUL_MEMPROFILE_REGISTER_ALLOCATOR(x)                                                    \
    do {                                                                                           \
      MemProfile::RegisterAllocator(x);                                                            \
    } while (0)
#  define SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(x)                                                  \
    do {                                                                                           \
      MemProfile::UnregisterAllocator(x);                                                          \
    } while (0)
#  define SOUL_MEMPROFILE_REGISTER_ALLOCATION(allocatorName, tag, addr, size)                      \
    do {                                                                                           \
      MemProfile::RegisterAllocation(allocatorName, tag, addr, size);                              \
    } while (0)
#  define SOUL_MEMPROFILE_REGISTER_DEALLOCATION(allocatorName, addr, size)                         \
    do {                                                                                           \
      MemProfile::RegisterDeallocation(allocatorName, addr, size);                                 \
    } while (0)
#  define SOUL_MEMPROFILE_SNAPSHOT(x)                                                              \
    do {                                                                                           \
      MemProfile::Snapshot(x);                                                                     \
    } while (0)
#  define SOUL_MEMPROFILE_FRAME() MemProfile::Scope()
#elif defined(SOUL_MEMPROFILE_CPU_BACKEND_TRACY)

#  if !defined(SOUL_PROFILE_CPU_BACKEND_TRACY)
static_assert(false, "Please enable tracy cpu profiler to use the memory profiler");
#  endif

#  include "core/type.h"
#  include <tracy/Tracy.hpp>

#  define SOUL_MEMPROFILE_REGISTER_ALLOCATOR(x) SOUL_NOOP
#  define SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(x) SOUL_NOOP
#  define SOUL_MEMPROFILE_REGISTER_ALLOCATION(allocatorName, tag, addr, size)                      \
    do {                                                                                           \
      TracyAllocNS(addr, size, SOUL_TRACY_STACKTRACE_DEPTH, allocatorName);                        \
    } while (0)
#  define SOUL_MEMPROFILE_REGISTER_DEALLOCATION(allocatorName, addr, size)                         \
    do {                                                                                           \
      TracyFreeNS(addr, SOUL_TRACY_STACKTRACE_DEPTH, allocatorName);                               \
    } while (0)
#  define SOUL_MEMPROFILE_SNAPSHOT(x) SOUL_NOOP
#  define SOUL_MEMPROFILE_FRAME() SOUL_NOOP

#else

#  define SOUL_MEMPROFILE_REGISTER_ALLOCATOR(x) SOUL_NOOP
#  define SOUL_MEMPROFILE_DEREGISTER_ALLOCATOR(x) SOUL_NOOP
#  define SOUL_MEMPROFILE_REGISTER_ALLOCATION(allocatorName, tag, addr, size) SOUL_NOOP
#  define SOUL_MEMPROFILE_REGISTER_DEALLOCATION(allocatorName, addr, size) SOUL_NOOP
#  define SOUL_MEMPROFILE_SNAPSHOT(x) SOUL_NOOP
#  define SOUL_MEMPROFILE_FRAME() SOUL_NOOP

#endif // SOUL_MEMPROFILE_CPU_BACKEND
