cmake_minimum_required(VERSION 3.10)

project(soul)

# ==================================================================================================
# General compiler flags
# ==================================================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path : ${CMAKE_CXX_COMPILER}")

# ==================================================================================================
# Try to find vulkan
# ==================================================================================================
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan library : ${Vulkan_LIBRARY}.")
else()
	message(FATAL_ERROR "Vulkan library not found!")
endif()


add_subdirectory(dependencies)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/soul)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demo)

