#include "render_nodes/shadow/shadow_type.hlsl"

#include "render_nodes/common/lighting.hlsl"
#include "render_nodes/common/ray_query.hlsl"

#include "utils/bnd_sampler.hlsl"
#include "utils/math.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"

groupshared u32 group_visibility;

[[vk::push_constant]] ShadowPushConstant push_constant;

vec2f32 TexelIndexToUV(vec2u32 index, vec2u32 texture_size)
{
  return vec2f32(index + 0.5) / texture_size;
}

vec2f32 next_sample(vec2i32 coord)
{
  Texture2D sobol_tex              = get_texture_2d(push_constant.sobol_texture);
  Texture2D scrambling_ranking_tex = get_texture_2d(push_constant.scrambling_ranking_texture);

  return vec2f32(
    sample_blue_noise(coord, i32(push_constant.num_frames), 0, sobol_tex, scrambling_ranking_tex),
    sample_blue_noise(coord, i32(push_constant.num_frames), 1, sobol_tex, scrambling_ranking_tex));
}

[numthreads(RAY_QUERY_WORK_GROUP_SIZE_X, RAY_QUERY_WORK_GROUP_SIZE_Y, 1)] void cs_main(
  vec3u32 dispatch_thread_id
  : SV_DispatchThreadID, vec3u32 group_id
  : SV_GroupID, u32 group_index
  : SV_GroupIndex)
{
  if (group_index == 0)
  {
    group_visibility = 0;
  }

  AllMemoryBarrierWithGroupSync();

  const GPUScene scene               = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  const GPUCameraData camera_data    = scene.camera_data;
  Texture2D depth_texture            = get_texture_2d(push_constant.depth_texture);
  Texture2D normal_roughness_texture = get_texture_2d(push_constant.normal_roughness_texture);

  const vec2i32 texel_coord = dispatch_thread_id.xy;
  const vec2f32 uv          = TexelIndexToUV(texel_coord, vec2u32(1920, 1080));
  const vec3i32 load_index  = uint3(texel_coord, 0);

  const vec4f32 normal_roughness = normal_roughness_texture.Load(load_index);

  const f32 ndc_depth = depth_texture.Load(load_index).x;
  const vec3f32 n     = normal_roughness.xyz * 2.0 - 1.0;

  const mat4f32 inv_proj_view_mat = camera_data.inv_proj_view_mat;
  const vec3f32 world_position    = world_position_from_depth(uv, ndc_depth, inv_proj_view_mat);

  u32 result = 0;
  if (ndc_depth != 1.0f && scene.light_count > 0)
  {
    const GPULightInstance light_instance = scene.get_light_instance(u32(0));
    vec2f32 rng_val                       = next_sample(texel_coord);
    LightRaySample ray_sample = sample_sun_light(light_instance, world_position, rng_val);
    result =
      query_visibility(scene.get_tlas(), world_position + n * 0.05f, ray_sample.direction, ray_sample.distance);
  }

  InterlockedOr(group_visibility, result << group_index);

  AllMemoryBarrierWithGroupSync();

  if (group_index == 0)
  {
    RWTexture2D<u32> out_texture = get_rw_texture_2d_u32(push_constant.output_texture);
    out_texture[group_id.xy]     = group_visibility;
  }
}
