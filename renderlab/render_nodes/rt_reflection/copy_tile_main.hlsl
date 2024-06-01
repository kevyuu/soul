#include "render_nodes/common/sv.hlsl"
#include "render_nodes/rt_reflection/rt_reflection.shared.hlsl"

[[vk::push_constant]] CopyTilePC push_constant;

[numthreads(COPY_TILE_WORK_GROUP_SIZE_X, COPY_TILE_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  Texture2D input_texture = get_texture_2d(push_constant.input_texture);
  RWTexture2D<vec4f32> output = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  vec2u32 coord = get_buffer<vec2u32>(push_constant.copy_coords_buffer, sv.group_id.x * sizeof(vec2u32)) +
                  vec2u32(sv.local_id.xy);
  output[coord] = input_texture.Load(vec3i32(coord, 0));
}

// ------------------------------------------------------------------
