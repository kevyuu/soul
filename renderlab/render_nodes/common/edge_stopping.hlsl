#ifndef SOULSL_EDGE_STOPPING_HLSL
#define SOULSL_EDGE_STOPPING_HLSL

#define DEPTH_FACTOR 0.5

f32 normal_edge_stopping_weight(vec3f32 center_normal, vec3f32 sample_normal, f32 power)
{
  return pow(clamp(dot(center_normal, sample_normal), 0.0f, 1.0f), power);
}

f32 depth_edge_stopping_weight(f32 center_depth, f32 sample_depth, f32 phi)
{
  return exp(-abs(center_depth - sample_depth) / phi);
}

f32 luma_edge_stopping_weight(f32 center_luma, f32 sample_luma, f32 phi)
{
  return abs(center_luma - sample_luma) / phi;
}

f32 compute_edge_stopping_weight(
  f32 center_depth,
  f32 sample_depth,
  f32 phi_z,
  vec3f32 center_normal,
  vec3f32 sample_normal,
  f32 phi_normal,
  f32 center_luma,
  f32 sample_luma,
  f32 phi_luma)
{
  const f32 wZ = depth_edge_stopping_weight(center_depth, sample_depth, phi_z);
  const f32 wNormal = normal_edge_stopping_weight(center_normal, sample_normal, phi_normal);
  const f32 wL = luma_edge_stopping_weight(center_luma, sample_luma, phi_luma);
  const f32 w = exp(0.0 - max(wL, 0.0) - max(wZ, 0.0)) * wNormal;
  return w;
}

#endif // SOULSL_EDGE_STOPPING_HLSL
