cmake_minimum_required(VERSION 3.22)
set(VCPKG_FEATURE_FLAGS "versions")
if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  set(CMAKE_TOOLCHAIN_FILE
      "${CMAKE_CURRENT_LIST_DIR}/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()
project(
  soul
  VERSION "0.0.1"
  LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# ==================================================================================================
# General compiler flags
# ==================================================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(SOUL_EXCEPTION_ENABLE "Build this target with exception" OFF)
if(NOT SOUL_EXCEPTION_ENABLE)
  message(STATUS "SOUL_EXCEPTION_ENABLE OFF")
  if(MSVC)
    # Disable C++ exceptions.
    string(
      REGEX
      REPLACE "/EH[a-z]+"
              ""
              CMAKE_CXX_FLAGS
              "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c- /D_HAS_EXCEPTIONS=0")
  endif()
endif()

option(SOUL_RTTI_ENABLE "Build this target with rtti" OFF)
if(NOT SOUL_RTTI_ENABLE)
  if(MSVC)
    # Disable RTTI
    string(
      REGEX
      REPLACE "/GR"
              ""
              CMAKE_CXX_FLAGS
              "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
  endif()
endif()

message(STATUS "Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path : ${CMAKE_CXX_COMPILER}")
message(STATUS "System Name : ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO : ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." ON)
if(${FORCE_COLORED_OUTPUT})
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics -fansi-escape-codes)
  endif()
endif()

option(SOUL_ASAN_ENABLE "Build this target with AddressSanitizer" OFF)
if(SOUL_ASAN_ENABLE)
  if(MSVC)
    add_compile_options(/fsanitize=address)
  else()
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
  add_compile_definitions(_DISABLE_VECTOR_ANNOTATION)
  add_compile_definitions(_DISABLE_STRING_ANNOTATION)
endif()

option(SOUL_TIME_TRACE_ENABLE "Build this target with ftime-trace" ON)
if(SOUL_TIME_TRACE_ENABLE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-ftime-trace)
  endif()
endif()

# ==================================================================================================
# Try to find vulkan
# ==================================================================================================
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
  message(STATUS "Found Vulkan library : ${Vulkan_LIBRARY}.")
  message(STATUS "Include Dir : ${Vulkan_INCLUDE_DIR}")
else()
  message(FATAL_ERROR "Vulkan library not found!")
endif()

# ==================================================================================================
# Try to find NVTX
# ==================================================================================================
message(STATUS $ENV{NVTOOLSEXT_PATH})
find_library(
  NVTX
  NAMES nvToolsExt64_1
  PATHS "$ENV{NVTOOLSEXT_PATH}" ENV NVTOOLSEXT_PATH
  PATH_SUFFIXES "lib/x64")

if(NVTX)
  message(STATUS "Found NVTX library : ${NVTX}.")
  set(NVTX_INCLUDE_DIR "$ENV{NVTOOLSEXT_PATH}/include")
  message(STATUS "NVTX include dir : ${NVTX_INCLUDE_DIR}")
else()
  message(STATUS "NVTX library not found")
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
add_library(vk_mem_alloc ALIAS unofficial::vulkan-memory-allocator::vulkan-memory-allocator)
find_package(volk REQUIRED)
find_package(Threads REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(gtest CONFIG REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/cmake_lib/SystemLink.cmake)

enable_testing()

add_subdirectory(dependencies)

add_subdirectory(soul)
add_subdirectory(sample)
add_subdirectory(test)

if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/benchmark_sandbox")
  add_subdirectory(benchmark_sandbox)
endif()

file(
  GLOB_RECURSE
  ALL_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/soul/*.cpp
  ${PROJECT_SOURCE_DIR}/soul/*.h
  ${PROJECT_SOURCE_DIR}/sample/*.cpp
  ${PROJECT_SOURCE_DIR}/sample/*.h
  ${PROJECT_SOURCE_DIR}/test/*.cpp
  ${PROJECT_SOURCE_DIR}/test/*.h)

add_custom_target(clang_format COMMAND clang-format -i ${ALL_SOURCE_FILES})
