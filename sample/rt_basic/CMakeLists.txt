set(SHADER_SOURCES
	../common/shaders/rt_common.hlsl
	../common/shaders/wavefront_func.hlsl
	shaders/rt_basic_type.hlsl
	shaders/rt_basic_sample.hlsl
)

create_shaders_target(rt_basic_shaders "${SHADER_SOURCES}")

add_executable(rt_basic_sample "rt_basic_sample.cpp")
target_link_libraries(rt_basic_sample 
	PRIVATE sample_common)
add_dependencies(rt_basic_sample rt_basic_shaders)
executable_add_dxc_dependencies(rt_basic_sample)