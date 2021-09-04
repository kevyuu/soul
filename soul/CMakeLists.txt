cmake_minimum_required(VERSION 3.10)
project(soul_lib)

set (PUBLIC_HEADER_DIR src)

set (SOUL_HEADERS
	
	src/core/architecture.h
	src/core/array.h
	src/core/config.h
	src/core/dev_util.h
	src/core/uint64_hash_map.h
	src/core/hash_map.h
	src/core/math.h
	src/core/packed_pool.h
	src/core/pool.h
	src/core/static_array.h
	src/core/type.h
	src/core/util.h
	src/core/slice.h
	src/core/enum_array.h
	src/core/bitset.h
	src/core/geometry.h
	src/core/string.h

	src/gpu/data.h
	src/gpu/system.h
	src/gpu/render_graph.h
	src/gpu/intern/enum_mapping.h
	src/gpu/intern/render_graph_execution.h
	src/gpu/intern/render_compiler.h
	src/gpu/render_graph_registry.h

	src/runtime/data.h
	src/runtime/system.h
	src/runtime/runtime.h
	src/runtime/scope_allocator.h

	src/memory/allocator.h
	src/memory/allocators/page_allocator.h
	src/memory/allocators/proxy_allocator.h
	src/memory/allocators/malloc_allocator.h
	src/memory/allocators/linear_allocator.h
	src/memory/allocators/mt_linear_allocator.h
)

set (SOUL_SOURCES_OSX
	src/memory/intern/osx/page_allocator.cpp)

set (SOUL_SOURCES_WINDOWS
	src/memory/intern/windows/page_allocator.cpp)

set (SOUL_SOURCES

	src/core/config.cpp
	src/core/dev_util.cpp
	src/core/math.cpp
	src/core/bitset.cpp
	src/core/geometry.cpp
	src/core/string.cpp

	src/gpu/intern/gpu_system_glfw.cpp
	src/gpu/intern/gpu_system.cpp
	src/gpu/intern/render_graph.cpp
	
	src/gpu/intern/render_graph_registry.cpp
	src/gpu/intern/render_graph_execution.cpp
	src/gpu/intern/render_graph_registry.cpp
	src/gpu/intern/render_compiler.cpp

	src/memory/intern/malloc_allocator.cpp
	src/memory/intern/proxy.cpp
	src/memory/intern/profiler.cpp
	src/memory/intern/linear_allocator.cpp
	
	src/runtime/intern/runtime_system.cpp
	src/runtime/intern/task_deque.cpp src/memory/intern/profiler.cpp
	

	"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Darwin>:${SOUL_SOURCES_OSX}>"
	"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:${SOUL_SOURCES_WINDOWS}>")

add_library(soul STATIC ${SOUL_SOURCES} "src/core/compiler.h" "src/core/type_traits.h")
target_link_libraries(soul
	PUBLIC glfw ${GLFW_LIBRARIES} vk_mem_alloc ${NVTX}
	PRIVATE volk shaderc spirv-cross-core tracy)
target_include_directories(soul 
	PUBLIC src/ ${NVTX_INCLUDE_DIR})

option(SOUL_OPTION_LOGGING_ENABLE "Option for enabling SOUL_LOGGING_*" OFF)
option(SOUL_OPTION_ASSERTION_ENABLE "Option for enabling SOUL_ASSERT" OFF)
option(SOUL_OPTION_VULKAN_VALIDATION_ENABLE "Option for enabling Vulkan validation layer" OFF)

set(SOUL_PROFILE_CPU_BACKEND "DISABLED" CACHE STRING "CPU profiling backend to use")
SET_PROPERTY(CACHE SOUL_PROFILE_CPU_BACKEND PROPERTY STRINGS DISABLED TRACY NVTX)

set(SOUL_MEMPROFILE_CPU_BACKEND "DISABLED" CACHE STRING "CPU Memory profiling backend to use")
SET_PROPERTY(CACHE SOUL_MEMPROFILE_CPU_BACKEND PROPERTY STRINGS DISABLED TRACY SOUL_PROFILER)

set(SOUL_ASSERTION_PARANOIA_LEVEL "0" CACHE STRING "Assertion paranoia level")
SET_PROPERTY(CACHE SOUL_ASSERTION_PARANOIA_LEVEL PROPERTY STRINGS 0 1 2 3 4 5)

set(SOUL_TRACY_STACKTRACE_DEPTH "0" CACHE STRING "Stacktrace depth when doing memory profile using Tracy")

target_compile_definitions(soul
	PUBLIC
		"$<$<BOOL:${SOUL_OPTION_LOGGING_ENABLE}>:SOUL_OPTION_LOGGING_ENABLE>"
		"$<$<BOOL:${SOUL_OPTION_ASSERTION_ENABLE}>:SOUL_OPTION_ASSERTION_ENABLE>"
		"$<$<BOOL:${SOUL_OPTION_VULKAN_VALIDATION_ENABLE}>:SOUL_OPTION_VULKAN_VALIDATION_ENABLE>"
		SOUL_ASSERTION_PARANOIA_LEVEL=${SOUL_ASSERTION_PARANOIA_LEVEL}
		SOUL_PROFILE_CPU_BACKEND_${SOUL_PROFILE_CPU_BACKEND}
		SOUL_MEMPROFILE_CPU_BACKEND_${SOUL_MEMPROFILE_CPU_BACKEND}
		SOUL_TRACY_STACKTRACE_DEPTH=${SOUL_TRACY_STACKTRACE_DEPTH}
	PRIVATE
		"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:SOUL_OS_WINDOWS>"
		"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Darwin>:SOUL_OS_APPLE>")


