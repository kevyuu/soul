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
	vec3f32 position;
	vec3f32 normal;
	vec3f32 color;
	vec2f32 tex_coord;
};

struct RTObjScene
{
	soulsl::DescriptorID gpu_obj_buffer_descriptor_id;
  vec3f32 camera_position;
	mat4f32 view_inverse;
	mat4f32 projection_inverse;
	vec4f32 clear_color;
	vec3f32 light_position;
	f32 light_intensity;
	int light_type;
};

#endif
