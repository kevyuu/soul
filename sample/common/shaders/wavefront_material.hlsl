#ifndef WAVEFRONT_MATERIAL_HLSL
#define WAVEFRONT_MATERIAL_HLSL

struct WavefrontMaterial
{
	soulsl::float3 ambient;
	soulsl::float3 diffuse;
	soulsl::float3 specular;
	soulsl::float3 transmittance;
	soulsl::float3 emission;
	f32 shininess;
	f32 ior;
	f32 dissolve;

	int illum;
	soulsl::DescriptorID diffuse_texture_id;
};

#ifndef SOUL_HOST_CODE
soulsl::float3 compute_diffuse(WavefrontMaterial mat, soulsl::float3 light_dir, soulsl::float3 normal)
{
	// Lambertian
	f32 dotNL = max(dot(normal, light_dir), 0.0);
	soulsl::float3 c = mat.diffuse * dotNL;
	if (mat.illum >= 1)
		c += mat.ambient;
	return c;
}

soulsl::float3 compute_specular(WavefrontMaterial mat, soulsl::float3 view_dir, 
	soulsl::float3 light_dir, soulsl::float3 normal)
{
	if (mat.illum < 2)
		return soulsl::float3(0, 0, 0);

	// Compute specular only if not in shadow
	const f32 kPi = 3.14159265;
	const f32 kShininess = max(mat.shininess, 4.0);

	// Specular
	const f32 kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	soulsl::float3 V = normalize(-view_dir);
	soulsl::float3 R = reflect(-light_dir, normal);
	f32 specular = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

	return soulsl::float3(mat.specular * specular);
}
#endif // no SOUL_HOST_CODE

#endif // WAVEFRONT_MATERIAL_HLSL
