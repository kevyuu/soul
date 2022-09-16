set(SHADER_SOURCES
	shaders/index_buffer_sample.hlsl
)

create_shaders_target(index_buffer_shaders "${SHADER_SOURCES}")

add_executable(index_buffer_sample "index_buffer_sample.cpp")
target_link_libraries(index_buffer_sample 
	PRIVATE sample_common)
add_dependencies(index_buffer_sample index_buffer_shaders)
executable_add_dxc_dependencies(index_buffer_sample)