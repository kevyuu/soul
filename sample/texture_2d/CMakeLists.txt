set(SHADER_SOURCES shaders/texture_2d_sample.hlsl)

create_shaders_target(texture_2d_shaders "${SHADER_SOURCES}")

add_executable(texture_2d_sample "texture_2d_sample.cpp")
target_link_libraries(texture_2d_sample PRIVATE sample_common)
add_dependencies(texture_2d_sample texture_2d_shaders)

add_custom_target(
  texture_2d_assets COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets
                            ${CMAKE_CURRENT_BINARY_DIR}/assets)
add_dependencies(texture_2d_sample texture_2d_assets)
executable_add_dxc_dependencies(texture_2d_sample)
