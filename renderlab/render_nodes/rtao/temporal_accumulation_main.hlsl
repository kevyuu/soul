#include "render_nodes/rtao/rtao.shared.hlsl"
#include "render_nodes/common/sv.hlsl"

#include "utils/math.hlsl"
#include "utils/reprojection.hlsl"

#include "scene.hlsl"

[[vk::push_constant]] TemporalAccumulationPC push_constant;

groupshared u32 g_ao_hit_masks[3][6];
groupshared f32 g_mean_accumulation[8][24];
groupshared u32 g_should_denoise;

void populate_cache(ComputeSV sv)
{
  Texture2D<u32> ray_query_result = get_texture_2d_u32(push_constant.ray_query_result_texture);
  vec2u32 texture_size            = get_texture_dimension(ray_query_result);

  if (sv.local_id.x < 3 && sv.local_id.y < 6)
  {
    vec2i32 coord =
      vec2i32(sv.group_id.x, sv.group_id.y * 2) - vec2i32(1, 2) + vec2i32(sv.local_id.xy);

    if (any(coord < vec2i32(0, 0)) || any(coord > (texture_size - vec2i32(1, 1))))
    {
      g_ao_hit_masks[sv.local_id.x][sv.local_id.y] = 0xFFFFFFFF;
    } else
    {
      g_ao_hit_masks[sv.local_id.x][sv.local_id.y] = ray_query_result.Load(vec3i32(coord, 0)).x;
    }
  }
}

f32 unpack_ao_hit_value(vec2i32 coord, ComputeSV sv)
{
  // Find the global coordinate for the top left corner of the current work group.
  const vec2i32 work_group_start_coord =
    vec2i32(sv.group_id.xy) *
    vec2i32(TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X, TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y);

  // Find the global coordinate for the top left corner of the cache.
  const vec2i32 cache_start_coord =
    work_group_start_coord - vec2i32(RAY_QUERY_WORK_GROUP_SIZE_X, RAY_QUERY_WORK_GROUP_SIZE_Y * 2);

  // Compute the local coordinate within the cache for the requested global coordinate.
  const vec2i32 unpacked_cache_coord = coord - cache_start_coord;

  // From the unpacked local coordinate, compute which ray mask the requested hit belongs to.
  // aka the packed local coordinate.
  const vec2i32 packed_cache_coord =
    unpacked_cache_coord / vec2i32(RAY_QUERY_WORK_GROUP_SIZE_X, RAY_QUERY_WORK_GROUP_SIZE_Y);

  // From the packed local coordinate, compute the unpacked local coordinate for the start of the
  // current ray mask.
  const vec2i32 mask_start_coord =
    packed_cache_coord * vec2i32(RAY_QUERY_WORK_GROUP_SIZE_X, RAY_QUERY_WORK_GROUP_SIZE_Y);

  // Find the relative coordinate of the requested sample within the ray mask.
  const vec2i32 relative_mask_coord = unpacked_cache_coord - mask_start_coord;

  // Compute the flattened hit index of the requested sample within the ray mask.
  const i32 hit_index = relative_mask_coord.y * RAY_QUERY_WORK_GROUP_SIZE_X + relative_mask_coord.x;

  // Use the hit index to bit shift the value from the cache and retrieve the requested sample.
  return f32((g_ao_hit_masks[packed_cache_coord.x][packed_cache_coord.y] >> hit_index) & 1u);
}

f32 horizontal_neighborhood_mean(vec2i32 coord, ComputeSV sv)
{
  f32 result = 0.0f;

  for (i32 x = -8; x <= 8; x++)
  {
    result += unpack_ao_hit_value(vec2i32(coord.x + x, coord.y), sv);
  }

  return result;
}

f32 neighborhood_mean(vec2i32 coord, ComputeSV sv)
{
  f32 top    = horizontal_neighborhood_mean(vec2i32(coord.x, coord.y - 8), sv);
  f32 middle = horizontal_neighborhood_mean(vec2i32(coord.x, coord.y), sv);
  f32 bottom = horizontal_neighborhood_mean(vec2i32(coord.x, coord.y + 8), sv);

  g_mean_accumulation[sv.local_id.x][sv.local_id.y]      = top;
  g_mean_accumulation[sv.local_id.x][sv.local_id.y + 8]  = middle;
  g_mean_accumulation[sv.local_id.x][sv.local_id.y + 16] = bottom;

  AllMemoryBarrierWithGroupSync();

  const i32 radius = 8;
  const f32 weight = (f32(radius) * 2.0f + 1.0f) * (f32(radius) * 2.0f + 1.0f);

  f32 mean = 0.0f;

  for (i32 y = 0; y <= 16; y++)
  {
    mean += g_mean_accumulation[sv.local_id.x][sv.local_id.y + y];
  }

  return mean / weight;
}

struct TemporalGBuffers
{
  Texture2D normal_roughness;
  Texture2D motion_curve;
  Texture2D<u32> meshid;
  Texture2D depth;
};

