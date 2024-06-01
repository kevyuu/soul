#include "render_nodes/common/sv.hlsl"
#include "render_nodes/common/misc.hlsl"
#include "render_nodes/rt_reflection/rt_reflection.shared.hlsl"

#include "utils/math.hlsl"
#include "utils/reprojection.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"

[[vk::push_constant]] TemporalAccumulationPC push_constant;

groupshared u32 g_should_denoise;


struct NeighborhoodStatistic
{
  vec3f32 mean;
  vec3f32 std_dev;
};

NeighborhoodStatistic calculate_neighborhood_statistic(vec2i32 coord, ComputeSV sv)
{
  NeighborhoodStatistic statistic;
  Texture2D ray_trace_result_texture = get_texture_2d(push_constant.ray_trace_result_texture);

  vec3f32 m1 = 0.0f;
  vec3f32 m2 = 0.0f;

  i32 radius = 8;
  f32 weight = (f32(radius) * 2.0f + 1.0f) * (f32(radius) * 2.0f + 1.0f);

  for (i32 dx = -radius; dx <= radius; dx++)
  {
    for (i32 dy = -radius; dy <= radius; dy++)
    {
      vec2i32 sample_coord = coord + vec2i32(dx, dy);
      vec3f32 sample_color = ray_trace_result_texture.Load(vec3u32(sample_coord, 0)).xyz;

      m1 += sample_color;
      m2 += sample_color * sample_color;
    }
  }

  vec3f32 mean     = m1 / weight;
  vec3f32 variance = (m2 / weight) - (mean * mean);

  statistic.mean    = mean;
  statistic.std_dev = sqrt(max(variance, 0.01f));

  return statistic;
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
  vec3f32 history_output;
  vec2f32 history_moments;
  f32 history_length;
  b8 success;
};

ReprojectResult reproject(
  vec2i32 texel_coord,
  f32 depth,
  vec3f32 camera_pos,
  mat4f32 inv_proj_view_mat,
  mat4f32 prev_proj_view_mat,
  f32 ray_length,
  TemporalGBuffers current_gbuffers,
  TemporalGBuffers prev_gbuffers,
  Texture2D history_color_variance_texture,
  Texture2D history_moments_texture)
{
  const vec2f32 image_dim    = vec2f32(get_texture_dimension(history_color_variance_texture));
  const vec2f32 pixel_center = vec2f32(texel_coord) + vec2f32(0.5f, 0.5f);
  const vec2f32 uv           = pixel_center / vec2f32(image_dim);

  const vec4f32 center_normal_roughness =
    current_gbuffers.normal_roughness.Load(vec3i32(texel_coord, 0));
  const vec4f32 center_motion_curve = current_gbuffers.motion_curve.Load(vec3i32(texel_coord, 0));
  const u32 center_meshid           = current_gbuffers.meshid.Load(vec3i32(texel_coord, 0));

  const vec2f32 current_motion = center_motion_curve.xy;
  const vec3f32 current_normal = center_normal_roughness.xyz * 2.0 - 1.0;
  const u32 current_mesh_id    = center_meshid;

  const vec3f32 current_pos = world_position_from_depth(uv, depth, inv_proj_view_mat);

  const f32 curvature             = center_motion_curve.z;
  const vec2f32 history_uv        = uv + current_motion;
  const vec2f32 reprojected_coord = compute_history_coord(
    texel_coord,
    vec2i32(image_dim),
    depth,
    current_motion,
    curvature,
    ray_length,
    camera_pos,
    inv_proj_view_mat,
    prev_proj_view_mat);
  const vec2i32 history_coord       = vec2i32(reprojected_coord);
  const vec2f32 history_coord_floor = reprojected_coord;

  vec3f32 history_output  = 0.0f;
  vec2f32 history_moments = 0.0f;
  f32 history_length      = 0.0f;

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

    vec3f32 history_pos = world_position_from_depth(history_uv, prev_depth, inv_proj_view_mat);

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

    history_output  = 0.0f;
    history_moments = vec2f32(0.0f, 0.0f);

    // perform the actual bilinear interpolation
    for (i32 sample_idx = 0; sample_idx < 4; sample_idx++)
    {
      vec2i32 loc = vec2i32(history_coord_floor) + offset[sample_idx];

      if (v[sample_idx])
      {
        history_output += w[sample_idx] * history_color_variance_texture.Load(vec3i32(loc, 0)).xyz;
        history_moments += w[sample_idx] * history_moments_texture.Load(vec3i32(loc, 0)).xy;
        sumw += w[sample_idx];
      }
    }

    // redistribute weights in case not all taps were used
    valid           = (sumw >= 0.01);
    history_output  = valid ? history_output / sumw : 0.0f;
    history_moments = valid ? history_moments / sumw : vec2f32(0.0f, 0.0f);
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

        vec3f32 history_normal = prev_normal_roughness.xyz;
        vec3f32 sample_pos = world_position_from_depth(history_uv, sample_depth, inv_proj_view_mat);

        if (is_reprojection_valid(
              history_coord,
              current_pos,
              sample_pos,
              current_normal,
              history_normal,
              current_mesh_id,
              history_mesh_id,
              vec2i32(image_dim)))
        {
          history_output += history_color_variance_texture.Load(vec3i32(p, 0)).xyz;
          history_moments += history_moments_texture.Load(vec3i32(p, 0)).xy;
          cnt += 1.0f;
        }
      }
    }
    if (cnt > 0.0f)
    {
      valid = true;
      history_output /= cnt;
      history_moments /= cnt;
    }
  }

  if (valid)
  {
    history_length = history_moments_texture.Load(vec3i32(history_coord, 0)).z;
  } else
  {
    history_output  = 0.0f;
    history_moments = vec2f32(0.0f, 0.0f);
    history_length  = 0.0f;
  }

  ReprojectResult result;
  result.history_output  = history_output;
  result.history_moments = history_moments;
  result.history_length  = history_length;
  result.success         = valid;
  return result;
}

