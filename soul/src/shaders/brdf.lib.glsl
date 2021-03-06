#define MaterialFlag_USE_ALBEDO_TEX (1u << 0)
#define MaterialFlag_USE_NORMAL_TEX (1u << 1)
#define MaterialFlag_USE_METALLIC_TEX (1u << 2)
#define MaterialFlag_USE_ROUGHNESS_TEX (1u << 3)
#define MaterialFlag_USE_AO_TEX (1u << 4)
#define MaterialFlag_USE_EMISSIVE_TEX (1u << 5)

#define MaterialFlag_METALLIC_CHANNEL_RED (1u << 8)
#define MaterialFlag_METALLIC_CHANNEL_GREEN (1u << 9)
#define MaterialFlag_METALLIC_CHANNEL_BLUE (1u << 10)
#define MaterialFlag_METALLIC_CHANNEL_ALPHA (1u << 11)

#define MaterialFlag_ROUGHNESS_CHANNEL_RED (1u << 12)
#define MaterialFlag_ROUGHNESS_CHANNEL_GREEN (1u << 13)
#define MaterialFlag_ROUGHNESS_CHANNEL_BLUE (1u << 14)
#define MaterialFlag_ROUGHNESS_CHANNEL_ALPHA (1u << 15)

#define MaterialFlag_AO_CHANNEL_RED (1u << 16)
#define MaterialFlag_AO_CHANNEL_GREEN (1u << 17)
#define MaterialFlag_AO_CHANNEL_BLUE (1u << 18)
#define MaterialFlag_AO_CHANNEL_ALPHA (1u << 19)

struct Material {
	sampler2D albedoMap;
	sampler2D normalMap;
	sampler2D metallicMap;
	sampler2D roughnessMap;
	sampler2D aoMap;
	sampler2D emissiveMap;

	vec3 albedo;
	float metallic;
	float roughness;
	vec3 emissive;

	uint flags;
};

struct PixelMaterial {
	vec3 f0;
	vec3 albedo;
	vec3 normal;
	float metallic;
	float roughness;
	float ao;
	vec3 emissive;
};

bool bitTest(uint flags, uint mask) {
	return ((flags & mask) == mask);
}

PixelMaterial pixelMaterialCreate(Material material, vec2 texCoord) {

	PixelMaterial pixelMaterial;
	
	pixelMaterial.albedo = pow(material.albedo, vec3(2.2));
	if (bitTest(material.flags, MaterialFlag_USE_ALBEDO_TEX)) {
		pixelMaterial.albedo *= pow(texture(material.albedoMap, texCoord).rgb, vec3(2.2));
	}

	if (bitTest(material.flags, MaterialFlag_USE_NORMAL_TEX)) {
		pixelMaterial.normal = normalize(vec3(texture(material.normalMap, texCoord).rgb * 2 - 1.0f));
	}
	else {
		pixelMaterial.normal = vec3(0, 0, 1.0f);
	}

	if (bitTest(material.flags, MaterialFlag_USE_METALLIC_TEX)) {
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_RED)) pixelMaterial.metallic = texture(material.metallicMap, texCoord).r;
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_GREEN)) pixelMaterial.metallic = texture(material.metallicMap, texCoord).g;
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_BLUE)) pixelMaterial.metallic = texture(material.metallicMap, texCoord).b;
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_ALPHA)) pixelMaterial.metallic = texture(material.metallicMap, texCoord).a;
		pixelMaterial.metallic *= material.metallic;
	}
	else {
		pixelMaterial.metallic = material.metallic;
	}

	if (bitTest(material.flags, MaterialFlag_USE_ROUGHNESS_TEX)) {
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_RED)) pixelMaterial.roughness = texture(material.roughnessMap, texCoord).r;
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_GREEN)) pixelMaterial.roughness = texture(material.roughnessMap, texCoord).g;
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_BLUE)) pixelMaterial.roughness = texture(material.roughnessMap, texCoord).b;
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_ALPHA)) pixelMaterial.roughness = texture(material.roughnessMap, texCoord).a;
		pixelMaterial.roughness *= material.roughness;
	}
	else {
		pixelMaterial.roughness = material.roughness;
	}

	pixelMaterial.roughness = max(pixelMaterial.roughness, 1e-4);

	if (bitTest(material.flags, MaterialFlag_USE_AO_TEX)) {
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_RED)) pixelMaterial.ao = texture(material.aoMap, texCoord).r;
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_GREEN)) pixelMaterial.ao = texture(material.aoMap, texCoord).g;
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_BLUE)) pixelMaterial.ao = texture(material.aoMap, texCoord).b;
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_ALPHA)) pixelMaterial.ao = texture(material.aoMap, texCoord).a;
	}
	else {
		pixelMaterial.ao = 1;
	}

	if (bitTest(material.flags, MaterialFlag_USE_EMISSIVE_TEX)) {
		pixelMaterial.emissive = texture(material.emissiveMap, texCoord).rgb;
		pixelMaterial.emissive *= material.emissive;
	}
	else {
		pixelMaterial.emissive = material.emissive;
	}

	pixelMaterial.f0 = mix(vec3(0.04f), pixelMaterial.albedo, pixelMaterial.metallic);

	return pixelMaterial;

}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 1e-4);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = M_PI * denom * denom;
	denom = max(denom, 1e-8);

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

float GeometrySchlickGGXIBL(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom/denom;
}

float GeometrySmithIBL(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGXIBL(NdotV, roughness);
    float ggx1 = GeometrySchlickGGXIBL(NdotL, roughness);

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

