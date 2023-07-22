set(SHADER_SOURCES 
  shaders/draw_indexed_indirect_sample.hlsl 
  shaders/draw_indexed_indirect_type.hlsl
  ../common/shaders/wavefront_material.hlsl
  ../common/shaders/raster_type.hlsl)

create_shaders_target(draw_indexed_indirect_shaders "${SHADER_SOURCES}")

add_executable(draw_indexed_indirect_sample "draw_indexed_indirect_sample.cpp")
target_link_libraries(draw_indexed_indirect_sample PRIVATE sample_common)
add_dependencies(draw_indexed_indirect_sample draw_indexed_indirect_shaders)
executable_add_dxc_dependencies(draw_indexed_indirect_sample)
