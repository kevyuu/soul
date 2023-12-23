#include "shaders/rt_type.hlsl"
#include "shaders/rt_basic_type.hlsl"

[[vk::push_constant]]
RayTracingPushConstant push_constant;

struct ColorPayload
{
	float3 hit_value;
};

struct ShadowPayload
{
	bool is_shadowed;
};

[shader("raygeneration")]
void rgen_main()
{
	uint3 LaunchID = DispatchRaysIndex();
	uint3 LaunchSize = DispatchRaysDimensions();
	RTObjScene scene = get_buffer<RTObjScene>(push_constant.scene_descriptor_id, 0);

	const float2 pixel_center = float2(LaunchID.xy) + float2(0.5, 0.5);
	const float2 in_uv = pixel_center / float2(LaunchSize.xy);
	float2 d = in_uv * 2.0 - 1.0;
	float4 target = mul(scene.projection_inverse, float4(d.x, d.y, 1, 1));

	float3 origin = mul(scene.view_inverse, float4(0, 0, 0, 1)).xyz;
	float3 direction = mul(scene.view_inverse, float4(normalize(target.xyz), 0)).xyz;

	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.Direction = direction;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;

	ColorPayload payload;
	RaytracingAccelerationStructure rs = get_as(push_constant.as_descriptor_id);
	TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

	RWTexture2D<float4> image = get_rw_texture_2d_float4(push_constant.image_descriptor_id);\
	f32 gamma = 1. / 2.2;
	float4 color = pow(float4(payload.hit_value, 0.0), float4(gamma, gamma, gamma, gamma));
	image[int2(LaunchID.xy)] = float4(color);
}
 
[shader("miss")]
void rmiss_main(inout ColorPayload p)
{
	RTObjScene scene = get_buffer<RTObjScene>(push_constant.scene_descriptor_id, 0);
	p.hit_value = scene.clear_color.xyz;
}

[shader("miss")]
void rmiss_shadow_main(inout ShadowPayload p)
{
	p.is_shadowed = false;
}

struct Attribute
{
	float2 attribs;
};

[shader("closesthit")]
void rchit_main(inout ColorPayload p, in float2 attribs)
{
	const float3 barycentrics = float3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

  RTObjScene scene = get_buffer<RTObjScene>(push_constant.scene_descriptor_id, 0);
  RTObjDesc gpu_obj_desc = get_buffer_array<RTObjDesc>(scene.gpu_obj_buffer_descriptor_id, InstanceID());

  const uint3 triangle_indices = get_buffer_array<uint3>(gpu_obj_desc.index_descriptor_id, PrimitiveIndex());

  const RTObjVertex v0 = get_buffer_array<RTObjVertex>(gpu_obj_desc.vertex_descriptor_id, triangle_indices.x);
	const RTObjVertex v1 = get_buffer_array<RTObjVertex>(gpu_obj_desc.vertex_descriptor_id, triangle_indices.y);
	const RTObjVertex v2 = get_buffer_array<RTObjVertex>(gpu_obj_desc.vertex_descriptor_id, triangle_indices.z);

  const float3 object_pos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	const float3 world_pos = mul(ObjectToWorld3x4(), float4(object_pos, 1));
	const float3 object_normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
	const float3 world_normal = mul(ObjectToWorld3x4(), float4(object_normal, 1));

	int mat_index = get_buffer_array<int>(gpu_obj_desc.material_indices_descriptor_id, PrimitiveIndex());
	WavefrontMaterial material = get_buffer_array<WavefrontMaterial>(gpu_obj_desc.material_descriptor_id, mat_index);

	float3 L;
	f32 light_intensity = scene.light_intensity;
	f32 light_distance = 100000.0;

	if (scene.light_type == 0)
	{
		float3 light_dir = scene.light_position - world_pos;
		light_distance = length(light_dir);
		light_intensity = scene.light_intensity / (light_distance * light_distance);
		L = normalize(light_dir);
	}
	else
	{
		L = normalize(scene.light_position);
	}
	float3 diffuse = compute_diffuse(material, L, world_normal);
	if (material.diffuse_texture_id.is_valid())
	{
		float2 tex_coord = v0.tex_coord * barycentrics.x + v1.tex_coord * barycentrics.y + v2.tex_coord * barycentrics.z;
		Texture2D diffuse_texture = get_texture_2d(material.diffuse_texture_id);
		SamplerState diffuse_sampler = get_sampler(push_constant.sampler_descriptor_id);
		diffuse = diffuse_texture.SampleLevel(diffuse_sampler, tex_coord, 0).xyz;
	}

	float3 specular = float3(0, 0, 0);
	f32 attenuation = 1;

	if (dot(world_normal, L) > 0)
	{
		RayDesc ray_desc;
		ray_desc.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
		ray_desc.Direction = L;
		ray_desc.TMin = 0.001;
		ray_desc.TMax = light_distance;

		uint flags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
		ShadowPayload payload;
		payload.is_shadowed = true;
		RaytracingAccelerationStructure rs = get_as(push_constant.as_descriptor_id);
		TraceRay(rs, flags, 0xFF, 0, 0, 1, ray_desc, payload);

		if (payload.is_shadowed)
		{
			attenuation = 0.3;
		}
		else
		{
			specular = compute_specular(material, WorldRayDirection(), L, world_normal);
		}
	}

    p.hit_value = (light_intensity * attenuation * (diffuse + specular));
}
