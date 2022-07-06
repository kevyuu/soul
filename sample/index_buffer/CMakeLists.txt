set(SHADER_SOURCES
	shaders/index_buffer.slang
)

create_shaders_target(index_buffer_shaders ${SHADER_SOURCES})

add_executable(index_buffer main.cpp)
target_link_libraries(index_buffer 
	PRIVATE sample_common)
add_dependencies(index_buffer index_buffer_shaders)
executable_add_slang_dependencies(index_buffer)