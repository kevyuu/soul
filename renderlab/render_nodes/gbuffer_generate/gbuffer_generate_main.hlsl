#include "render_nodes/gbuffer_generate/gbuffer_generate_type.hlsl"
#include "scene.hlsl"

[[vk::push_constant]] GBufferGeneratePushConstant push_constant;

struct VSInput
{
  [[vk::location(0)]] vec3f32 position : POSITION;
  [[vk::location(1)]] vec3f32 normal : NORMAL;
  [[vk::location(2)]] vec4f32 tangent : TANGENT;
  [[vk::location(3)]] vec2f32 tex_coord : TEXCOORD;
  uint instance_id : SV_InstanceID;
};

struct VSOutput
{
  vec4f32 position : SV_POSITION;
  vec4f32 cs_position : CS_POSITION;
  vec4f32 prev_cs_position : PREV_CS_POSITION;
  vec2f32 tex_coord : TEXCOORD0;
  mat3f32 tbn_matrix : TBN_MATRIX;
  uint instance_id : INSTANCE_ID;
};

VSOutput vs_main(VSInput input)
{
  VSOutput output;

  GPUScene scene = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);

  MeshInstance instance = scene.get_mesh_instance(input.instance_id);

  mat4f32 world_matrix       = scene.get_world_matrix(instance.matrix_index);
  mat4f32 normal_matrix      = scene.get_normal_matrix(instance.matrix_index);
  mat4f32 prev_world_matrix  = scene.get_prev_world_matrix(instance.matrix_index);
  mat4f32 prev_normal_matrix = scene.get_prev_normal_matrix(instance.matrix_index);

  vec4f32 world_position      = mul(world_matrix, vec4f32(input.position, 1.0));
  vec4f32 prev_world_position = mul(prev_world_matrix, vec4f32(input.position, 1.0));
  vec3f32 world_normal        = normalize(mul(normal_matrix, vec4f32(input.normal, 0.0)).xyz);
  vec3f32 world_tangent       = normalize(mul(normal_matrix, vec4f32(input.tangent.xyz, 0.0)).xyz);
  vec3f32 world_bitangent     = cross(world_normal, world_tangent) * input.tangent.w;

  output.position       = mul(scene.camera_data.proj_view_mat, world_position);
  output.cs_position    = mul(scene.camera_data.proj_view_mat_no_jitter, world_position);
  output.prev_cs_position =
    mul(scene.prev_camera_data.proj_view_mat_no_jitter, prev_world_position);
  output.tex_coord   = input.tex_coord;
  output.tbn_matrix  = mat3_from_columns(world_tangent, world_bitangent, world_normal);
  output.instance_id = input.instance_id;

  return output;
}

struct PSOutput
{
  [[vk::location(0)]] vec4f32 albedo_metal : SV_Target0;
  [[vk::location(1)]] vec4f32 normal_roughness : SV_Target1;
  [[vk::location(2)]] vec3f32 motion_curve : SV_Target2;
  [[vk::location(3)]] u32 mesh_index : SV_Target3;
  [[vk::location(4)]] vec3f32 emissive : SV_TARGET4;
};

vec2f32 compute_motion_vector(
  vec4f32 prev_pos, vec4f32 current_pos, vec2f32 prev_jitter, vec2f32 current_jitter)
{
  // Perspective division, covert clip space positions to NDC.
  vec2f32 current = (current_pos.xy / current_pos.w);
  vec2f32 prev    = (prev_pos.xy / prev_pos.w);

  // Remap to [0, 1] range
  current = current * 0.5 + 0.5;
  prev    = prev * 0.5 + 0.5;

  // Calculate velocity (current -> prev)
  return (prev + prev_jitter) - (current + current_jitter);
}

f32 compute_curvature(vec3f32 normal)
{
  vec3f32 dx = ddx(normal);
  vec3f32 dy = ddy(normal);

  f32 x = dot(dx, dx);
  f32 y = dot(dy, dy);

  return pow(max(x, y), 0.5f);
}

[shader("pixel")] PSOutput ps_main(VSOutput input, uint primitive_id
                                   : SV_PrimitiveId)
{
  GPUScene scene = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);

  MeshInstance instance = scene.get_mesh_instance(input.instance_id);
  MaterialInstance material_instance =
    scene.get_material_instance_ps(instance.material_index, input.tex_coord, input.tbn_matrix);

  vec2f32 motion_vector = compute_motion_vector(
    input.prev_cs_position,
    input.cs_position,
    scene.prev_camera_data.jitter,
    scene.camera_data.jitter);
  f32 linearz = input.position.z / input.position.w;

  PSOutput output;
  output.albedo_metal = vec4f32(material_instance.albedo.xyz, material_instance.metallic);
  output.normal_roughness =
    vec4f32(material_instance.normal * 0.5 + 0.5, material_instance.roughness);
  output.motion_curve = vec3f32(motion_vector, compute_curvature(input.tbn_matrix._m02_m12_m22));
  output.mesh_index   = instance.mesh_id;
  output.emissive = vec3f32(material_instance.emissive);
  return output;
}
