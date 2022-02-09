cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)
project(soul)

# ==================================================================================================
# General compiler flags
# ==================================================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


message(STATUS "Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path : ${CMAKE_CXX_COMPILER}")
message(STATUS "System Name : ${CMAKE_SYSTEM_NAME}")
string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")

# ==================================================================================================
# Try to find vulkan
# ==================================================================================================
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan library : ${Vulkan_LIBRARY}.")
	message(STATUS "Include Dir : ${Vulkan_INCLUDE_DIR}")
else()
	message(FATAL_ERROR "Vulkan library not found!")
endif()

# ==================================================================================================
# Try to find NVTX
# ==================================================================================================
message(STATUS $ENV{NVTOOLSEXT_PATH})
find_library(NVTX 
	NAMES nvToolsExt64_1
	PATHS "$ENV{NVTOOLSEXT_PATH}" ENV NVTOOLSEXT_PATH
	PATH_SUFFIXES "lib/x64")

if (NVTX)
	message(STATUS "Found NVTX library : ${NVTX}.")
	set(NVTX_INCLUDE_DIR "$ENV{NVTOOLSEXT_PATH}/include")
	message(STATUS "NVTX include dir : ${NVTX_INCLUDE_DIR}")
else()
	message(STATUS "NVTX library not found")
endif()


if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(dependencies)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/soul)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demo)

