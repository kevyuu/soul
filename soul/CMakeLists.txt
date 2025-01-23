set(PUBLIC_HEADER_DIR src)

set(SOUL_SOURCES_OSX src/memory/impl/osx/page_allocator.cpp)

set(SOUL_SOURCES_WINDOWS src/memory/impl/windows/page_allocator.cpp
                         src/app/impl/windows_platform/app.cpp)

set(SOUL_SOURCES
    src/core/log.cpp
    src/core/panic.cpp
    src/core/panic_format.cpp
    src/memory/impl/linear_allocator.cpp
    src/memory/impl/malloc_allocator.cpp
    src/memory/impl/proxy.cpp
    src/misc/filesystem.cpp
    src/misc/image_data.cpp
    src/misc/json.cpp
    src/misc/string_util.cpp
    src/gpu/impl/vulkan/bindless_descriptor_allocator.cpp
    src/gpu/impl/vulkan/common.cpp
    src/gpu/impl/vulkan/glfw_wsi.cpp
    src/gpu/impl/vulkan/gpu_system.cpp
    src/gpu/impl/vulkan/render_compiler.cpp
    src/gpu/impl/vulkan/render_graph.cpp
    src/gpu/impl/vulkan/render_graph_registry.cpp
    src/gpu/impl/vulkan/render_graph_execution.cpp
    src/gpu/impl/vulkan/vma_impl.cpp
    src/gpu/impl/vulkan/type.cpp
    src/runtime/impl/runtime_system.cpp
    src/runtime/impl/task_deque.cpp
    "$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Darwin>:${SOUL_SOURCES_OSX}>"
    "$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:${SOUL_SOURCES_WINDOWS}>")

add_library(soul STATIC ${SOUL_SOURCES})
target_include_directories(soul PUBLIC src ${PROJECT_SOURCE_DIR}/dependencies)
target_link_system_libraries(
  soul
  PUBLIC
  glfw
  nvtx3-cpp
  volk::volk
  vk_mem_alloc
  Tracy::TracyClient
  PRIVATE
  dxc)
target_compile_definitions(soul PUBLIC TRACY_ENABLE NOMINMAX SOUL_HOST_CODE)

get_target_property(soul_include_directories soul INCLUDE_DIRECTORIES)
message(STATUS "Soul include directories : ${soul_include_directories}")

option(SOUL_ASSERT_ENABLE "Option for enabling SOUL_ASSERT" OFF)
option(SOUL_VULKAN_VALIDATION_ENABLE "Option for enabling Vulkan validation layer" OFF)

set(SOUL_LOG_LEVEL
    "DISABLED"
    CACHE STRING "log level")
set_property(
  CACHE SOUL_LOG_LEVEL
  PROPERTY STRINGS
           FATA
           ERROR
           WARN
           INFO
           DEBUG)

set(SOUL_PROFILE_CPU_BACKEND
    "DISABLED"
    CACHE STRING "CPU profiling backend to use")
set_property(
  CACHE SOUL_PROFILE_CPU_BACKEND
  PROPERTY STRINGS
           DISABLED
           TRACY
           NVTX)

set(SOUL_MEMPROFILE_CPU_BACKEND
    "DISABLED"
    CACHE STRING "CPU Memory profiling backend to use")
set_property(
  CACHE SOUL_MEMPROFILE_CPU_BACKEND
  PROPERTY STRINGS
           DISABLED
           TRACY
           SOUL_PROFILER)

set(SOUL_ASSERT_PARANOIA_LEVEL
    "0"
    CACHE STRING "Assertion paranoia level")

set(SOUL_TRACY_STACKTRACE_DEPTH
    "0"
    CACHE STRING "Stacktrace depth when doing memory profile using Tracy")

target_compile_definitions(
  soul
  PUBLIC "$<$<BOOL:${SOUL_ASSERT_ENABLE}>:SOUL_ASSERT_ENABLE>"
         "$<$<BOOL:${SOUL_VULKAN_VALIDATION_ENABLE}>:SOUL_VULKAN_VALIDATION_ENABLE>"
         SOUL_ASSERT_PARANOIA_LEVEL=${SOUL_ASSERT_PARANOIA_LEVEL}
         SOUL_LOG_LEVEL=${SOUL_LOG_LEVEL}
         SOUL_PROFILE_CPU_BACKEND_${SOUL_PROFILE_CPU_BACKEND}
         SOUL_MEMPROFILE_CPU_BACKEND_${SOUL_MEMPROFILE_CPU_BACKEND}
         SOUL_TRACY_STACKTRACE_DEPTH=${SOUL_TRACY_STACKTRACE_DEPTH}
         SOUL_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}/"
  PRIVATE "$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:SOUL_OS_WINDOWS>"
          "$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Darwin>:SOUL_OS_APPLE>")

target_precompile_headers(
  soul
  PUBLIC
  <algorithm>
  <atomic>
  <concepts>
  <condition_variable>
  <cstdint>
  <filesystem>
  <format>
  <limits>
  <mutex>
  <ranges>
  <random>
  <shared_mutex>
  <thread>)

set(SOUL_APP_SOURCES
    src/app/input_state.cpp
    src/app/window.cpp
    src/app/gui.cpp
    src/app/app.cpp)

add_library(soul_app STATIC ${SOUL_APP_SOURCES})
target_include_directories(soul_app PUBLIC src ${PROJECT_SOURCE_DIR}/dependencies)
target_link_system_libraries(
  soul_app
  PUBLIC
  soul
  PRIVATE
  yyjson::yyjson
  imgui
  imguizmo
  glfw)
target_compile_definitions(soul_app PRIVATE IMGUI_USER_CONFIG="app/impl/soul_imconfig.h")
