#include "render_nodes/shadow/shadow_type.hlsl"
#include "render_nodes/common/sv.hlsl"


[[vk::push_constant]] CopyTilePC push_constant;

[numthreads(COPY_TILE_WORK_GROUP_SIZE_X, COPY_TILE_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  RWTexture2D<vec2f32> output = get_rw_texture_2d_vec2f32(push_constant.output_texture);

  vec2u32 coord = get_buffer<vec2u32>(push_constant.copy_coords_buffer, sv.group_id.x * sizeof(vec2u32)) + vec2u32(sv.local_id.xy);

  output[coord] = vec2f32(0.0, 0.0);
}

// ------------------------------------------------------------------