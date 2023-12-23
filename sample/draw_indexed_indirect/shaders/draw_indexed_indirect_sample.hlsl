#include "shaders/draw_indexed_indirect_type.hlsl"
#include "shaders/raster_type.hlsl"

[[vk::push_constant]] RasterPushConstant push_constant;

struct VSInput {
  [[vk::location(0)]] float3 position : POSITION;
  [[vk::location(1)]] float3 normal : NORMAL;
  [[vk::location(2)]] float3 color : COLOR;
  [[vk::location(3)]] float2 tex_coord : TEXCOORD;
  uint instance_id: SV_InstanceID;
};

struct VSOutput {
  float4 position : SV_POSITION;
  float3 world_position : POSITION0;
  float3 world_normal : NORMAL0;
  float3 color : COLOR0;
  float2 tex_coord : TEXCOORD;
  uint instance_id : INSTANCE_ID;
};

[shader("vertex")] VSOutput vs_main(VSInput input) {

  VSOutput output;

  RasterObjScene scene = get_buffer<RasterObjScene>(push_constant.gpu_scene_id, 0);

  RasterObjInstanceData instance_data = scene.get_instance_data(input.instance_id);

  float4 world_position = mul(instance_data.transform, float4(input.position, 1.0));

  output.position = mul(scene.projection, mul(scene.view, world_position));

  output.world_position = world_position.xyz;

  output.world_normal = mul(instance_data.normal_matrix, input.normal);

  output.color = input.color;

  output.tex_coord = input.tex_coord;

  output.instance_id = input.instance_id;

  return output;
}

struct PSOutput {
  [[vk::location(0)]] float4 color : SV_TARGET;
};

[shader("pixel")] PSOutput ps_main(VSOutput input, uint primitive_id : SV_PrimitiveId) {
  RasterObjScene scene = get_buffer<RasterObjScene>(push_constant.gpu_scene_id, 0);

  RasterObjInstanceData instance_data = scene.get_instance_data(input.instance_id);

  float3 L;
  f32 light_intensity = scene.light_intensity;
  f32 light_distance = 100000.0;
  float3 light_color = float3(1.0, 1.0, 1.0);

  uint primitive_debug_idx = primitive_id;
  if (input.instance_id == 1) {
    primitive_debug_idx = 3;
  }
  WavefrontMaterial material = instance_data.get_material_by_primitive_id(primitive_id);

  if (scene.light_type == 0) {
    float3 light_dir = scene.light_position - input.world_position;
    light_distance = length(light_dir);
    light_intensity = scene.light_intensity / (light_distance * light_distance);
    L = normalize(light_dir);
  } else {
    L = normalize(scene.light_position);
  }

  float3 diffuse = compute_diffuse(material, L, input.world_normal);
  if (material.diffuse_texture_id.is_valid()) {
    Texture2D diffuse_texture = get_texture_2d(material.diffuse_texture_id);
    SamplerState diffuse_sampler = get_sampler(push_constant.sampler_id);
    diffuse = diffuse_texture.SampleLevel(diffuse_sampler, input.tex_coord, 0).xyz;
  }

  float3 view_dir = normalize(scene.camera_position - input.world_position);
  float3 specular = compute_specular(material, view_dir, L, input.world_normal);

  PSOutput output;
  output.color = float4(light_intensity * (diffuse + specular), 1.0);
  return output;
}
