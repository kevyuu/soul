set(SHADER_SOURCES
	shaders/texture_3d_sample.hlsl
)

create_shaders_target(texture_3d_shaders "${SHADER_SOURCES}")

add_executable(texture_3d_sample "texture_3d_sample.cpp")
target_link_libraries(texture_3d_sample
	PRIVATE sample_common)
add_dependencies(texture_3d_sample texture_3d_shaders)