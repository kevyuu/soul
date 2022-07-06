set(SHADER_SOURCES
	shaders/transform.hlsl
	shaders/buffer_transfer_command_sample.hlsl
)

create_shaders_target(update_buffer_shaders "${SHADER_SOURCES}")

add_executable(buffer_transfer_command_sample "buffer_transfer_command_sample.cpp")
target_link_libraries(buffer_transfer_command_sample
	PRIVATE sample_common)
add_dependencies(buffer_transfer_command_sample update_buffer_shaders)