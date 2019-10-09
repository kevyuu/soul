cmake_minimum_required(VERSION 3.10)

project(soul)

# ==================================================================================================
# General compiler flags
# ==================================================================================================
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path : ${CMAKE_CXX_COMPILER}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function")

# ==================================================================================================
# Try to find vulkan
# ==================================================================================================
if (APPLE)
	if (DEFINED ENV{VULKAN_SDK} AND EXISTS $ENV{VULKAN_SDK})
		message(STATUS "Found Vulkan SDK: $ENV{VULKAN_SDK}")
	else()
		message(FATAL_ERROR "Vulkan SDK not found")
	endif()

	if (DEFINED ENV{VK_ICD_FILENAMES} AND EXISTS $ENV{VK_ICD_FILENAMES})
		message(STATUS "Found Vulkan ICD Filenames: $ENV{VK_ICD_FILENAMES}")
	else()
		message(FATAL_ERROR "Vulkan ICD not found")
	endif()

	find_library(Vulkan_LIBRARY NAMES vulkan HINTS "/usr/local/lib")
    if (Vulkan_LIBRARY)
        message(STATUS "Found Vulkan library in SDK: ${Vulkan_LIBRARY}.")
		add_definitions(-DSOUL_VKLIBRARY_PATH=\"${Vulkan_LIBRARY}\")
	else()
		message(FATAL_ERROR "Vulkan library not found!")
    endif()
endif()

add_subdirectory(dependencies)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/soul)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demo)

