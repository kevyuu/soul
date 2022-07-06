set(SHADER_SOURCES
	shaders/triangle_sample.hlsl
)

create_shaders_target(shaders "${SHADER_SOURCES}")

add_executable(triangle_sample "triangle_sample.cpp")
target_link_libraries(triangle_sample
	PRIVATE sample_common)
add_dependencies(triangle_sample shaders)