#ifndef SOULSL_SV_HLSL
#define SOULSL_SV_HLSL

struct ComputeSV
{
  vec3u32 global_id : SV_DispatchThreadID;
  vec3u32 local_id : SV_GroupThreadID;
  vec3u32 group_id : SV_GroupID;
  u32 local_index : SV_GroupIndex;
};

#endif // SOULSL_SV_HLSL
