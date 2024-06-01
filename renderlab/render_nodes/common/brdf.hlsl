#ifndef SOULSL_BRDF_HLSL
#define SOULSL_BRDF_HLSL

#include "utils/math.hlsl"
#include "utils/constant.hlsl"
#include "type.shared.hlsl"

f32 ndf_trowbridgre_reitz_ggx(vec3f32 n, vec3f32 h, f32 roughness)
{
  const f32 a  = roughness * roughness;
  f32 a2       = a * a;
  f32 n_dot_h  = max(dot(n, h), 0.0);
  f32 n_dot_h2 = n_dot_h * n_dot_h;

  f32 num   = a2;
  f32 denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
  denom     = M_PI * denom * denom;

  return num / denom;
}

f32 geometry_schlick_ggx(f32 n_dot_v, f32 roughness)
{
  const f32 r = (roughness + 1.0);
  const f32 k = (r * r) / 8.0;

  const f32 num   = n_dot_v;
  const f32 denom = n_dot_v * (1.0 - k) + k;

  return num / denom;
}

f32 geometry_smith(vec3f32 n, vec3f32 wo, vec3f32 wi, f32 roughness)
{
  const f32 n_dot_wo = max(dot(n, wo), 0.0);
  const f32 n_dot_wi = max(dot(n, wi), 0.0);
  const f32 ggx_wo   = geometry_schlick_ggx(n_dot_wo, roughness);
  const f32 ggx_wi   = geometry_schlick_ggx(n_dot_wi, roughness);
  return ggx_wo * ggx_wi;
}

vec3f32 fresnel_schlick(f32 cos_theta, vec3f32 fresnel_base)
{
  return fresnel_base + (1.0 - fresnel_base) * pow(saturate(1.0f - cos_theta), 5.0);
}

vec3f32 fresnel_schlick_roughness(f32 cos_theta, vec3f32 fresnel_base, f32 roughness)
{
  f32 a = 1.0 - roughness;
  return fresnel_base + (max(vec3f32(a, a, a), fresnel_base) - fresnel_base) * pow(saturate(1.0 - cos_theta), 5.0);
}

vec4f32 importance_sample_ggx(vec2f32 rng_val, vec3f32 normal, f32 roughness)
{
    f32 a  = roughness * roughness;
    f32 m2 = a * a;

    f32 phi      = 2.0f * M_PI * rng_val.x;
    f32 cos_theta = sqrt((1.0f - rng_val.y) / (1.0f + (m2 - 1.0f) * rng_val.y));
    f32 sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    vec3f32 H;
    H.x = cos(phi) * sin_theta;
    H.y = sin(phi) * sin_theta;
    H.z = cos_theta;

    f32 d = (cos_theta * m2 - cos_theta) * cos_theta + 1;
    f32 D = m2 / (M_PI * d * d);

    f32 pdf = D * cos_theta;

    // from tangent-space H vector to world-space sample vector
    vec3f32 up        = abs(normal.z) < 0.999f ? vec3f32(0.0f, 0.0f, 1.0f) : vec3f32(1.0f, 0.0f, 0.0f);
    vec3f32 tangent   = normalize(cross(up, normal));
    vec3f32 bitangent = cross(normal, tangent);

    vec3f32 sample_vec = tangent * H.x + bitangent * H.y + normal * H.z;
    return vec4f32(normalize(sample_vec), pdf);
}

vec3f32 evaluate_brdf(
  vec3f32 wo, vec3f32 wi, vec3f32 n, vec3f32 albedo, f32 metallic, f32 roughness, vec3f32 fresnel_base)
{
  const vec3f32 h = normalize(wi + wo);

  const f32 ndf_val      = ndf_trowbridgre_reitz_ggx(n, h, roughness);
  const f32 g_val        = geometry_smith(n, wo, wi, roughness);
  const vec3f32 f_val    = fresnel_schlick(max(dot(h, wo), 0.0), fresnel_base);
  const vec3f32 num      = ndf_val * g_val * f_val;
  const f32 denom        = 4.0f * max(dot(n, wo), 0.0) * max(dot(n, wi), 0.0);
  const vec3f32 specular = denom > 0.0001 ? num / denom : 0.0;

  const vec3f32 ks = f_val;
  vec3f32 kd       = vec3f32(1.0, 1.0, 1.0) - ks;
  kd *= 1.0 - metallic;
  const vec3f32 diffuse = kd * (albedo / M_PI);

  return diffuse + specular;
}

vec3f32 evaluate_normalized_brdf(vec3f32 wo, vec3f32 wi, MaterialInstance mi)
{
  const f32 n_dot_wi         = max(dot(mi.normal, wi), 0.0);
  const vec3f32 albedo       = mi.albedo.xyz;
  const f32 metallic         = mi.metallic;
  const f32 roughness        = mi.roughness;
  const vec3f32 fresnel_base = lerp(vec3f32(0.04f, 0.04f, 0.04f), albedo, metallic);

  return evaluate_brdf(wo, wi, mi.normal, albedo, metallic, roughness, fresnel_base) * n_dot_wi;
}

#endif // SOULSL_BRDF_HLSL
