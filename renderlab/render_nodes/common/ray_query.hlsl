#ifndef SOULSL_RAY_QUERY_HLSL
#define SOULSL_RAY_QUERY_HLSL

f32 query_visibility(
  RaytracingAccelerationStructure tlas, vec3f32 world_pos, vec3f32 direction, f32 t_max)
{
  RayQuery<
    RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
    RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH>
    query;

  RayDesc ray_desc;
  ray_desc.Origin    = world_pos;
  ray_desc.Direction = direction;
  ray_desc.TMin      = 0.001;
  ray_desc.TMax      = t_max;

  query.TraceRayInline(tlas, 0, 0xff, ray_desc);

  query.Proceed();

  if (query.CommittedStatus() == COMMITTED_NOTHING)
  {
    return 1.0;
  }
  return 0.0;
}


#endif
