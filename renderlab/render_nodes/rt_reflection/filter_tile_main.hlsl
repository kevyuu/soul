#include "utils/math.hlsl"

#include "render_nodes/common/edge_stopping.hlsl"
#include "render_nodes/common/sv.hlsl"
#include "render_nodes/rt_reflection/rt_reflection.shared.hlsl"

[[vk::push_constant]] FilterTilePC push_constant;

f32 compute_variance_center(vec2i32 texel_coord, Texture2D color_texture)
{
  f32 sum = 0.0f;

  const f32 kernel[2][2] = {{1.0 / 4.0, 1.0 / 8.0}, {1.0 / 8.0, 1.0 / 16.0}};

  const i32 radius = 1;
  for (i32 yy = -radius; yy <= radius; yy++)
  {
    for (i32 xx = -radius; xx <= radius; xx++)
    {
      vec2i32 p = texel_coord + vec2i32(xx, yy);

      f32 k = kernel[abs(xx)][abs(yy)];

      sum += color_texture.Load(vec3i32(p, 0)).w * k;
    }
  }

  return sum;
}

[numthreads(FILTER_TILE_WORK_GROUP_SIZE_X, FILTER_TILE_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  RWTexture2D<vec4f32> output_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  Texture2D color_texture            = get_texture_2d(push_constant.color_texture);
  Texture2D gbuffer_normal_roughness = get_texture_2d(push_constant.gbuffer_normal_roughness);
  Texture2D gbuffer_depth            = get_texture_2d(push_constant.gbuffer_depth);

  vec2i32 size = get_texture_dimension(gbuffer_depth);
  vec2u32 texel_coord =
    get_buffer<vec2u32>(push_constant.filter_coords_buffer, sv.group_id.x * sizeof(vec2u32)) +
    vec2u32(sv.local_id.xy);
  vec3u32 load_coord = vec3u32(texel_coord, 0);


  // constant samplers to prevent the compiler from generating code which
  // fetches the sampler descriptor from memory for each texture access
  const vec4f32 center_color  = color_texture.Load(load_coord);
  const f32 center_color_luma = luminance(center_color.xyz);

  vec3f32 center_normal = gbuffer_normal_roughness.Load(load_coord).xyz;
  f32 center_depth      = gbuffer_depth.Load(load_coord).r;

  if (center_depth < 0)
  {
    output_texture[texel_coord] = center_color;
    return;
  }

  const f32 eps_variance      = 1e-10;
  const f32 kernel_weights[3] = {1.0, 2.0 / 3.0, 1.0 / 6.0};

  // variance for direct and indirect, filtered using 3x3 gaussin blur
  const f32 var = compute_variance_center(texel_coord, color_texture);
  const f32 phi_color = push_constant.phi_color * sqrt(max(0.0, eps_variance + var));

  // explicitly store/accumulate center pixel with weight 1 to prevent issues
  // with the edge-stopping functions
  f32 sum_weight   = 1.0;
  vec4f32 sum_color = center_color;

  for (i32 yy = -push_constant.radius; yy <= push_constant.radius; yy++)
  {
    for (i32 xx = -push_constant.radius; xx <= push_constant.radius; xx++)
    {
      const vec2i32 p                 = texel_coord + vec2i32(xx, yy) * push_constant.step_size;
      const vec3i32 sample_load_coord = vec3i32(p, 0);
      const b8 inside                 = all(p >= vec2i32(0, 0)) && all(p < size);
      const f32 kernel                = kernel_weights[abs(xx)] * kernel_weights[abs(yy)];

      if (inside && (xx != 0 || yy != 0)) // skip center pixel, it is already accumulated
      {
        const vec4f32 sample_color  = color_texture.Load(sample_load_coord);
        const f32 sample_color_luma = luminance(sample_color.xyz);

        const vec3f32 sample_normal = gbuffer_normal_roughness.Load(sample_load_coord).xyz;
        const f32 sample_depth      = gbuffer_depth.Load(sample_load_coord).r;

        // compute the edge-stopping functions
        const f32 edge_stopping_weight = compute_edge_stopping_weight(
          center_depth,
          sample_depth,
          push_constant.sigma_depth,
          center_normal,
          sample_normal,
          push_constant.phi_normal,
          center_color_luma,
          sample_color_luma,
          phi_color);

        const f32 weight = edge_stopping_weight * kernel;

        sum_weight += weight;
        sum_color +=
          vec4f32(weight, weight, weight, weight * weight) * sample_color;
      }
    }
  }

  // renormalization is different for variance, check paper for the formula
  vec4f32 out_color =
    sum_color / vec4f32(sum_weight, sum_weight, sum_weight, sum_weight * sum_weight);

  output_texture[texel_coord] = out_color;
}
