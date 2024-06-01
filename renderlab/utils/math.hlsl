#ifndef SOULSL_MATH_HLSL
#define SOULSL_MATH_HLSL

#include "utils/constant.hlsl"

/******************************************************************************

    Miscellaneous functions

******************************************************************************/

f32 square(f32 v)
{
  return v * v;
}

vec3f32 square(vec3f32 v)
{
  return v * v;
}

f32 pow3(f32 x)
{
  return x * x * x;
}

f32 sign_not_zero(f32 k)
{
  return (k >= 0.0) ? 1.0 : -1.0;
}

vec2f32 sign_not_zero(vec2f32 v)
{
  return vec2f32(sign_not_zero(v.x), sign_not_zero(v.y));
}

mat3f32 mat3_identity()
{
  mat3f32 result;
  result[0] = vec3f32(1, 0, 0);
  result[1] = vec3f32(0, 1, 0);
  result[2] = vec3f32(0, 0, 1);
  return result;
}

mat4f32 mat4_identity()
{
  mat4f32 result;
  result[0] = vec4f32(1, 0, 0, 0);
  result[1] = vec4f32(0, 1, 0, 0);
  result[2] = vec4f32(0, 0, 1, 0);
  result[3] = vec4f32(0, 0, 0, 1);
  return result;
}

mat3f32 mat3_from_columns(vec3f32 col0, vec3f32 col1, vec3f32 col2)
{
  mat3f32 result;
  result[0] = vec3f32(col0.x, col1.x, col2.x);
  result[1] = vec3f32(col0.y, col1.y, col2.y);
  result[2] = vec3f32(col0.z, col1.z, col2.z);
  return result;
}

mat4f32 mat4_from_columns(vec4f32 col0, vec4f32 col1, vec4f32 col2, vec4f32 col3)
{
  mat4f32 result;
  result[0] = vec4f32(col0.x, col1.x, col2.x, col3.x);
  result[1] = vec4f32(col0.y, col1.y, col2.y, col3.x);
  result[2] = vec4f32(col0.z, col1.z, col2.z, col3.z);
  result[3] = vec4f32(col0.w, col1.w, col2.w, col3.w);
  return result;
}

mat3f32 mat3_from_upper_mat4(mat4f32 original_mat)
{
  mat3f32 result;
  result[0] = original_mat[0].xyz;
  result[1] = original_mat[1].xyz;
  result[2] = original_mat[2].xyz;
  return result;
}

mat3f32 create_basis_matrix(vec3f32 z)
{
  const vec3f32 ref = abs(dot(z, vec3f32(0, 1, 0))) > 0.99f ? vec3f32(0, 0, 1) : vec3f32(0, 1, 0);

  const vec3f32 x = normalize(cross(ref, z));
  const vec3f32 y = cross(z, x);

  return mat3_from_columns(x, y, z);
}

mat4f32 mat4_rotation_from_direction(vec3f32 z)
{
  const vec3f32 ref = abs(dot(z, vec3f32(0, 1, 0))) > 0.99f ? vec3f32(0, 0, 1) : vec3f32(0, 1, 0);

  const vec3f32 x = normalize(cross(ref, z));
  const vec3f32 y = cross(z, x);
  
  return mat4_from_columns(vec4f32(x, 0), vec4f32(y, 0), vec4f32(z, 0), vec4f32(0, 0, 0, 1));
}

f32 gaussian_weight(f32 offset, f32 deviation)
{
  f32 weight = 1.0 / sqrt(2.0 * M_PI * deviation * deviation);
  weight *= exp(-(offset * offset) / (2.0 * deviation * deviation));
  return weight;
}

vec3f32 world_position_from_depth(vec2f32 uv, f32 ndc_depth, mat4f32 inv_proj_view_mat)
{

  // Take texture coordinate and remap to [-1.0, 1.0] range.
  const vec2f32 screen_pos = uv * 2.0 - 1.0;

  // // Create NDC position.
  const vec4f32 ndc_pos = vec4f32(screen_pos, ndc_depth, 1.0);

  // Transform back into world position.
  vec4f32 world_pos = mul(inv_proj_view_mat, ndc_pos);

  // Undo projection.
  world_pos = world_pos / world_pos.w;

  return world_pos.xyz;
}

vec2f32 equirectangular_uv_from_direction(vec3f32 dir)
{
  const f32 phi     = atan2(dir.z, dir.x);
  const f32 thetha  = asin(dir.y);
  const f32 rng_val = (phi / (2 * M_PI)) + 0.5f;
  const f32 v       = (thetha / M_PI) + 0.5f;

  return vec2f32(rng_val, -v);
}

/******************************************************************************

    Octahedral mapping

    The center of the map represents the +z axis and its corners -z.
    The rotated inner square is the xy-plane projected onto the upper hemi-
    sphere, the outer four triangles folds down over the lower hemisphere.


******************************************************************************/
vec2f32 oct_snorm_from_ndir(vec3f32 v)
{
  f32 l1norm     = abs(v.x) + abs(v.y) + abs(v.z);
  vec2f32 result = v.xy * (1.0 / l1norm);
  if (v.z < 0.0)
  {
    result = (1.0 - abs(result.yx)) * sign_not_zero(result.xy);
  }
  return result;
}

