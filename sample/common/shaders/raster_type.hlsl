#ifndef RASTER_TYPE_HLSL
#define RASTER_TYPE_HLSL

#include "shaders/wavefront_material.hlsl"

struct RasterObjInstanceData {
  mat4f32 transform; // Matrix of the instance
  mat4f32 normal_matrix; // Matrix of normal orientation
  soulsl::DescriptorID material_buffer_descriptor_id;
  soulsl::DescriptorID material_indices_descriptor_id;
  vec3f32 debug_color;

#ifndef SOULSL_HOST_CODE
  WavefrontMaterial get_material_by_primitive_id(u32 primitive_id) {
    u32 mat_id = get_buffer_array<u32>(material_indices_descriptor_id, primitive_id);
    return get_buffer_array<WavefrontMaterial>(material_buffer_descriptor_id, mat_id);
  } 
#endif // not SOUL_HOST_CODE
};

struct RasterObjScene
{
  soulsl::DescriptorID instance_buffer_descriptor_id;
  mat4f32 view;
  mat4f32 projection;
  vec3f32 camera_position;
	vec3f32 light_position;
	f32 light_intensity;
	int light_type;

#ifndef SOULSL_HOST_CODE
  RasterObjInstanceData get_instance_data(u32 id) {
    return get_buffer_array<RasterObjInstanceData>(instance_buffer_descriptor_id, id);
  }
#endif // not SOUL_HOST_CODE
};


#endif // RASTER_TYPE_HLSL
