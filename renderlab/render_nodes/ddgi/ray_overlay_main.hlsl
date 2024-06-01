#include "render_nodes/ddgi/ddgi.shared.hlsl"
#include "scene.hlsl"

#include "math/matrix.hlsl"

[[vk::push_constant]] RayOverlayPC push_constant;

struct VsInput
{
  [[vk::location(0)]] vec3f32 position : position;
  u32 instance_id : SV_InstanceID;
};

struct VsOutput
{
  vec4f32 position : SV_POSITION;
  vec3f32 color : COLOR;
};

[shader("vertex")] VsOutput vs_main(VsInput input)
{
  const GPUScene scene              = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  Texture2D irradiance_texture      = get_texture_2d(push_constant.irradiance_texture);
  Texture2D ray_dir_dist_texture = get_texture_2d(push_constant.ray_dir_dist_texture);

  const u32 probe_index        = push_constant.probe_index;
  const u32 ray_index          = input.instance_id;
  const vec3f32 probe_position = push_constant.ddgi_volume.position_from_probe_index(probe_index);

  const vec2i32 pixel_id        = vec2i32(ray_index, probe_index);
  const vec3i32 load_id         = vec3i32(pixel_id, 0);
  const vec4f32 direction_depth = ray_dir_dist_texture.Load(load_id);
  const vec3f32 ray_direction   = direction_depth.xyz;

  const mat4f32 rotation_mat    = math::orthonormal_basis(ray_direction);
  const mat4f32 translation_mat = math::translate_identity(probe_position);
  const mat4f32 scale_mat       = math::scale_identity(vec3f32(0.004, 0.004, direction_depth.w));
  const mat4f32 model_mat       = mul(translation_mat, mul(rotation_mat, scale_mat));

  // make cube z start at 0 so, ray will shoot from probe center
  const vec3f32 vertex = input.position + vec3f32(0, 0, 0.5f);

  const vec3f32 world_position = mul(model_mat, vec4f32(vertex, 1)).xyz;

  VsOutput output;
  output.position = mul(scene.camera_data.proj_view_mat_no_jitter, vec4f32(world_position, 1.0f));
  output.color    = irradiance_texture.Load(load_id).xyz;
  return output;
}

struct PsOutput
{
  [[vk::location(0)]] vec4f32 color : SV_Target;
};

[shader("pixel")] PsOutput ps_main(VsOutput input)
{
  PsOutput output;
  output.color = vec4f32(vec3f32(1, 0, 0), 1);
  return output;
}
