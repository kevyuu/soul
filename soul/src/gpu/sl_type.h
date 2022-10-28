#pragma once

#include "gpu/id.h"

namespace soulsl
{
    using DescriptorID = soul::gpu::DescriptorID;

    using float1 = float;
    using float2 = soul::vec2f;
    using float3 = soul::vec3f;
    using float4 = soul::vec4f;

    using double1 = double;
    using double2 = soul::vec2d;
    using double3 = soul::vec3d;
    using double4 = soul::vec4d;

    using int1 = int32;
    using int2 = soul::vec2i32;
    using int3 = soul::vec3i32;
    using int4 = soul::vec4i32;

    using uint1 = uint32;
    using uint2 = soul::vec2ui32;
    using uint3 = soul::vec3ui32;
    using uint4 = soul::vec4ui32;

    using float3x3 = soul::mat3f;
    using float4x4 = soul::mat4f;

    using address = soul::gpu::GPUAddress;
    using duint = uint64;
    
}