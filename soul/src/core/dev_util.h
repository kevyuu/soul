// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#pragma once

#include <format>
#include <utility>
#include <string_view>

#include "compiler.h"

namespace soul
{
  constexpr auto relative_from_project_path(const char* filepath) -> const char*
  {
    return filepath + sizeof(SOUL_PROJECT_SOURCE_DIR) - 1;
  }

} // namespace soul

namespace soul::impl
{
  constexpr size_t PANIC_OUTPUT_MAX_LENGTH = 5096;
  auto output_panic_message(std::string_view str) -> void;
} // namespace soul::impl

namespace soul
{
  template <typename... Args>
  auto panic(
    const char* const file_name,
    const size_t line,
    const char* function,
    std::format_string<Args...> fmt = "No panic message",
    Args&&... args) -> void
  {
    char str[impl::PANIC_OUTPUT_MAX_LENGTH];
    auto result = std::format_to_n(
      str,
      std::size(str) - 1,
      "Panic in {}::{}\nin file: {}\nMessage: ",
      function,
      line,
      relative_from_project_path(file_name));
    result = std::format_to_n(
      result.out, std::size(str) - 1 - result.size, std::move(fmt), std::forward<Args>(args)...);

    impl::output_panic_message(std::string_view(str, result.out - str));
    SOUL_DEBUG_BREAK();
  }

  template <typename... Args>
  auto panic_assert(
    const char* const file_name,
    const size_t line,
    const char* function,
    const char* const expr,
    std::format_string<Args...> fmt = "No assert message",
    Args&&... args) -> void
  {
    char str[impl::PANIC_OUTPUT_MAX_LENGTH];
    auto result = std::format_to_n(
      str,
      std::size(str) - 1,
      "Assertion failed in {}::{}\nin file: {}\nExpression: ({})\nMessage: ",
      function,
      line,
      relative_from_project_path(file_name),
      expr);
    result = std::format_to_n(
      result.out, std::size(str) - 1 - result.size, std::move(fmt), std::forward<Args>(args)...);

    impl::output_panic_message(std::string_view(str, result.out - str));
    SOUL_DEBUG_BREAK();
  }
} // namespace soul

#if defined(SOUL_ASSERT_ENABLE)
#  define SOUL_ASSERT(paranoia, test, ...)                                                         \
    do {                                                                                           \
      if (!(test) && paranoia <= SOUL_ASSERT_PARANOIA_LEVEL) {                                     \
        soul::panic_assert(__FILE__, __LINE__, __FUNCTION__, #test, ##__VA_ARGS__);                \
      }                                                                                            \
    } while (0)
#  define SOUL_PANIC(...)                                                                          \
    do {                                                                                           \
      soul::panic(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                                \
    } while (0)
#  define SOUL_NOT_IMPLEMENTED()                                                                   \
    do {                                                                                           \
      soul::panic(__FILE__, __LINE__, __FUNCTION__, "Not implemented yet! \n");                    \
    } while (0)
#else
#  define SOUL_ASSERT(paranoia, test, ...) ((void)0)
#  define SOUL_PANIC(...) ((void)0)
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
