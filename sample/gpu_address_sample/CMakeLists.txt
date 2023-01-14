set (SHADER_SOURCES
	shaders/gpu_address_sample_type.hlsl
	shaders/gpu_address_sample.hlsl
)

create_shaders_target(gpu_address_sample_shaders "${SHADER_SOURCES}")

add_executable(gpu_address_sample "gpu_address_sample.cpp")
target_link_libraries(gpu_address_sample
	PRIVATE sample_common)
add_dependencies(gpu_address_sample gpu_address_sample_shaders)
executable_add_dxc_dependencies(gpu_address_sample)