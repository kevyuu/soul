cmake_minimum_required(VERSION 3.10)
project(soul_lib)

set (PUBLIC_HEADER_DIR src)

set (SOUL_HEADERS
	
	src/core/architecture.h
	src/core/array.h
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

	src/gpu/data.h
	src/gpu/system.h
	src/gpu/render_graph.h
	src/gpu/intern/enum_mapping.h
	src/gpu/intern/render_graph_execution.h
	src/gpu/render_graph_registry.h

	src/runtime/data.h
	src/runtime/system.h
	src/runtime/runtime.h

	src/memory/allocator.h
	src/memory/allocators/page_allocator.h
	src/memory/allocators/proxy_allocator.h
	src/memory/allocators/malloc_allocator.h
	src/memory/allocators/linear_allocator.h
	src/memory/allocators/mt_linear_allocator.h
	src/memory/allocators/scope_allocator.h
)

set (SOUL_SOURCES_OSX
	src/memory/intern/osx/page_allocator.cpp)

set (SOUL_SOURCES_WINDOWS
	src/memory/intern/windows/page_allocator.cpp)

set (SOUL_SOURCES

	src/core/dev_util.cpp
	src/core/math.cpp
	src/core/bitset.cpp

	src/gpu/intern/gpu_system_glfw.cpp
	src/gpu/intern/gpu_system.cpp
	src/gpu/intern/render_graph.cpp
	src/gpu/intern/commands.cpp
	src/gpu/intern/render_graph_registry.cpp
	src/gpu/intern/render_graph_execution.cpp
	src/gpu/intern/render_graph_registry.cpp

	src/memory/intern/malloc_allocator.cpp
	src/memory/intern/proxy.cpp
	src/memory/intern/profiler.cpp
	src/memory/intern/linear_allocator.cpp
	src/memory/intern/mt_linear_allocator.cpp
	src/memory/intern/scope_allocator.cpp

		src/runtime/intern/runtime_system.cpp
	src/runtime/intern/task_deque.cpp src/memory/intern/profiler.cpp

	"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Darwin>:${SOUL_SOURCES_OSX}>"
	"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:${SOUL_SOURCES_WINDOWS}>")

add_library(soul STATIC ${SOUL_SOURCES})
target_link_libraries(soul
	PUBLIC glfw ${GLFW_LIBRARIES} vk_mem_alloc
	PRIVATE volk shaderc spirv-cross-core tracy)
target_include_directories(soul 
	PUBLIC src/)
target_compile_definitions(soul
	PUBLIC
		"$<$<CONFIG:Debug>:SOUL_ENV_DEBUG>"
		"$<$<CONFIG:Release>:SOUL_ENV_RELEASE>"
	PRIVATE
		"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:SOUL_OS_WINDOWS>"
		"$<$<STREQUAL:${CMAKE_SYSTEM_NAME},Darwin>:SOUL_OS_APPLE>")
