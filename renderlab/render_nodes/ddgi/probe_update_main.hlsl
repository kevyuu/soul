#include "render_nodes/common/color.hlsl"
#include "render_nodes/common/sv.hlsl"

#include "render_nodes/ddgi/ddgi.shared.hlsl"

#define CACHE_SIZE 64

groupshared vec4f32 g_dir_dist[CACHE_SIZE];
groupshared vec3f32 g_radiance[CACHE_SIZE];

[[vk::push_constant]] ProbeUpdatePC push_constant;

void populate_cache(i32 probe_index, u32 ray_offset, u32 num_rays, ComputeSV sv)
{
  Texture2D ray_dir_dist_texture = get_texture_2d(push_constant.ray_dir_dist_texture);
  Texture2D ray_radiance_texture = get_texture_2d(push_constant.ray_radiance_texture);
  if (sv.local_index < num_rays)
  {
    vec2i32 trace_map_texel_id = vec2i32(ray_offset + u32(sv.local_index), probe_index);
    vec3i32 trace_map_load_id  = vec3i32(trace_map_texel_id, 0);
    g_dir_dist[sv.local_index] = ray_dir_dist_texture.Load(trace_map_load_id);
    g_radiance[sv.local_index] = ray_radiance_texture.Load(trace_map_load_id).xyz;
  }
}

struct GatherResult
{
  vec3f32 total_irradiance;
  f32 total_irradiance_weight;
  vec2f32 total_depth_moment;
  f32 total_depth_moment_weight;
};

void gather_rays(vec2i32 probe_map_texel_id, u32 num_rays, inout GatherResult result)
{
  const f32 energy_conservation = 0.95f;
  const DdgiVolume ddgi               = push_constant.ddgi_volume;

  // For each ray
  for (i32 ray_index = 0; ray_index < num_rays; ++ray_index)
  {
    const vec4f32 ray_dir_dist = g_dir_dist[ray_index];
    const vec3f32 ray_dir      = ray_dir_dist.xyz;
    f32 ray_dist         = min(ddgi.max_depth, ray_dir_dist.w - 0.01f);

    // Detect misses and force depth
    if (ray_dist == -1.0f)
    {
      ray_dist = ddgi.max_depth;
    }

    const vec3f32 ray_irradiance = g_radiance[ray_index] * energy_conservation;

    const vec3f32 texel_direction =
      ndir_from_oct_snorm(ddgi.oct_snorm_from_probe_map_texel_id(probe_map_texel_id));

    const f32 depth_moment_weight =
      pow(max(0.0, dot(texel_direction, ray_dir)), ddgi.depth_sharpness);
    const f32 irradiance_weight = max(0.0, dot(texel_direction, ray_dir));

    if (depth_moment_weight >= FLT_EPSILON)
    {
      result.total_depth_moment +=
        vec2f32(ray_dist * depth_moment_weight, square(ray_dist) * depth_moment_weight);
      result.total_depth_moment_weight += depth_moment_weight;
    }

    result.total_irradiance += vec3f32(ray_irradiance * irradiance_weight);
    result.total_irradiance_weight += irradiance_weight;
  }
}

vec3f32 tonemap(vec3f32 x)
{
  return x / (x + vec3f32(1.0f, 1.0f, 1.0f));
}

vec3f32 inverse_tonemap(vec3f32 x)
{
  vec3f32 divisor =
    max(vec3f32(1.0f, 1.0f, 1.0f) - x, vec3f32(FLT_EPSILON, FLT_EPSILON, FLT_EPSILON));
  return x / divisor;
}

vec3f32 mix_irradiance(vec3f32 current_irradiance, vec3f32 history_irradiance)
{
  // return lerp(current_irradiance, history_irradiance, push_constant.ddgi_volume.hysteresis);
  
  vec3f32 percetual_irradiance = pow(current_irradiance, 1 / GAMMA_IRRADIANCE);
  vec3f32 result_irradiance = lerp(percetual_irradiance, history_irradiance, push_constant.ddgi_volume.hysteresis);
  return result_irradiance;
}

[numthreads(PROBE_UPDATE_WORK_GROUP_SIZE_X, PROBE_UPDATE_WORK_GROUP_SIZE_Y, 1)] void cs_main(
  ComputeSV sv)
{
  const vec2i32 probe_map_texel_id =
    vec2i32(sv.global_id.xy) + (vec2i32(sv.group_id.xy) * vec2i32(2, 2)) + vec2i32(2, 2);
  const vec3i32 probe_map_load_id = vec3i32(probe_map_texel_id, 0);

  const DdgiVolume ddgi = push_constant.ddgi_volume;
  const i32 probe_index = ddgi.probe_index_from_probe_map_coord(probe_map_texel_id);

  GatherResult result;
  result.total_irradiance          = 0.0f;
  result.total_irradiance_weight   = 0.0f;
  result.total_depth_moment        = vec2f32(0, 0);
  result.total_depth_moment_weight = 0.0f;

  u32 remaining_rays = push_constant.ddgi_volume.rays_per_probe;
  u32 ray_offset     = 0;

  while (remaining_rays > 0)
  {
    u32 num_rays = min(CACHE_SIZE, remaining_rays);

    populate_cache(probe_index, ray_offset, num_rays, sv);

    AllMemoryBarrierWithGroupSync();

    gather_rays(probe_map_texel_id, num_rays, result);

    AllMemoryBarrierWithGroupSync();

    remaining_rays -= num_rays;
    ray_offset += num_rays;
  }

  vec3f32 irradiance_result = result.total_irradiance;
  if (result.total_irradiance_weight > FLT_EPSILON)
  {
    irradiance_result /= result.total_irradiance_weight;
  }

  vec2f32 depth_moment_result = result.total_depth_moment;
  if (result.total_depth_moment_weight > FLT_EPSILON)
  {
    depth_moment_result /= result.total_depth_moment_weight;
  }

  Texture2D history_irradiance_probe_texture =
    get_texture_2d(push_constant.history_irradiance_probe_texture);
  Texture2D history_depth_probe_texture = get_texture_2d(push_constant.history_depth_probe_texture);

  const vec3f32 history_irradiance =
    history_irradiance_probe_texture.Load(probe_map_load_id).xyz;
  const vec2f32 history_depth_moment = history_depth_probe_texture.Load(probe_map_load_id).xy;

  if (push_constant.frame_counter != 0)
  {
    irradiance_result   = mix_irradiance(irradiance_result, history_irradiance);
    depth_moment_result = lerp(depth_moment_result, history_depth_moment, ddgi.hysteresis);
  }

  RWTexture2D<vec4f32> irradiance_probe_texture =
    get_rw_texture_2d_vec4f32(push_constant.irradiance_probe_texture);
  RWTexture2D<vec2f32> depth_probe_texture =
    get_rw_texture_2d_vec2f32(push_constant.depth_probe_texture);

  irradiance_probe_texture[probe_map_texel_id] = vec4f32(irradiance_result, 0.0f);
  depth_probe_texture[probe_map_texel_id]      = depth_moment_result;
}