[numthreads(TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X, TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y, 1)] void cs_main(
  ComputeSV sv)
{
  const GPUScene scene                 = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  const GPUCameraData camera_data      = scene.camera_data;
  const GPUCameraData prev_camera_data = scene.prev_camera_data;

  Texture2D ray_trace_result_texture = get_texture_2d(push_constant.ray_trace_result_texture);

  TemporalGBuffers current_gbuffers;
  current_gbuffers.normal_roughness =
    get_texture_2d(push_constant.current_normal_roughness_gbuffer);
  current_gbuffers.motion_curve = get_texture_2d(push_constant.current_motion_curve_gbuffer);
  current_gbuffers.meshid       = get_texture_2d_u32(push_constant.current_meshid_gbuffer);
  current_gbuffers.depth        = get_texture_2d(push_constant.current_depth_gbuffer);

  TemporalGBuffers prev_gbuffers;
  prev_gbuffers.normal_roughness = get_texture_2d(push_constant.prev_normal_roughness_gbuffer);
  prev_gbuffers.motion_curve     = get_texture_2d(push_constant.prev_motion_curve_gbuffer);
  prev_gbuffers.meshid           = get_texture_2d_u32(push_constant.prev_meshid_gbuffer);
  prev_gbuffers.depth            = get_texture_2d(push_constant.prev_depth_gbuffer);

  RWTexture2D<vec4f32> output_color_variance_texture =
    get_rw_texture_2d_vec4f32(push_constant.output_color_variance_texture);
  RWTexture2D<vec4f32> output_moments_texture =
    get_rw_texture_2d_vec4f32(push_constant.output_moments_texture);

  Texture2D prev_color_variance_texture = get_texture_2d(push_constant.prev_color_variance_texture);
  Texture2D prev_moments_texture        = get_texture_2d(push_constant.prev_moments_texture);

  RWByteAddressBuffer filter_dispatch_arg = get_rw_buffer(push_constant.filter_dispatch_arg_buffer);
  RWByteAddressBuffer copy_dispatch_arg   = get_rw_buffer(push_constant.copy_dispatch_arg_buffer);
  RWByteAddressBuffer filter_coords_buffer = get_rw_buffer(push_constant.filter_coords_buffer);
  RWByteAddressBuffer copy_coords_buffer   = get_rw_buffer(push_constant.copy_coords_buffer);

  vec2u32 texel_coord = sv.global_id.xy;

  g_should_denoise = 0;

  AllMemoryBarrierWithGroupSync();

  f32 depth = current_gbuffers.depth.Load(vec3u32(texel_coord, 0)).r;

  vec4f32 output_color_variance = 0.0f;
  vec4f32 output_moments        = 0.0f;
  f32 history_length            = 0.0f;

  if (depth != 1.0f)
  {
    vec4f32 color_ray_length = ray_trace_result_texture.Load(vec3u32(texel_coord, 0));
    vec3f32 color            = color_ray_length.xyz;
    f32 ray_length           = color_ray_length.w;

    ReprojectResult result = reproject(
      texel_coord,
      depth,
      camera_data.position,
      camera_data.inv_proj_view_mat,
      prev_camera_data.proj_view_mat,
      ray_length,
      current_gbuffers,
      prev_gbuffers,
      prev_color_variance_texture,
      prev_moments_texture);

    history_length        = min(32.0f, result.success ? result.history_length + 1.0f : 1.0f);
    vec3f32 history_color = result.history_output;

    if (result.success)
    {
      const NeighborhoodStatistic statistic = calculate_neighborhood_statistic(texel_coord, sv);

      const vec3f32 radiance_min = statistic.mean - statistic.std_dev;
      const vec3f32 radiance_max = statistic.mean + statistic.std_dev;

      history_color = clip_aabb(radiance_min, radiance_max, history_color.xyz);
      history_color.x = isnan(history_color.x) ? color.x : history_color.x;
      history_color.y = isnan(history_color.y) ? color.y : history_color.y;
      history_color.z = isnan(history_color.z) ? color.z : history_color.z;
    }

    const f32 alpha = result.success ? max(push_constant.alpha, 1.0f / history_length) : 1.0;
    const f32 alpha_moments =
      result.success ? max(push_constant.moments_alpha, 1.0f / history_length) : 1.0;

    // compute first two moments of luminance
    vec2f32 moments = 0.0f;
    moments.r       = luminance(color);
    moments.g       = moments.r * moments.r;

    // temporal integration of the moments
    moments = lerp(result.history_moments, moments, alpha_moments);

    f32 variance = max(0.0f, moments.g - moments.r * moments.r);

    // temporal integration of radiance
    vec3f32 accumulated_color = lerp(history_color, color, alpha);

    output_moments        = vec4f32(moments.xy, history_length, 0.0f);
    output_color_variance = vec4f32(accumulated_color, variance);
  }

  output_moments_texture[texel_coord]        = output_moments;
  output_color_variance_texture[texel_coord] = output_color_variance;

  const vec4f32 normal_roughness =
    current_gbuffers.normal_roughness.Load(vec3i32(texel_coord, 0));
  if (depth != 1.0f)
  {
    g_should_denoise = 1;
  }

  AllMemoryBarrierWithGroupSync();

  if (sv.local_index == 0)
  {
    if (g_should_denoise == 1)
    {
      u32 original_index;
      filter_dispatch_arg.InterlockedAdd(0, 1, original_index);
      filter_coords_buffer.Store(original_index * sizeof(vec2u32), texel_coord);
    } else
    {
      u32 original_index;
      copy_dispatch_arg.InterlockedAdd(0, 1, original_index);
      copy_coords_buffer.Store(original_index * sizeof(vec2u32), texel_coord);
    }
  }
}