vec2f32 oct_unorm_from_ndir(vec3f32 v)
{
  return oct_snorm_from_ndir(v) * 0.5 + 0.5;
}

vec3f32 ndir_from_oct_snorm(vec2f32 o)
{
  vec3f32 v = vec3f32(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));

  if (v.z < 0.0)
  {
    v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
  }

  return normalize(v);
}

vec3f32 ndir_from_oct_unorm(vec2f32 o)
{
  return ndir_from_oct_snorm(o * 2.0 - 1.0);
}

/******************************************************************************

    Sampling functions

******************************************************************************/
/**
 * Uniform sampling of the unit disk.
 * @param[in] rng_val Uniform random number [0,1)^2.
 * @return Sampled point on the unit disk.
 */
vec2f32 sample_disk_polar(vec2f32 rng_val)
{
  const f32 r   = sqrt(rng_val.x);
  const f32 phi = rng_val.y * M_2PI;
  return vec2f32(r * cos(phi), r * sin(phi));
}

/**
 * Uniform sampling of the unit disk using Shirley's concentric mapping.
 * @param[in] rng_val Uniform random numbers [0,1)^2.
 * @return Sampled point on the unit disk.
 */
vec2f32 sample_disk_concentric(vec2f32 rng_val)
{
  rng_val = 2.f * rng_val - 1.f;
  if (rng_val.x == 0.f && rng_val.y == 0.f)
  {
    return rng_val;
  }
  f32 phi, r;
  if (abs(rng_val.x) > abs(rng_val.y))
  {
    r   = rng_val.x;
    phi = (rng_val.y / rng_val.x) * M_PI_4;
  } else
  {
    r   = rng_val.y;
    phi = M_PI_2 - (rng_val.x / rng_val.y) * M_PI_4;
  }
  return r * vec2f32(cos(phi), sin(phi));
}

/**
 * Uniform sampling of direction within a cone
 * @param[in] rng_val Uniform random number [0,1)^2.
 * @param[in] cos_theta Cosine of the cone half-angle
 * @return Sampled direction within the cone with (0,0,1) axis
 */
vec3f32 sample_cone(vec2f32 rng_val, f32 cos_theta)
{
  const f32 z   = rng_val.x * (1.f - cos_theta) + cos_theta;
  const f32 r   = sqrt(1.f - z * z);
  const f32 phi = M_PI * 2 * rng_val.y;
  return vec3f32(r * cos(phi), r * sin(phi), z);
}

vec3f32 sample_hemisphere_cosine_w(vec3f32 n, vec2f32 r)
{
  f32 min_value       = 0.00001f;
  vec2f32 rand_sample = max(vec2f32(min_value, min_value), r);

  const f32 phi = 2.0f * M_PI * rand_sample.y;

  const f32 cos_theta = sqrt(rand_sample.x);
  const f32 sin_theta = sqrt(1 - rand_sample.x);

  vec3f32 t = vec3f32(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

  return normalize(mul(create_basis_matrix(n), t));
}

vec3f32 sample_hemisphere_uniform(vec3f32 n, vec2f32 r)
{
  f32 min_value       = 0.00001f;
  vec2f32 rand_sample = max(vec2f32(min_value, min_value), r);

  const f32 phi = 2.0f * M_PI * rand_sample.y;

  const f32 cos_theta = rand_sample.x;
  const f32 sin_theta = 1 - rand_sample.x;

  vec3f32 t = vec3f32(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

  return normalize(mul(create_basis_matrix(n), t));
}

/******************************************************************************

    Hash functions

******************************************************************************/
u32 wang_hash(u32 seed)
{
  seed = (seed ^ 61) ^ (seed >> 16);
  seed *= 9;
  seed = seed ^ (seed >> 4);
  seed *= 0x27d4eb2d;
  seed = seed ^ (seed >> 15);
  return seed;
}

u32 pcg_hash(u32 input)
{
  u32 state = input * 747796405u + 2891336453u;
  u32 word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

vec3f32 spherical_fibonacci(f32 i, f32 n)
{
  const f32 PHI = sqrt(5) * 0.5 + 0.5;

#define madfrac(A, B) ((A) * (B)-floor((A) * (B)))

  f32 phi      = 2.0 * M_PI * madfrac(i, PHI - 1);
  f32 cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
  f32 sinTheta = sqrt(saturate(1.0 - cosTheta * cosTheta));

  return vec3f32(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

#undef madfrac
}

f32 luminance(vec3f32 rgb)
{
    return max(dot(rgb, vec3f32(0.299, 0.587, 0.114)), 0.0001);
}

vec3f32 mix(vec3f32 x, vec3f32 y, f32 s)
{
  return x * (1.0 - s) + y * s;
}

#endif // SOULSL_MATH_HLSL
