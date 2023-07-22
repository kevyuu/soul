struct GPUObjDesc
{
	soulsl::DescriptorID vertex_descriptor_id;         // Address of the Vertex buffer
	soulsl::DescriptorID index_descriptor_id;          // Address of the index buffer
	soulsl::DescriptorID material_descriptor_id;       // Address of the material buffer
	soulsl::DescriptorID material_indices_descriptor_id;  // Address of the triangle material index buffer
};

struct GPUObjMaterial
{
	soulsl::float3 ambient;
	soulsl::float3 diffuse;
	soulsl::float3 specular;
	soulsl::float3 transmittance;
	soulsl::float3 emission;
	float shininess;
	float ior;
	float dissolve;

	int illum;
	soulsl::DescriptorID diffuse_texture_id;
};

struct GPUObjVertex  // See ObjLoader, copy of VertexObj, could be compressed for device
{
	soulsl::float3 position;
	soulsl::float3 normal;
	soulsl::float3 color;
	soulsl::float2 tex_coord;
};

struct GPUObjScene
{
	soulsl::DescriptorID gpu_obj_buffer_descriptor_id;
  soulsl::float3 camera_position;
	soulsl::float4x4 view_inverse;
	soulsl::float4x4 projection_inverse;
	soulsl::float4 clear_color;
	soulsl::float3 light_position;
	float light_intensity;
	int light_type;
};