struct ReprojectResult
{
  f32 history_output;
  f32 history_length;
  b8 success;
};

ReprojectResult reproject(
  vec2i32 texel_coord,
  f32 depth,
  mat4f32 inv_proj_view_mat,
  mat4f32 prev_inv_proj_view_mat,
  TemporalGBuffers current_gbuffers,
  TemporalGBuffers prev_gbuffers,
  Texture2D history_output_texture,
  Texture2D history_length_texture)
{

  const vec2f32 image_dim    = vec2f32(get_texture_dimension(history_output_texture));
  const vec2f32 pixel_center = vec2f32(texel_coord) + vec2f32(0.5f, 0.5f);
  const vec2f32 uv     = pixel_center / vec2f32(image_dim);

  const vec4f32 center_normal_roughness =
    current_gbuffers.normal_roughness.Load(vec3i32(texel_coord, 0));
  const vec4f32 center_motion_curve =
    current_gbuffers.motion_curve.Load(vec3i32(texel_coord, 0));
  const u32 center_meshid = current_gbuffers.meshid.Load(vec3i32(texel_coord, 0));

  const vec2f32 current_motion = center_motion_curve.xy;
  const vec3f32 current_normal = center_normal_roughness.xyz * 2.0 - 1.0;
  const u32 current_mesh_id    = center_meshid;

  const vec3f32 current_pos = world_position_from_depth(uv, depth, inv_proj_view_mat);

  // +0.5 to account for texel center offset
  const vec2i32 history_coord =
    vec2i32(vec2f32(texel_coord) + current_motion.xy * image_dim + vec2f32(0.5f, 0.5f));
  const vec2f32 history_coord_floor = floor(texel_coord.xy) + current_motion.xy * image_dim;
  const vec2f32 history_uv    = uv + current_motion.xy;

  f32 history_output = 0.0f;
  f32 history_length = 0.0f;

  b8 v[4];
  const vec2i32 offset[4] = {vec2i32(0, 0), vec2i32(1, 0), vec2i32(0, 1), vec2i32(1, 1)};

  // check for all 4 taps of the bilinear filter for validity
  b8 valid = false;
  for (i32 sample_idx = 0; sample_idx < 4; sample_idx++)
  {
    vec2i32 loc = vec2i32(history_coord_floor) + offset[sample_idx];

    vec4f32 prev_normal_roughness = prev_gbuffers.normal_roughness.Load(vec3i32(loc, 0));
    u32 prev_meshid               = prev_gbuffers.meshid.Load(vec3i32(loc, 0));
    f32 prev_depth                = prev_gbuffers.depth.Load(vec3i32(loc, 0)).r;

    vec3f32 history_normal = prev_normal_roughness.xyz * 2.0 - 1.0;
    u32 history_mesh_id    = prev_meshid;

    vec3f32 history_pos =
      world_position_from_depth(history_uv, prev_depth, inv_proj_view_mat);

    v[sample_idx] = is_reprojection_valid(
      history_coord,
      current_pos,
      history_pos,
      current_normal,
      history_normal,
      current_mesh_id,
      history_mesh_id,
      vec2i32(image_dim));

    valid = valid || v[sample_idx];
  }

  if (valid)
  {
    f32 sumw = 0;
    f32 x    = frac(history_coord_floor.x);
    f32 y    = frac(history_coord_floor.y);

    // bilinear weights
    f32 w[4] = {(1 - x) * (1 - y), x * (1 - y), (1 - x) * y, x * y};

    history_output = 0.0f;

    // perform the actual bilinear interpolation
    for (i32 sample_idx = 0; sample_idx < 4; sample_idx++)
    {
      vec2i32 loc = vec2i32(history_coord_floor) + offset[sample_idx];

      if (v[sample_idx])
      {
        history_output += w[sample_idx] * history_output_texture.Load(vec3i32(loc, 0)).r;
        sumw += w[sample_idx];
      }
    }

    // redistribute weights in case not all taps were used
    valid          = (sumw >= 0.01);
    history_output = valid ? history_output / sumw : 0.0f;
  }
  if (!valid) // perform cross-bilateral filter in the hope to find some suitable samples somewhere
  {
    f32 cnt = 0.0;

    // this code performs a binary descision for each tap of the cross-bilateral filter
    const i32 radius = 1;
    for (i32 yy = -radius; yy <= radius; yy++)
    {
      for (i32 xx = -radius; xx <= radius; xx++)
      {
        vec2i32 p = history_coord + vec2i32(xx, yy);

        vec4f32 prev_normal_roughness = prev_gbuffers.normal_roughness.Load(vec3i32(p, 0));
        u32 history_mesh_id           = prev_gbuffers.meshid.Load(vec3i32(p, 0));
        f32 sample_depth              = prev_gbuffers.depth.Load(vec3i32(p, 0)).r;

        vec3f32 history_normal = prev_normal_roughness.xyz * 2.0 - 1.0;
        vec3f32 history_pos =
          world_position_from_depth(history_uv, sample_depth, inv_proj_view_mat);

        if (is_reprojection_valid(
              history_coord,
              current_pos,
              history_pos,
              current_normal,
              history_normal,
              current_mesh_id,
              history_mesh_id,
              vec2i32(image_dim)))
        {
          history_output += history_output_texture.Load(vec3i32(p, 0)).r;
          cnt += 1.0;
        }
      }
    }
    if (cnt > 0)
    {
      valid = true;
      history_output /= cnt;
    }
  }

  if (valid)
  {
    history_length = history_length_texture.Load(vec3i32(history_coord, 0)).r;
  } else
  {
    history_output = 0.0f;
    history_length = 0.0f;
  }

  ReprojectResult result;
  result.history_output = history_output;
  result.history_length = history_length;
  result.success        = valid;
  return result;
}

