#ifndef WAVEFRONT_MATERIAL_HLSL
#define WAVEFRONT_MATERIAL_HLSL

struct WavefrontMaterial
{
	vec3f32 ambient;
	vec3f32 diffuse;
	vec3f32 specular;
	vec3f32 transmittance;
	vec3f32 emission;
	f32 shininess;
	f32 ior;
	f32 dissolve;

	int illum;
	soulsl::DescriptorID diffuse_texture_id;
};

#ifndef SOUL_HOST_CODE
vec3f32 compute_diffuse(WavefrontMaterial mat, vec3f32 light_dir, vec3f32 normal)
{
	// Lambertian
	f32 dotNL = max(dot(normal, light_dir), 0.0);
	vec3f32 c = mat.diffuse * dotNL;
	if (mat.illum >= 1)
		c += mat.ambient;
	return c;
}

vec3f32 compute_specular(WavefrontMaterial mat, vec3f32 view_dir, 
	vec3f32 light_dir, vec3f32 normal)
{
	if (mat.illum < 2)
		return vec3f32(0, 0, 0);

	// Compute specular only if not in shadow
	const f32 kPi = 3.14159265;
	const f32 kShininess = max(mat.shininess, 4.0);

	// Specular
	const f32 kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	vec3f32 V = normalize(-view_dir);
	vec3f32 R = reflect(-light_dir, normal);
	f32 specular = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

	return vec3f32(mat.specular * specular);
}
#endif // no SOUL_HOST_CODE

#endif // WAVEFRONT_MATERIAL_HLSL
