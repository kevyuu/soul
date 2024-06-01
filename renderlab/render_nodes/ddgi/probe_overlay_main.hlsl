#include "render_nodes/ddgi/ddgi.shared.hlsl"
#include "scene.hlsl"

#include "math/matrix.hlsl"

[[vk::push_constant]] ProbeOverlayPC push_constant;

struct VsInput
{
  [[vk::location(0)]] vec2f32 position : position;
  u32 instance_id : SV_InstanceID;
};

struct VsOutput
{
  vec4f32 position : SV_POSITION;
  mat3f32 normal_matrix : NORMAL_MATRIX;
  vec2f32 local_position : LOCAL_POSITION;
  u32 probe_index : PROBE_INDEX;
};

[shader("vertex")] VsOutput vs_main(VsInput input)
{
  const GPUScene scene = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);

  const u32 probe_index = input.instance_id;

  const f32 probe_radius = push_constant.probe_radius;
  const vec3f32 probe_position =
    push_constant.ddgi_volume.probe_location_from_probe_index(probe_index);
  const vec3f32 billboard_to_camera = normalize(scene.camera_data.position - probe_position);
  const mat4f32 rotation_mat        = math::orthonormal_basis(billboard_to_camera);

  const mat4f32 scale_mat     = math::scale(MAT4F32_IDENTITY, vec3f32(probe_radius, probe_radius, 1));
  const mat4f32 translate_mat = math::translate(MAT4F32_IDENTITY, probe_position);
  const mat4f32 model_mat      = mul(translate_mat, mul(rotation_mat, scale_mat));
  const vec3f32 world_pos     = mul(model_mat, vec4f32(input.position, 0, 1)).xyz;

  VsOutput output;
  output.position       = mul(scene.camera_data.proj_view_mat_no_jitter, vec4f32(world_pos, 1));
  output.normal_matrix  = rotation_mat;
  output.local_position = input.position;
  output.probe_index    = probe_index;

  return output;
}

struct PsOutput
{
  [[vk::location(0)]] vec4f32 color : SV_Target;
};

[shader("pixel")] PsOutput ps_main(VsOutput input)
{
  const GPUScene scene               = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  Texture2D irradiance_probe_texture = get_texture_2d(push_constant.irradiance_probe_texture);
  const f32 plane_distance           = length(input.local_position);

  // Make a nice round circle out of a quad
  if (plane_distance > 0.5f)
  {
    discard;
  }

  // x^2 + y^2 + z^2 = 1
  // z = sqrt(1 - (x^2 + y^2))
  // z = sqrt(1 - (plane_distance ^ 2))
  const f32 z = sqrt(1.0 - plane_distance * plane_distance);

  // Calculate the normal for the sphere
  const vec3f32 local_sphere_normal = normalize(vec3f32(input.local_position, z));

  // Rotate to world space
  const vec3f32 world_sphere_normal = mul(input.normal_matrix, local_sphere_normal);

  const vec2f32 uv =
    push_constant.ddgi_volume.texture_coord_from_direction(world_sphere_normal, input.probe_index);
  const vec3f32 radiance =
    irradiance_probe_texture.SampleLevel(scene.get_linear_clamp_sampler(), uv, 0).xyz;

  PsOutput output;
  output.color = vec4f32(pow(radiance, 1.0f / 2.2f), 1.0f);
  return output;
}
