set(SHADER_SOURCES shaders/texture_cube_sample.hlsl)

create_shaders_target(texture_cube_shaders "${SHADER_SOURCES}")

add_executable(texture_cube_sample "texture_cube_sample.cpp")
target_link_libraries(texture_cube_sample PRIVATE sample_common)
add_dependencies(texture_cube_sample texture_cube_shaders)

add_custom_target(
  texture_cube_assets COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets
                              ${CMAKE_CURRENT_BINARY_DIR}/assets)
add_dependencies(texture_cube_sample texture_cube_assets)
executable_add_dxc_dependencies(texture_cube_sample)
