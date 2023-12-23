#ifndef RT_TYPE_HLSL
#define RT_TYPE_HLSL

#include "shaders/wavefront_material.hlsl"

struct RTObjDesc
{
	soulsl::DescriptorID vertex_descriptor_id;         // Address of the Vertex buffer
	soulsl::DescriptorID index_descriptor_id;          // Address of the index buffer
	soulsl::DescriptorID material_descriptor_id;       // Address of the material buffer
	soulsl::DescriptorID material_indices_descriptor_id;  // Address of the triangle material index buffer
};

struct RTObjVertex  // See ObjLoader, copy of VertexObj, could be compressed for device
{
	soulsl::float3 position;
	soulsl::float3 normal;
	soulsl::float3 color;
	soulsl::float2 tex_coord;
};

struct RTObjScene
{
	soulsl::DescriptorID gpu_obj_buffer_descriptor_id;
  soulsl::float3 camera_position;
	soulsl::float4x4 view_inverse;
	soulsl::float4x4 projection_inverse;
	soulsl::float4 clear_color;
	soulsl::float3 light_position;
	f32 light_intensity;
	int light_type;
};

#endif
