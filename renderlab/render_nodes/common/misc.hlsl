#ifndef SOULSL_MISC_HLSL
#define SOULSL_MISC_HLSL

#include "utils/math.hlsl"

vec4f32 clip_aabb(vec3f32 aabb_min, vec3f32 aabb_max, vec4f32 clip_origin, vec4f32 clip_input)
{
  vec4f32 r     = clip_input - clip_origin;
  vec3f32 rmax  = aabb_max - clip_origin.xyz;
  vec3f32 rmin  = aabb_min - clip_origin.xyz;
  const f32 eps = FLT_EPSILON;
  if (r.x > rmax.x + eps)
  {
    r *= (rmax.x / r.x);
  }
  if (r.y > rmax.y + eps)
  {
    r *= (rmax.y / r.y);
  }
  if (r.z > rmax.z + eps)
  {
    r *= (rmax.z / r.z);
  }
  if (r.x < rmin.x - eps)
  {
    r *= (rmin.x / r.x);
  }
  if (r.y < rmin.y - eps)
  {
    r *= (rmin.y / r.y);
  }
  if (r.z < rmin.z - eps)
  {
    r *= (rmin.z / r.z);
  }
  return clip_origin + r;
}

vec3f32 clip_aabb(vec3f32 aabb_min, vec3f32 aabb_max, vec3f32 clip_input)
{
  // Note: only clips towards aabb center
  vec3f32 aabb_center = 0.5f * (aabb_max + aabb_min);
  vec3f32 extent_clip = 0.5f * (aabb_max - aabb_min) + 0.001f; // avoid division by zero

  // Find color vector
  vec3f32 color_vector      = clip_input - aabb_center;
  // Transform into clip space
  vec3f32 color_vector_clip = color_vector / extent_clip;
  // Find max absolute component
  color_vector_clip         = abs(color_vector_clip);
  f32 max_abs_unit = max(max(color_vector_clip.x, color_vector_clip.y), color_vector_clip.z);

  if (max_abs_unit > 1.0)
  {
    return aabb_center + color_vector / max_abs_unit; // clip towards color vector
  } else
  {
    return clip_input; // point is inside aabb
  }
}

#endif
