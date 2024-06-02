#include "render_nodes/common/brdf.hlsl"
#include "render_nodes/common/lighting.hlsl"
#include "render_nodes/common/ray_query.hlsl"
#include "render_nodes/rt_reflection/rt_reflection.shared.hlsl"

#include "utils/math.hlsl"
#include "utils/bnd_sampler.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"

struct Payload
{
  vec3f32 irradiance;
  f32 hit_distance;
};

[[vk::push_constant]] RayTracePC push_constant;

GPUScene get_scene()
{
  return get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
}

vec2u32 generate_rng_val(vec2u32 id, u32 frame_idx)
{
  u32 s0 = (id.x << 16) | id.y;
  u32 s1 = frame_idx;
  return vec2u32(pcg_hash(s0), pcg_hash(s1));
}

vec2f32 next_sample(vec2i32 coord)
{
  Texture2D sobol_tex              = get_texture_2d(push_constant.sobol_texture);
  Texture2D scrambling_ranking_tex = get_texture_2d(push_constant.scrambling_ranking_texture);

  return vec2f32(
    sample_blue_noise(coord, i32(push_constant.num_frames), 0, sobol_tex, scrambling_ranking_tex),
    sample_blue_noise(coord, i32(push_constant.num_frames), 1, sobol_tex, scrambling_ranking_tex));
}


[shader("raygeneration")] void rgen_main()
{
  const vec3u32 launch_id     = DispatchRaysIndex();
  const vec3u32 launch_size   = DispatchRaysDimensions();
  const vec2i32 current_coord = launch_id.xy;
  const vec2f32 pixel_center  = vec2f32(current_coord) + vec2f32(0.5, 0.5f);
  const vec2f32 current_uv    = pixel_center / launch_size.xy;

  RWTexture2D<vec4f32> output_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);
  Texture2D normal_roughness_texture  = get_texture_2d(push_constant.normal_roughness_gbuffer);
  Texture2D depth_texture             = get_texture_2d(push_constant.depth_gbuffer);

  GPUScene scene = get_scene();

  const vec4f32 normal_roughness = normal_roughness_texture.Load(vec3u32(current_coord, 0));
  const vec3f32 normal           = normal_roughness.xyz * 2.0f - 1.0f;
  const f32 roughness            = normal_roughness.w;
  const f32 depth                = depth_texture.Load(vec3u32(current_coord, 0)).x;

  const GPUCameraData camera_data = scene.camera_data;
  const mat4f32 inv_proj_view_mat = camera_data.inv_proj_view_mat;
  const vec3f32 world_position =
    world_position_from_depth(current_uv, depth, inv_proj_view_mat);
  const vec3f32 wo = normalize(camera_data.position.xyz - world_position);

  RayDesc ray_desc;
  ray_desc.Origin = world_position + push_constant.trace_normal_bias * normal;
  ray_desc.TMin   = 0.001;
  ray_desc.TMax   = 10000.0;

  u32 flags = RAY_FLAG_FORCE_OPAQUE;

  Payload payload;
  payload.irradiance   = vec3f32(0, 0, 0);
  payload.hit_distance = -1;

  if (roughness < MIRROR_REFLECTIONS_ROUGHNESS_THRESHOLD)
  {
    ray_desc.Direction = reflect(-wo, normal.xyz);
    TraceRay(scene.get_tlas(), flags, 0xff, 0, 0, 0, ray_desc, payload);
  } else
  {
    const vec2f32 rng_val = next_sample(current_coord) * push_constant.lobe_trim;
    const vec4f32 wh_pdf = importance_sample_ggx(rng_val, normal, roughness);


    const f32 pdf = wh_pdf.w;
    vec3f32 wi  = reflect(-wo, wh_pdf.xyz);
    ray_desc.Direction = reflect(-wo, wh_pdf.xyz);
    TraceRay(scene.get_tlas(), flags, 0xff, 0, 0, 0, ray_desc, payload);
  }

  output_texture[current_coord] = vec4f32(payload.irradiance, payload.hit_distance);
}

  [shader("miss")] void rmiss_main(inout Payload payload)
{
  GPUScene scene     = get_scene();
  EnvMap env_map     = scene.get_env_map();
  vec3f32 irradiance = env_map.sample_by_direction(WorldRayDirection());
  payload.irradiance = irradiance;
}

vec3f32 evaluate_punctual_light(
  GPULightInstance light_instance,
  RaytracingAccelerationStructure tlas,
  vec3f32 wo,
  vec3f32 surface_position,
  f32 normal_bias,
  MaterialInstance material_instance)
{
  const LightRaySample ray_sample = sample_light(light_instance, surface_position);
  const f32 visibility            = query_visibility(
    tlas,
    surface_position + normal_bias * material_instance.normal,
    ray_sample.direction,
    ray_sample.distance);

  return evaluate_normalized_brdf(wo, ray_sample.direction, material_instance) *
         ray_sample.radiance * visibility;
}


[shader("closesthit")] void rchit_main(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
  const vec3f32 barycentrics = vec3f32(1.0f - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
  GPUScene scene             = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);

  const MeshGeometryID mesh_geometry_id = MeshGeometryID::New(InstanceID(), GeometryIndex());

  VertexData vertex_data = scene.get_vertex_data(mesh_geometry_id, PrimitiveIndex(), barycentrics);

  const mat3f32 tbn_matrix = mat3_from_columns(
    vertex_data.world_tangent, vertex_data.world_bitangent, normalize(vertex_data.world_normal));
  MaterialInstance material_instance =
    scene.get_material_instance(vertex_data.material_index, vertex_data.tex_coord, tbn_matrix);

  const vec3f32 wo = -WorldRayDirection();

  vec3f32 irradiance = vec3f32(0.0f, 0.0f, 0.0f);
  for (u32 light_index = 0; light_index < scene.light_count; light_index++)
  {
    irradiance += evaluate_punctual_light(
      scene.get_light_instance(light_index),
      scene.get_tlas(),
      wo,
      vertex_data.world_position,
      push_constant.trace_normal_bias,
      material_instance);
  }

  payload.irradiance   = irradiance;
  payload.hit_distance = RayTCurrent();
}
