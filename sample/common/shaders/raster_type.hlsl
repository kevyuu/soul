#ifndef RASTER_TYPE_HLSL
#define RASTER_TYPE_HLSL

#include "shaders/wavefront_material.hlsl"

struct RasterObjInstanceData {
  soulsl::float4x4 transform; // Matrix of the instance
  soulsl::float4x4 normal_matrix; // Matrix of normal orientation
  soulsl::DescriptorID material_buffer_descriptor_id;
  soulsl::DescriptorID material_indices_descriptor_id;
  soulsl::float3 debug_color;

#ifndef SOUL_HOST_CODE
  WavefrontMaterial get_material_by_primitive_id(soulsl::uint1 primitive_id) {
    soulsl::uint1 mat_id = get_buffer_array<soulsl::uint1>(material_indices_descriptor_id, primitive_id);
    return get_buffer_array<WavefrontMaterial>(material_buffer_descriptor_id, mat_id);
  } 
#endif // not SOUL_HOST_CODE
};

struct RasterObjScene
{
  soulsl::DescriptorID instance_buffer_descriptor_id;
  soulsl::float4x4 view;
  soulsl::float4x4 projection;
  soulsl::float3 camera_position;
	soulsl::float3 light_position;
	float light_intensity;
	int light_type;

#ifndef SOUL_HOST_CODE
  RasterObjInstanceData get_instance_data(soulsl::uint1 id) {
    return get_buffer_array<RasterObjInstanceData>(instance_buffer_descriptor_id, id);
  }
#endif // not SOUL_HOST_CODE
};


#endif // RASTER_TYPE_HLSL