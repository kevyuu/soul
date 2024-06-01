#include "render_nodes/common/brdf.hlsl"
#include "render_nodes/common/lighting.hlsl"
#include "render_nodes/common/sv.hlsl"
#include "render_nodes/deferred_shading/deferred_shading.shared.hlsl"

#include "scene.hlsl"
#include "type.shared.hlsl"
#include "utils/constant.hlsl"
#include "utils/math.hlsl"

[[vk::push_constant]] DeferredShadingPC push_constant;

vec3f32 compute_indirect_diffuse_lighting(
  vec3f32 wo, vec3i32 load_index, MaterialInstance material_instance)
{
  Texture2D indirect_diffuse_texture = get_texture_2d(push_constant.indirect_diffuse_texture);
  const vec3f32 R                    = reflect(-wo, material_instance.normal);
  const vec3f32 fresnel_base =
    lerp(vec3f32(0.04f, 0.04f, 0.04f), material_instance.albedo, material_instance.metallic);

  const vec3f32 diffuse_color = lerp(
    material_instance.albedo * (vec3f32(1.0f, 1.0f, 1.0f) - fresnel_base),
    vec3f32(0.0f, 0.0f, 0.0f),
    material_instance.metallic);

  vec3f32 F = fresnel_schlick_roughness(
    max(dot(material_instance.normal, wo), 0.0), fresnel_base, material_instance.roughness);

  vec3f32 kS = F;
  vec3f32 kD = vec3f32(1.0, 1.0, 1.0) - kS;
  kD *= (1.0 - material_instance.metallic);

  vec3f32 irradiance = indirect_diffuse_texture.Load(load_index).rgb;
  vec3f32 diffuse    = irradiance * diffuse_color;
  return kD * diffuse;
}

vec3f32 compute_indirect_specular_lighting(
  vec3f32 wo, vec3i32 load_index, MaterialInstance material_instance, GPUScene scene)
{
  Texture2D indirect_specular_texture = get_texture_2d(push_constant.indirect_specular_texture);
  Texture2D brdf_lut_texture          = get_texture_2d(push_constant.brdf_lut_texture);
  vec3f32 specular_radiance           = indirect_specular_texture.Load(load_index).rgb;
  SamplerState brdf_sampler           = scene.get_nearest_clamp_sampler();

  const vec3f32 R = reflect(-wo, material_instance.normal);
  const vec3f32 fresnel_base =
    lerp(vec3f32(0.04f, 0.04f, 0.04f), material_instance.albedo, material_instance.metallic);
  vec3f32 F = fresnel_schlick_roughness(
    max(dot(material_instance.normal, wo), 0.0), fresnel_base, material_instance.roughness);

  vec2f32 brdf_sample_coord =
    vec2f32(max(dot(material_instance.normal, wo), 0.0), material_instance.roughness);
  vec2f32 brdf     = brdf_lut_texture.SampleLevel(brdf_sampler, brdf_sample_coord, 0).xy;
  vec3f32 specular = specular_radiance * (F * brdf.x + brdf.y);
  return specular;
}

[numthreads(WORK_GROUP_SIZE_X, WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  RWTexture2D<vec4f32> out_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  const vec2i32 texel_coord  = sv.global_id.xy;
  const vec2u32 dimension    = get_rw_texture_dimension(out_texture);
  const vec2f32 pixel_center = vec2f32(texel_coord) + vec2f32(0.5f, 0.5f);
  const vec2f32 uv_coord     = pixel_center / vec2f32(dimension);

  const GPUScene scene = get_buffer<GPUScene>(push_constant.gpu_scene_id, 0);

  SamplerState nearest_clamp_sampler = scene.get_nearest_clamp_sampler();
  Texture2D albedo_metallic_texture  = get_texture_2d(push_constant.albedo_metallic_texture);
  Texture2D motion_curve_texture     = get_texture_2d(push_constant.motion_curve_texture);
  Texture2D normal_roughness_texture = get_texture_2d(push_constant.normal_roughness_texture);
  Texture2D depth_texture            = get_texture_2d(push_constant.depth_texture);
  Texture2D light_visibility_texture = get_texture_2d(push_constant.light_visibility_texture);
  Texture2D ao_texture               = get_texture_2d(push_constant.ao_texture);

  const vec2u32 visiblity_dimension = get_texture_dimension(light_visibility_texture);
  const vec3u32 visibility_coord    = vec3u32(uv_coord * visiblity_dimension, 0);

  const vec3i32 load_index = vec3u32(texel_coord, 0);

  const GPUCameraData camera_data = scene.camera_data;
  const vec4f32 albedo_metallic   = albedo_metallic_texture.Load(load_index);
  const vec4f32 motion_curve      = motion_curve_texture.Load(load_index);
  const f32 curvature             = motion_curve.z;
  const vec4f32 normal_roughness  = normal_roughness_texture.Load(load_index);

  const vec3f32 albedo = albedo_metallic.xyz;
  const f32 metallic   = albedo_metallic.w;
  const vec3f32 n      = normal_roughness.xyz * 2.0 - 1.0;
  const f32 roughness  = normal_roughness.w;

  MaterialInstance material_instance;
  material_instance.albedo    = albedo;
  material_instance.metallic  = metallic;
  material_instance.roughness = roughness;
  material_instance.normal    = n;

  const f32 ndc_depth             = depth_texture.Load(load_index).x;
  const mat4f32 inv_proj_view_mat = camera_data.inv_proj_view_mat;
  const vec3f32 world_position = world_position_from_depth(uv_coord, ndc_depth, inv_proj_view_mat);
  const vec3f32 wo             = normalize(camera_data.position - world_position);

  vec3f32 irradiance        = 0.0f;
  f32 visibility            = 1.0f;
  f32 ao                    = 1.0f;
  vec3f32 indirect_diffuse  = 0.0f;
  vec3f32 indirect_specular = 0.0f;

  if (ndc_depth != 1.0f)
  {
    visibility = light_visibility_texture.Load(visibility_coord).r;
    for (u32 light_index = 0; light_index < scene.light_count; light_index++)
    {
      const GPULightInstance light_instance = scene.get_light_instance(light_index);
      irradiance +=
        evaluate_irradiance_from_light(wo, world_position, light_instance, material_instance) *
        visibility;
    }
    ao = ao_texture.Load(load_index).r;

    indirect_diffuse = compute_indirect_diffuse_lighting(wo, load_index, material_instance) *
                       push_constant.indirect_diffuse_intensity;
    indirect_specular =
      compute_indirect_specular_lighting(wo, load_index, material_instance, scene) *
      push_constant.indirect_specular_intensity;

    // For now we use constant ambient term
    const vec3f32 ambient = (indirect_diffuse + indirect_specular) * ao;
    irradiance += ambient;
  } else
  {
    const EnvMap env_map = scene.get_env_map();
    RayDesc ray          = scene.camera_data.compute_ray_pinhole(texel_coord, dimension);
    irradiance           = env_map.sample_by_direction(ray.Direction);
  }

  out_texture[texel_coord] = vec4f32(irradiance.xyz, 1.0f);
}
