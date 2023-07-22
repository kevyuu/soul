#ifndef WAVEFRONT_MATERIAL_HLSL
#define WAVEFRONT_MATERIAL_HLSL

struct WavefrontMaterial
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

#endif // WAVEFRONT_MATERIAL_HLSL
