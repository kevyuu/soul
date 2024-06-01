#include "render_nodes/common/sv.hlsl"
#include "render_nodes/rtao/rtao.shared.hlsl"

#include "utils/bnd_sampler.hlsl"
#include "utils/constant.hlsl"
#include "utils/math.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"

groupshared u32 g_ao;

[[vk::push_constant]] RayQueryPC push_constant;

vec2f32 texel_coord_to_uv(vec2u32 index, vec2u32 texture_size)
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

[numthreads(RAY_QUERY_WORK_GROUP_SIZE_X, RAY_QUERY_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{

  if (sv.local_index == 0)
  {
    g_ao = 0;
  }

  AllMemoryBarrierWithGroupSync();

  const GPUScene scene               = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  const GPUCameraData camera_data    = scene.camera_data;
  Texture2D depth_texture            = get_texture_2d(push_constant.depth_texture);
  Texture2D normal_roughness_texture = get_texture_2d(push_constant.normal_roughness_texture);

  const vec2i32 texel_coord = sv.global_id.xy;
  const vec2f32 uv          = texel_coord_to_uv(texel_coord, vec2u32(1920, 1080));
  const vec3i32 load_index  = uint3(texel_coord, 0);

  const vec4f32 normal_roughness  = normal_roughness_texture.Load(load_index);

  const f32 ndc_depth = depth_texture.Load(load_index).x;
  const vec3f32 n     = normal_roughness.xyz * 2.0 - 1.0;

  u32 result = 0;
  if (ndc_depth != 1.0f)
  {
    vec2f32 rng_val = next_sample(texel_coord);

    const mat4f32 inv_proj_view_mat = camera_data.inv_proj_view_mat;
    const vec3f32 world_position    = world_position_from_depth(uv, ndc_depth, inv_proj_view_mat);

    vec3f32 ray_direction = sample_hemisphere_cosine_w(n, rng_val);

    RayQuery<
      RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
      RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH>
      query;

    RayDesc ray_desc;
    ray_desc.Origin    = world_position + (n * push_constant.bias);
    ray_desc.Direction = ray_direction;
    ray_desc.TMin      = 0.001f;
    ray_desc.TMax      = push_constant.ray_max;

    query.TraceRayInline(scene.get_tlas(), 0, 0xff, ray_desc);

    query.Proceed();

    if (query.CommittedStatus() == COMMITTED_NOTHING)
    {
      result = 1;
    }
  }

  InterlockedOr(g_ao, result << sv.local_index);

  AllMemoryBarrierWithGroupSync();

  if (sv.local_index == 0)
  {
    RWTexture2D<u32> out_texture = get_rw_texture_2d_u32(push_constant.output_texture);
    out_texture[sv.group_id.xy]  = g_ao;
  }
}
