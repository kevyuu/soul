set(SHADER_SOURCES
	shaders/texture_cubemap_sample.hlsl
)

create_shaders_target(texture_cubemap_shaders "${SHADER_SOURCES}")

add_executable(texture_cubemap_sample "texture_cubemap_sample.cpp")
target_link_libraries(texture_cubemap_sample
	PRIVATE sample_common)
add_dependencies(texture_cubemap_sample texture_cubemap_shaders)