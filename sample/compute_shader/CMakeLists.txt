set(SHADER_SOURCES
	shaders/compute_type.hlsl
	shaders/compute_shader_sample.hlsl
)

create_shaders_target(compute_shader_sample_shaders "${SHADER_SOURCES}")

add_executable(compute_shader_sample "compute_shader_sample.cpp")
target_link_libraries(compute_shader_sample 
	PRIVATE sample_common)
add_dependencies(compute_shader_sample compute_shader_sample_shaders)