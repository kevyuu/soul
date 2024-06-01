#include "render_nodes/common/edge_stopping.hlsl"
#include "render_nodes/shadow/shadow_type.hlsl"

struct ComputeSV
{
  vec3u32 global_id : SV_DispatchThreadID;
  vec3u32 local_id : SV_GroupThreadID;
  vec3u32 group_id : SV_GroupID;
  u32 local_index : SV_GroupIndex;
};

[[vk::push_constant]] FilterTilePC push_constant;

f32 compute_variance_center(vec2i32 texel_coord, Texture2D visibility_texture)
{
  f32 sum = 0.0f;

  const f32 kernel[2][2] = {{1.0 / 4.0, 1.0 / 8.0}, {1.0 / 8.0, 1.0 / 16.0}};

  const i32 radius = 1;
  for (i32 yy = -radius; yy <= radius; yy++)
  {
    for (i32 xx = -radius; xx <= radius; xx++)
    {
      vec2i32 sample_texel_coord = texel_coord + vec2i32(xx, yy);

      f32 k = kernel[abs(xx)][abs(yy)];

      sum += visibility_texture.Load(vec3i32(sample_texel_coord, 0)).g * k;
    }
  }

  return sum;
}

[numthreads(FILTER_TILE_WORK_GROUP_SIZE_X, FILTER_TILE_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  RWTexture2D<vec2f32> output_texture = get_rw_texture_2d_vec2f32(push_constant.output_texture);

  Texture2D visibility_texture       = get_texture_2d(push_constant.visibility_texture);
  Texture2D gbuffer_normal_roughness = get_texture_2d(push_constant.gbuffer_normal_roughness);
  Texture2D gbuffer_depth            = get_texture_2d(push_constant.gbuffer_depth);

  vec2i32 size = get_texture_dimension(gbuffer_depth);
  vec2u32 texel_coord =
    get_buffer<vec2u32>(push_constant.filter_coords_buffer, sv.group_id.x * sizeof(vec2u32)) +
    vec2u32(sv.local_id.xy);
  vec3u32 load_coord = vec3u32(texel_coord, 0);

  const f32 eps_variance      = 1e-10;
  const f32 kernel_weights[3] = {1.0, 2.0 / 3.0, 1.0 / 6.0};

  // constant samplers to prevent the compiler from generating code which
  // fetches the sampler descriptor from memory for each texture access
  const vec2f32 center_visibility = visibility_texture.Load(load_coord).rg;

  // variance for direct and indirect, filtered using 3x3 gaussin blur
  const f32 var = compute_variance_center(texel_coord, visibility_texture);

  vec3f32 center_normal = gbuffer_normal_roughness.Load(load_coord).xyz;
  f32 center_depth      = gbuffer_depth.Load(load_coord).r;

  if (center_depth < 0)
  {
    output_texture[texel_coord] = center_visibility;
    return;
  }

  const f32 phi_visibility = push_constant.phi_visibility * sqrt(max(0.0, eps_variance + var));

  // explicitly store/accumulate center pixel with weight 1 to prevent issues
  // with the edge-stopping functions
  f32 sum_w_visibility   = 1.0;
  vec2f32 sum_visibility = center_visibility;

  for (i32 yy = -push_constant.radius; yy <= push_constant.radius; yy++)
  {
    for (i32 xx = -push_constant.radius; xx <= push_constant.radius; xx++)
    {
      const vec2i32 sample_texel_coord                 = texel_coord + vec2i32(xx, yy) * push_constant.step_size;
      const vec3i32 sample_load_coord = vec3i32(sample_texel_coord, 0);
      const b8 inside                 = all(sample_texel_coord >= vec2i32(0, 0)) && all(sample_texel_coord < size);
      const f32 kernel                = kernel_weights[abs(xx)] * kernel_weights[abs(yy)];

      if (inside && (xx != 0 || yy != 0)) // skip center pixel, it is already accumulated
      {
        const vec2f32 sample_visibility = visibility_texture.Load(sample_load_coord).rg;
        vec3f32 sample_normal           = gbuffer_normal_roughness.Load(sample_load_coord).xyz;
        f32 sample_depth                = gbuffer_depth.Load(sample_load_coord).r;

        // compute the edge-stopping functions
        const f32 edge_stopping_weight = compute_edge_stopping_weight(
          center_depth,
          sample_depth,
          push_constant.sigma_depth,
          center_normal,
          sample_normal,
          push_constant.phi_normal,
          center_visibility.r,
          sample_visibility.r,
          phi_visibility);

        const f32 w_visibility = edge_stopping_weight * kernel;

        sum_w_visibility += w_visibility;
        sum_visibility += vec2f32(w_visibility, w_visibility * w_visibility) * sample_visibility;
      }
    }
  }

  // renormalization is different for variance, check paper for the formula
  vec2f32 out_visibility =
    sum_visibility / vec2f32(sum_w_visibility, sum_w_visibility * sum_w_visibility);

  // temporal integration
  if (push_constant.power != 0.0f)
  {
    out_visibility.x = pow(out_visibility.x, push_constant.power);
  }

  output_texture[texel_coord] = out_visibility;
}