[numthreads(TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X, TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  const GPUScene scene = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);

  TemporalGBuffers current_gbuffers;
  current_gbuffers.normal_roughness =
    get_texture_2d(push_constant.current_normal_roughness_gbuffer);
  current_gbuffers.motion_curve = get_texture_2d(push_constant.current_motion_curve_gbuffer);
  current_gbuffers.meshid         = get_texture_2d_u32(push_constant.current_meshid_gbuffer);
  current_gbuffers.depth          = get_texture_2d(push_constant.current_depth_gbuffer);

  TemporalGBuffers prev_gbuffers;
  prev_gbuffers.normal_roughness = get_texture_2d(push_constant.prev_normal_roughness_gbuffer);
  prev_gbuffers.motion_curve   = get_texture_2d(push_constant.prev_motion_curve_gbuffer);
  prev_gbuffers.meshid           = get_texture_2d_u32(push_constant.prev_meshid_gbuffer);
  prev_gbuffers.depth            = get_texture_2d(push_constant.prev_depth_gbuffer);

  RWTexture2D<f32> output_val_texture = get_rw_texture_2d_f32(push_constant.output_val_texture);
  RWTexture2D<f32> output_history_length_texture =
    get_rw_texture_2d_f32(push_constant.output_history_length_texture);

  RWByteAddressBuffer filter_dispatch_arg = get_rw_buffer(push_constant.filter_dispatch_arg_buffer);
  RWByteAddressBuffer filter_coords_buffer = get_rw_buffer(push_constant.filter_coords_buffer);

  Texture2D prev_val_texture           = get_texture_2d(push_constant.prev_val_texture);
  Texture2D prev_history_length_texture = get_texture_2d(push_constant.prev_history_length_texture);

  vec2u32 texel_coord = sv.global_id.xy;

  if (sv.local_index == 0)
  {
    g_should_denoise = 0;
  }

  populate_cache(sv);

  AllMemoryBarrierWithGroupSync();

  f32 mean = neighborhood_mean(texel_coord, sv);

  f32 depth = current_gbuffers.depth.Load(vec3u32(texel_coord, 0)).r;

  f32 out_ao         = 1.0f;
  f32 history_length = 0.0f;
  f32 history_ao     = 1.0f;

  if (depth != 1.0f)
  {
    f32 ao = unpack_ao_hit_value(texel_coord, sv);

    ReprojectResult result = reproject(
      texel_coord,
      depth,
      scene.camera_data.inv_proj_view_mat,
      scene.prev_camera_data.inv_proj_view_mat,
      current_gbuffers,
      prev_gbuffers,
      prev_val_texture,
      prev_history_length_texture);

    history_length = min(32.0f, result.success ? result.history_length + 1.0f : 1.0f);
    history_ao = result.history_output;

    if (result.success)
    {
      f32 spatial_variance = mean;
      spatial_variance     = max(spatial_variance - mean * mean, 0.0f);

      // Compute the clamping bounding box
      const f32 std_deviation = sqrt(spatial_variance);
      const f32 nmin          = mean - 0.5f * std_deviation;
      const f32 nmax          = mean + 0.5f * std_deviation;

      history_ao = clamp(history_ao, nmin, nmax);
    }

    const float alpha = result.success ? max(push_constant.alpha, 1.0 / history_length) : 1.0;

    out_ao = lerp(history_ao, ao, alpha);
  }

  output_history_length_texture[texel_coord] = history_length;
  output_val_texture[texel_coord]            = out_ao;

  // If all the threads are in shadow, skip the A-Trous filter.
  if (out_ao <= 1.0f)
  {
    g_should_denoise = 1;
  }

  AllMemoryBarrierWithGroupSync();

  if (sv.local_index == 0 && g_should_denoise == 1)
  {
    u32 original_index;
    filter_dispatch_arg.InterlockedAdd(0, 1, original_index);
    filter_coords_buffer.Store(original_index * sizeof(vec2u32), texel_coord);
  }
}
