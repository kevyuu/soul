#include "render_nodes/common/sv.hlsl"
#include "render_nodes/ddgi/ddgi.shared.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"
#include "utils/constant.hlsl"
#include "utils/math.hlsl"

vec2f32 texel_coord_to_uv(vec2u32 index, vec2u32 texture_size)
{
  return vec2f32(index + 0.5) / texture_size;
}

[[vk::push_constant]] SampleIrradiancePC push_constant;

[numthreads(SAMPLE_IRRADIANCE_WORK_GROUP_SIZE_X, SAMPLE_IRRADIANCE_WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  const GPUScene scene            = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  const GPUCameraData camera_data = scene.camera_data;
  const mat4f32 inv_proj_view_mat = camera_data.inv_proj_view_mat;

  SamplerState linear_clamp_sampler = scene.get_linear_clamp_sampler();

  Texture2D depth_texture            = get_texture_2d(push_constant.depth_texture);
  Texture2D normal_roughness_texture = get_texture_2d(push_constant.normal_roughness_texture);
  Texture2D irradiance_probe_texture = get_texture_2d(push_constant.irradiance_probe_texture);
  Texture2D depth_probe_texture      = get_texture_2d(push_constant.depth_probe_texture);

  RWTexture2D<vec4f32> out_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  const vec2i32 texel_coord = sv.global_id.xy;
  const vec2f32 uv          = texel_coord_to_uv(texel_coord, vec2u32(1920, 1080));
  const vec3i32 load_index  = vec3u32(texel_coord, 0);

  const vec4f32 normal_roughness = normal_roughness_texture.Load(load_index);

  const f32 ndc_depth        = depth_texture.Load(load_index).x;
  const vec3f32 world_normal = normal_roughness.xyz * 2.0 - 1.0;

  const vec3f32 world_position = world_position_from_depth(uv, ndc_depth, inv_proj_view_mat);
  const vec3f32 wo             = normalize(camera_data.position - world_position);

  vec3f32 irradiance = push_constant.ddgi_volume.sample_irradiance(
    world_position,
    world_normal,
    wo,
    linear_clamp_sampler,
    irradiance_probe_texture,
    depth_probe_texture);

  out_texture[texel_coord] = vec4f32(irradiance, 1.0f);
}
