#include "render_nodes/common/brdf.hlsl"
#include "render_nodes/common/lighting.hlsl"
#include "render_nodes/common/ray_query.hlsl"
#include "render_nodes/ddgi/ddgi.shared.hlsl"

#include "utils/bnd_sampler.hlsl"
#include "utils/math.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"

struct [raypayload] Payload
{
  vec3f32 irradiance;
  f32 hit_distance;
  vec2u32 rng_val;
};

[[vk::push_constant]] RayTracingPC push_constant;

GPUScene get_scene()
{
  return get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
}

vec2u32 generate_rng_val(vec2u32 id, u32 frame_idx);

[shader("raygeneration")] void rgen_main()
{
  const vec3u32 launch_id   = DispatchRaysIndex();
  const vec3u32 launch_size = DispatchRaysDimensions();

  GPUScene scene         = get_scene();
  DdgiVolume ddgi_volume = push_constant.ddgi_volume;

  RWTexture2D<vec4f32> irradiance_output_texture =
    get_rw_texture_2d_vec4f32(push_constant.irradiance_output_texture);
  RWTexture2D<vec4f32> direction_depth_output_texture =
    get_rw_texture_2d_vec4f32(push_constant.direction_depth_output_texture);

  const u32 probe_index  = launch_id.y;
  const u32 ray_id       = launch_id.x;
  const mat3f32 rotation = mat3_from_upper_mat4(push_constant.rotation);

  vec2u32 texel_coord = launch_id.xy;

  RayDesc ray_desc;
  ray_desc.Origin    = ddgi_volume.probe_location_from_probe_index(probe_index);
  ray_desc.Direction = ddgi_volume.get_sampling_direction(rotation, ray_id);
  ray_desc.TMin      = 0.001;
  ray_desc.TMax      = 10000.0;

  u32 flags = RAY_FLAG_FORCE_OPAQUE;

  Payload payload;
  payload.irradiance   = vec3f32(0, 0, 0);
  payload.hit_distance = ray_desc.TMax;
  payload.rng_val      = generate_rng_val(texel_coord, push_constant.frame_idx);
  TraceRay(scene.get_tlas(), flags, 0xff, 0, 0, 0, ray_desc, payload);

  irradiance_output_texture[texel_coord]      = vec4f32(payload.irradiance, 0);
  direction_depth_output_texture[texel_coord] = vec4f32(ray_desc.Direction, payload.hit_distance);
}

  [shader("miss")] void rmiss_main(inout Payload payload)
{
  GPUScene scene     = get_scene();
  EnvMap env_map     = scene.get_env_map();
  payload.irradiance = env_map.sample_by_direction(WorldRayDirection());
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

vec3f32 evaluate_env_map(
  EnvMap env_map,
  RaytracingAccelerationStructure tlas,
  vec3f32 wo,
  vec3f32 surface_position,
  f32 normal_bias,
  MaterialInstance material_instance,
  vec2f32 rng_val)
{
  const vec3f32 wi       = sample_hemisphere_cosine_w(material_instance.normal, rng_val);
  const vec3f32 radiance = env_map.sample_by_direction(wi);
  const f32 visibility =
    query_visibility(tlas, surface_position + normal_bias * material_instance.normal, wi, 10000);

  return evaluate_normalized_brdf(wo, wi, material_instance) * radiance * visibility;
}

vec3f32 evaluate_gi(
  vec3f32 wo, vec3f32 surface_position, MaterialInstance material_instance, SamplerState sampler)
{
  Texture2D depth_probe_texture = get_texture_2d(push_constant.history_depth_probe_texture);
  Texture2D irradiance_probe_texture =
    get_texture_2d(push_constant.history_irradiance_probe_texture);

  const vec3f32 fresnel_base =
    lerp(vec3f32(0.04f, 0.04f, 0.04f), material_instance.albedo, material_instance.metallic);

  const vec3f32 diffuse_color = lerp(
    material_instance.albedo * (vec3f32(1.0f, 1.0f, 1.0f) - fresnel_base),
    vec3f32(0.0f, 0.0f, 0.0f),
    material_instance.metallic);

  vec3f32 F = fresnel_schlick_roughness(
    max(dot(material_instance.normal, wo), 0.0), fresnel_base, material_instance.roughness);

  vec3f32 kS = F;
  vec3f32 kD = 1.0 - kS;
  kD *= 1.0 - material_instance.metallic;

  return kD * diffuse_color *
         push_constant.ddgi_volume.sample_irradiance(
           surface_position,
           material_instance.normal,
           wo,
           sampler,
           irradiance_probe_texture,
           depth_probe_texture);
}

[shader("closesthit")] void rchit_main(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
  const vec3f32 barycentrics = vec3f32(1.0f - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
  GPUScene scene             = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);
  const DdgiVolume ddgi      = push_constant.ddgi_volume;

  const MeshGeometryID mesh_geometry_id = MeshGeometryID::New(InstanceID(), GeometryIndex());

  VertexData vertex_data = scene.get_vertex_data(mesh_geometry_id, PrimitiveIndex(), barycentrics);

  if (dot(vertex_data.world_normal, WorldRayDirection()) >= 0.2f) // check backface
  {
    // set irradiance to 0, and reduce depth to 20%
    payload.irradiance   = 0;
    payload.hit_distance = 0.2f * RayTCurrent();
  } else
  {
    const mat3f32 tbn_matrix = mat3_from_columns(
      vertex_data.world_tangent, vertex_data.world_bitangent, vertex_data.world_normal);
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
        ddgi.visibility_normal_bias,
        material_instance);
    }

    if (push_constant.frame_idx != 0 && ddgi.bounce_intensity > 0)
    {
      irradiance +=
        ddgi.bounce_intensity *
        evaluate_gi(
          wo, vertex_data.world_position, material_instance, scene.get_linear_clamp_sampler());
    }

    irradiance += (material_instance.emissive / (RayTCurrent() * RayTCurrent()));

    payload.irradiance   = irradiance;
    payload.hit_distance = RayTCurrent();
  }
}

vec2u32 generate_rng_val(vec2u32 id, u32 frame_idx)
{
  u32 s0 = (id.x << 16) | id.y;
  u32 s1 = frame_idx;
  return vec2u32(pcg_hash(s0), pcg_hash(s1));
}
