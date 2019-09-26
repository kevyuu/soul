cmake_minimum_required(VERSION 3.10)

project(soul)

set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

# ==================================================================================================
# General compiler flags
# ==================================================================================================
set(CMAKE_CXX_STANDARD 14)
if (WIN32)
    set(CXX_STANDARD 14)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function --debug")

if (CLANG_CL)
    # Since the "secure" replacements that MSVC suggests are not portable, disable
    # the deprecation warnings. Also disable warnings about use of POSIX functions (i.e. "unlink").
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE")
endif()

message(STATUS "CC : $ENV{CC}")
message(STATUS "CXX : $ENV{CXX}")
message(STATUS "Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path : ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Flags : ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ Debug Flags : ${CMAKE_CXX_FLAGS_DEBUG}")

# ==================================================================================================
# Try to find vulkan
# ==================================================================================================
if (DEFINED ENV{VULKAN_SDK})
	message(STATUS "Found Vulkan SDK: $ENV{VULKAN_SDK}")
else()
	if (APPLE)
		set(ENV{VULKAN_SDK} ${EXTERNAL}/vulkan/macOS)
	endif()
	message(STATUS "Vulkan SDK not found. Use local copy: $ENV{VULKAN_SDK}")
endif()

if (APPLE)
	find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/macOS/lib")
    if (Vulkan_LIBRARY)
		set(Vulkan_FOUND ON)
        message(STATUS "Found Vulkan library in SDK: ${Vulkan_LIBRARY}.")
		add_definitions(-DSOUL_VKLIBRARY_PATH=\"${Vulkan_LIBRARY}\")
	else()
		message(FATAL_ERROR "Vulkan library not found!")
    endif()
endif()

add_subdirectory(dependencies)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/soul)

