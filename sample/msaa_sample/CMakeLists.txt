set(SHADER_SOURCES
	shaders/msaa_type.hlsl
	shaders/msaa_sample.hlsl
)

create_shaders_target(msaa_shaders "${SHADER_SOURCES}")

add_executable(msaa_sample "msaa_sample.cpp")
target_link_libraries(msaa_sample
	PRIVATE sample_common)
add_dependencies(msaa_sample msaa_shaders)
executable_add_dxc_dependencies(msaa_sample)