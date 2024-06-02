#ifndef SOULSL_LIGHTING_HLSL
#define SOULSL_LIGHTING_HLSL

#include "scene.hlsl"
#include "type.shared.hlsl"
#include "utils/constant.hlsl"

#include "render_nodes/common/brdf.hlsl"

static const float kMaxLightDistance = FLT_MAX;

struct LightRaySample
{
  vec3f32 world_pos;
  vec3f32 direction;
  f32 distance;
  vec3f32 radiance;
  f32 pdf;
};

LightRaySample sample_point_light(GPULightInstance light_instance, vec3f32 surface_pos)
{

  const vec3f32 light_position = light_instance.position;
  const vec3f32 wi             = normalize(light_position - surface_pos);

  const f32 distance    = length(light_position - surface_pos);
  const f32 attenuation = 1.0 / (distance * distance);

  LightRaySample sample;

  sample.world_pos = light_position;
  sample.direction = wi;
  sample.distance  = distance;
  sample.radiance  = light_instance.intensity * attenuation;
  sample.pdf       = 0.0f;

  return sample;
}

LightRaySample sample_sun_light(GPULightInstance light_instance, vec3f32 surface_pos)
{
  LightRaySample sample;

  sample.world_pos = vec3f32(0, 0, 0);
  sample.direction = light_instance.direction;
  sample.distance  = FLT_MAX;
  sample.radiance  = light_instance.intensity;
  sample.pdf       = 0.0f;

  return sample;
}

LightRaySample sample_sun_light(
  GPULightInstance light_instance, vec3f32 surface_pos, vec2f32 rng_val)
{
  const vec3f32 light_dir = normalize(light_instance.direction);
  vec3f32 light_tangent   = normalize(cross(light_dir, vec3f32(0.0f, 1.0f, 0.0f)));
  vec3f32 light_bitangent = normalize(cross(light_tangent, light_dir));

  vec2f32 disk_point = sample_disk_polar(rng_val) * 0.12;
  vec3f32 ray_direction =
    normalize(light_dir + disk_point.x * light_tangent + disk_point.y * light_bitangent);

  LightRaySample sample;

  sample.world_pos = vec3f32(0, 0, 0);
  sample.direction = ray_direction;
  sample.distance = FLT_MAX;
  sample.radiance = light_instance.intensity;
  sample.pdf = 0.0f;
  return sample;
}

LightRaySample sample_light(GPULightInstance light_instance, vec3f32 surface_pos)
{
  LightRaySample ray_sample;
  switch (light_instance.radiation_type)
  {
  case (u32)(LightRadiationType::POINT):
  {
    ray_sample = sample_point_light(light_instance, surface_pos);
    break;
  }
  case (u32)(LightRadiationType::DIRECTIONAL):
  {
    ray_sample = sample_sun_light(light_instance, surface_pos);
    break;
  }
  }
  return ray_sample;
}

vec3f32 evaluate_irradiance_from_light(
  vec3f32 wo,
  vec3f32 surface_pos,
  GPULightInstance light_instance,
  MaterialInstance material_instance)
{
  LightRaySample ray_sample = sample_light(light_instance, surface_pos);
  const vec3f32 radiance    = ray_sample.radiance;
  return evaluate_normalized_brdf(wo, ray_sample.direction, material_instance) * radiance;
}

vec3f32 evaluate_irradiance_from_env_map(
  vec3f32 wo, vec3f32 wi, EnvMap env_map, MaterialInstance material_instance)
{
  const vec3f32 radiance = env_map.sample_by_direction(wi);
  return evaluate_normalized_brdf(wo, wi, material_instance) * radiance;
}
#endif
