#include "render_nodes/common/edge_stopping.hlsl"
#include "render_nodes/common/sv.hlsl"
#include "render_nodes/rtao/rtao.shared.hlsl"

#include "utils/math.hlsl"

#define GAUSS_BLUR_DEVIATION 1.5

[[vk::push_constant]] BilateralBlurPC push_constant;

[numthreads(BILATERAL_BLUR_WORK_GROUP_SIZE_X, BILATERAL_BLUR_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  RWTexture2D<vec2f32> output_texture = get_rw_texture_2d_vec2f32(push_constant.output_texture);

  Texture2D gbuffer_normal_roughness = get_texture_2d(push_constant.gbuffer_normal_roughness);
  Texture2D gbuffer_depth            = get_texture_2d(push_constant.gbuffer_depth);
  Texture2D ao_input_texture         = get_texture_2d(push_constant.ao_input_texture);

  vec2i32 size = get_texture_dimension(ao_input_texture);
  vec2u32 texel_coord =
    get_buffer<vec2u32>(push_constant.filter_coords_buffer, sv.group_id.x * sizeof(vec2u32)) +
    vec2u32(sv.local_id.xy);
  vec3u32 load_coord = vec3u32(texel_coord, 0);

  f32 depth = gbuffer_depth.Load(load_coord).r;
  if (depth == 1.0f)
  {
    output_texture[texel_coord] = vec2f32(1.0, 1.0);
    return;
  }

  const f32 deviation = f32(push_constant.radius) / GAUSS_BLUR_DEVIATION;

  f32 ao           = ao_input_texture.Load(load_coord).r;
  f32 total_ao     = ao_input_texture.Load(load_coord).r;
  f32 total_weight = 1.0f;

  f32 center_depth      = gbuffer_depth.Load(load_coord).r;
  vec3f32 center_normal = gbuffer_normal_roughness.Load(load_coord).rgb * 2.0 - 1.0;

  i32 radius = push_constant.radius;

  for (i32 i = -radius; i <= radius; i++)
  {
    if (i == 0)
    {
      continue;
    }

    vec2i32 sample_coord      = texel_coord + push_constant.direction * vec2i32(i, i);
    vec3i32 sample_load_coord = vec3i32(sample_coord, 0);
    f32 sample_depth          = gbuffer_depth.Load(sample_load_coord).r;
    f32 sample_ao             = ao_input_texture.Load(sample_load_coord).r;
    vec3f32 sample_normal     = gbuffer_normal_roughness.Load(sample_load_coord).rgb * 2.0 - 1.0;

    f32 weight = gaussian_weight(f32(i), deviation);

    weight *= compute_edge_stopping_weight(
      center_depth, sample_depth, 1.0f, center_normal, sample_normal, 32.0f, ao, sample_ao, 10.0f);

    total_ao += weight * sample_ao;
    total_weight += weight;
  }

  f32 out_ao = total_ao / max(total_weight, 0.0001f);

  output_texture[texel_coord] = out_ao;
}
