set(SHADER_SOURCES shaders/texture_transfer_command_sample.hlsl)

create_shaders_target(texture_transfer_command_shaders "${SHADER_SOURCES}")

add_executable(texture_transfer_command_sample "texture_transfer_command_sample.cpp")
target_link_libraries(texture_transfer_command_sample PRIVATE sample_common)
add_dependencies(texture_transfer_command_sample texture_transfer_command_shaders)

add_custom_target(
  texture_transfer_command_assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets
          ${CMAKE_CURRENT_BINARY_DIR}/assets)
add_dependencies(texture_transfer_command_sample texture_transfer_command_assets)
executable_add_dxc_dependencies(texture_transfer_command_sample)
