#include "render_nodes/common/sv.hlsl"
#include "render_nodes/ddgi/ddgi.shared.hlsl"

[[vk::push_constant]] ProbeBorderUpdatePC push_constant;

static const vec4i32 probe_border_offsets[68] = {
  vec4i32(16, 1, 1, 0),   vec4i32(15, 1, 2, 0),   vec4i32(14, 1, 3, 0),   vec4i32(13, 1, 4, 0),
  vec4i32(12, 1, 5, 0),   vec4i32(11, 1, 6, 0),   vec4i32(10, 1, 7, 0),   vec4i32(9, 1, 8, 0),
  vec4i32(8, 1, 9, 0),    vec4i32(7, 1, 10, 0),   vec4i32(6, 1, 11, 0),   vec4i32(5, 1, 12, 0),
  vec4i32(4, 1, 13, 0),   vec4i32(3, 1, 14, 0),   vec4i32(2, 1, 15, 0),   vec4i32(1, 1, 16, 0),
  vec4i32(16, 16, 1, 17), vec4i32(15, 16, 2, 17), vec4i32(14, 16, 3, 17), vec4i32(13, 16, 4, 17),
  vec4i32(12, 16, 5, 17), vec4i32(11, 16, 6, 17), vec4i32(10, 16, 7, 17), vec4i32(9, 16, 8, 17),
  vec4i32(8, 16, 9, 17),  vec4i32(7, 16, 10, 17), vec4i32(6, 16, 11, 17), vec4i32(5, 16, 12, 17),
  vec4i32(4, 16, 13, 17), vec4i32(3, 16, 14, 17), vec4i32(2, 16, 15, 17), vec4i32(1, 16, 16, 17),
  vec4i32(1, 16, 0, 1),   vec4i32(1, 15, 0, 2),   vec4i32(1, 14, 0, 3),   vec4i32(1, 13, 0, 4),
  vec4i32(1, 12, 0, 5),   vec4i32(1, 11, 0, 6),   vec4i32(1, 10, 0, 7),   vec4i32(1, 9, 0, 8),
  vec4i32(1, 8, 0, 9),    vec4i32(1, 7, 0, 10),   vec4i32(1, 6, 0, 11),   vec4i32(1, 5, 0, 12),
  vec4i32(1, 4, 0, 13),   vec4i32(1, 3, 0, 14),   vec4i32(1, 2, 0, 15),   vec4i32(1, 1, 0, 16),
  vec4i32(16, 16, 17, 1), vec4i32(16, 15, 17, 2), vec4i32(16, 14, 17, 3), vec4i32(16, 13, 17, 4),
  vec4i32(16, 12, 17, 5), vec4i32(16, 11, 17, 6), vec4i32(16, 10, 17, 7), vec4i32(16, 9, 17, 8),
  vec4i32(16, 8, 17, 9),  vec4i32(16, 7, 17, 10), vec4i32(16, 6, 17, 11), vec4i32(16, 5, 17, 12),
  vec4i32(16, 4, 17, 13), vec4i32(16, 3, 17, 14), vec4i32(16, 2, 17, 15), vec4i32(16, 1, 17, 16),
  vec4i32(16, 16, 0, 0),  vec4i32(1, 16, 17, 0),  vec4i32(16, 1, 0, 17),  vec4i32(1, 1, 17, 17)};

void copy_texel(
  vec2i32 probe_base_coord,
  u32 border_index,
  RWTexture2D<vec4f32> irradiance_probe_texture,
  RWTexture2D<vec4f32> depth_probe_texture)
{
  const vec2i32 src_coord = probe_base_coord + probe_border_offsets[border_index].xy;
  const vec2i32 dst_coord = probe_base_coord + probe_border_offsets[border_index].zw;

  irradiance_probe_texture[dst_coord] = irradiance_probe_texture[src_coord];
  depth_probe_texture[dst_coord]   = depth_probe_texture[src_coord];
}

[numthreads(PROBE_BORDER_UPDATE_WORK_GROUP_SIZE_X, PROBE_BORDER_UPDATE_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  const vec2i32 probe_base_coord =
    (vec2i32(sv.group_id.xy) * PROBE_OCT_SIZE_WITH_BORDER) + vec2i32(1, 1);

  RWTexture2D<vec4f32> irradiance_probe_texture =
    get_rw_texture_2d_vec4f32(push_constant.irradiance_probe_texture);
  RWTexture2D<vec4f32> depth_probe_texture =
    get_rw_texture_2d_vec4f32(push_constant.depth_probe_texture);

  copy_texel(probe_base_coord, sv.local_index, irradiance_probe_texture, depth_probe_texture);

  if (sv.local_index < 4)
  {
    copy_texel(
      probe_base_coord,
      (PROBE_BORDER_UPDATE_WORK_GROUP_COUNT) + sv.local_index,
      irradiance_probe_texture,
      depth_probe_texture);
  }
}
