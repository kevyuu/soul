cmake_minimum_required(VERSION 3.10)
project(soul_lib)

set (PUBLIC_HEADER_DIR src)

set (SOUL_HEADERS
	
	src/core/architecture.h
	src/core/array.h
	src/core/debug.h
	src/core/hash_map.h
	src/core/math.h
	src/core/packed_array.h
	src/core/pool_array.h
	src/core/type.h

	src/gpu/data.h
	src/gpu/render_graph.h
	src/gpu/system.h

	src/job/data.h
	src/job/system.h

)

set (SOUL_SOURCES

	src/core/debug.cpp
	src/core/math.cpp

	src/gpu/intern/gpu_system_glfw.cpp
	src/gpu/intern/gpu_system.cpp
	src/gpu/intern/render_graph.cpp

	src/job/intern/job_system.cpp
	src/job/intern/task_deque.cpp
)

add_library(soul STATIC ${SOUL_SOURCES})
target_link_libraries(soul
	PUBLIC glfw ${GLFW_LIBRARIES} vk_mem_alloc
	PRIVATE volk)
target_include_directories(soul 
	PUBLIC src/)
target_compile_definitions(soul
	PUBLIC 
		$<$<CONFIG:Debug>:SOUL_OPTION_VULKAN_ENABLE_VALIDATION> 
		SOUL_OS_APPLE)