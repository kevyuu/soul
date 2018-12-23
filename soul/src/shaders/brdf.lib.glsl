struct PixelMaterial {
	vec3 f0;
	vec3 albedo;
	vec3 normal;
	float metallic;
	float roughness;
};

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = 3.14 * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 computeDiffuseBRDF(vec3 L, vec3 V, vec3 N, PixelMaterial pixelMaterial)
{
	vec3 H = normalize(L + V);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), pixelMaterial.f0);
	vec3 kd = vec3(1.0f) - F;
	kd *= 1.0f - pixelMaterial.metallic;
	return kd * pixelMaterial.albedo / M_PI;
}

vec3 computeSpecularBRDF(vec3 L, vec3 V, vec3 N, PixelMaterial pixelMaterial)
{
	vec3 H = normalize((L + V) / 2.0f);

	float NDF = DistributionGGX(N, H, pixelMaterial.roughness);
	float G = GeometrySmith(N, V, L, pixelMaterial.roughness);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), pixelMaterial.f0);

	float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);

	return (F * G * NDF) / max(denominator, 0.001f);
}

vec3 computeOutgoingRadiance(vec3 L, vec3 V, vec3 N, PixelMaterial pixelMaterial, vec3 radiance) {
	vec3 specular = computeSpecularBRDF(L, V, N, pixelMaterial);
	vec3 diffuse = computeDiffuseBRDF(L, V, N, pixelMaterial);

	return (specular + diffuse) * radiance * max(dot(N, L), 0.0);
}

