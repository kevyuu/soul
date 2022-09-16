#version 450
#extension GL_ARB_separate_shader_objects : enable

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

#define M_PI 3.1415926535897932384626433832795

layout(set = 0, binding = 0, std140) uniform CameraData{
	mat4 _camera_projection;
	mat4 _camera_view;
	mat4 _camera_projectionView;
	mat4 _camera_invProjectionView;

	vec3 _camera_cameraPosition;
	float _camera_exposure;
};

struct DirectionalLight {
	mat4 shadowMatrix[4];
	vec3 direction;
	float bias;
	vec3 color;
	float preExposedIlluminance;
	vec4 cascadeDepths;
};

layout(set = 0, binding = 1, std140) uniform Light {
	DirectionalLight dirLight;
};

layout(set = 0, binding = 2) uniform sampler2D shadowMap;

struct Material {

	vec3 albedo;
	float metallic;
	vec3 emissive;
	float roughness;

	uint flags;
};

layout(set = 1, binding = 0) uniform PerMaterial {
	Material material;
};

layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D metallicMap;
layout(set = 2, binding = 3) uniform sampler2D roughnessMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;
layout(set = 2, binding = 5) uniform sampler2D emissiveMap;

struct PixelMaterial {
	vec3 f0;
	vec3 albedo;
	vec3 normal;
	float metallic;
	float roughness;
	float ao;
	vec3 emissive;
};

layout(location = 0) in VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

layout(location = 0) out vec4 renderTarget1;
layout(location = 1) out vec4 renderTarget2;
layout(location = 2) out vec4 renderTarget3;
layout(location = 3) out vec4 renderTarget4;

bool bitTest(uint flags, uint mask) {
	return ((flags & mask) == mask);
}

PixelMaterial pixelMaterialCreate(Material material, vec2 texCoord) {

	PixelMaterial pixelMaterial;

	pixelMaterial.albedo = pow(material.albedo, vec3(2.2f));
	if (bitTest(material.flags, MaterialFlag_USE_ALBEDO_TEX)) {
		pixelMaterial.albedo *= pow(texture(albedoMap, texCoord).rgb, vec3(2.2f));
	}

	if (bitTest(material.flags, MaterialFlag_USE_NORMAL_TEX)) {
		pixelMaterial.normal = normalize(vec3(texture(normalMap, texCoord).rgb * 2 - 1.0f));
	}
	else {
		pixelMaterial.normal = vec3(0, 0, 1.0f);
	}

	if (bitTest(material.flags, MaterialFlag_USE_METALLIC_TEX)) {
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_RED)) pixelMaterial.metallic = texture(metallicMap, texCoord).r;
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_GREEN)) pixelMaterial.metallic = texture(metallicMap, texCoord).g;
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_BLUE)) pixelMaterial.metallic = texture(metallicMap, texCoord).b;
		if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_ALPHA)) pixelMaterial.metallic = texture(metallicMap, texCoord).a;
		pixelMaterial.metallic *= material.metallic;
	}
	else {
		pixelMaterial.metallic = material.metallic;
	}

	if (bitTest(material.flags, MaterialFlag_USE_ROUGHNESS_TEX)) {
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_RED)) pixelMaterial.roughness = texture(roughnessMap, texCoord).r;
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_GREEN)) pixelMaterial.roughness = texture(roughnessMap, texCoord).g;
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_BLUE)) pixelMaterial.roughness = texture(roughnessMap, texCoord).b;
		if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_ALPHA)) pixelMaterial.roughness = texture(roughnessMap, texCoord).a;
		pixelMaterial.roughness *= material.roughness;
	}
	else {
		pixelMaterial.roughness = material.roughness;
	}

	pixelMaterial.roughness = max(pixelMaterial.roughness, 1e-4);

	if (bitTest(material.flags, MaterialFlag_USE_AO_TEX)) {
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_RED)) pixelMaterial.ao = texture(aoMap, texCoord).r;
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_GREEN)) pixelMaterial.ao = texture(aoMap, texCoord).g;
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_BLUE)) pixelMaterial.ao = texture(aoMap, texCoord).b;
		if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_ALPHA)) pixelMaterial.ao = texture(aoMap, texCoord).a;
	}
	else {
		pixelMaterial.ao = 1;
	}

	if (bitTest(material.flags, MaterialFlag_USE_EMISSIVE_TEX)) {
		pixelMaterial.emissive = texture(emissiveMap, texCoord).rgb;
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

float computeShadowFactor(vec3 worldPosition, mat4 shadowMatrix, float bias) {

	vec4 lightSpacePosition = shadowMatrix * vec4(worldPosition, 1.0f);
	float currentDepth =  (lightSpacePosition.z + lightSpacePosition.w) / 2.0;
	vec4 shadowCoords = lightSpacePosition / lightSpacePosition.w;
	shadowCoords.y = shadowCoords.y * 0.5 + 0.5;
	shadowCoords.x = shadowCoords.x * 0.5 + 0.5;

	float shadowFactor = 1;

    float shadowFactorPCF = 0.0f;
    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            float sampledDepth = texture(shadowMap, shadowCoords.xy + vec2(i, j) * texelSize).r;
            shadowFactorPCF += currentDepth - bias > sampledDepth ? 1.0 : 0.0;
        }
    }
    shadowFactor = shadowFactorPCF / 25;

    return shadowFactor;
}

void main() {
    PixelMaterial pixelMaterial = pixelMaterialCreate(material, vs_out.texCoord);
	mat3 TBN = mat3(vs_out.worldTangent, vs_out.worldBinormal, vs_out.worldNormal);
	vec3 N = normalize(TBN * pixelMaterial.normal);
	vec3 V = normalize(_camera_cameraPosition - vs_out.worldPosition);
	vec3 L = normalize(dirLight.direction * -1);
	vec3 worldPosition = vs_out.worldPosition;

	vec3 specularOutput = vec3(0.0f);
	vec3 diffuseOutput = vec3(0.0f);

	float cascadeSplit[5] = {
		0,
		dirLight.cascadeDepths.x,
		dirLight.cascadeDepths.y,
		dirLight.cascadeDepths.z,
		dirLight.cascadeDepths.w
	};

	vec3 fragViewCoord = vec4(_camera_view * vec4(worldPosition, 1.0f)).xyz;

	float fragDepth = -1 * fragViewCoord.z;

	int cascadeIndex = -1;
	float shadowFactor = 0;

	for (int i = 0; i < 5; i++) {
		if (fragDepth > cascadeSplit[i]) cascadeIndex = i;
	}

	float cascadeBlendRange = 1.0f;
	float bias = dirLight.bias;

	vec3 colorMarker = vec3(1, 0.2f, 0.2f);
	if (cascadeIndex >= 3) {
		colorMarker = vec3(1, 0.2f, 0.2f);
		shadowFactor = 0;
	}
	if (cascadeIndex == 3) {
		colorMarker = vec3(0.5f, 1, 0.5f);
		shadowFactor = computeShadowFactor(worldPosition, dirLight.shadowMatrix[cascadeIndex], bias);
	}
	else {
		if (cascadeIndex == 2) {
			colorMarker = vec3(0.5f, 0.5f, 1.0f);
		} else if (cascadeIndex == 1) {
			colorMarker = vec3(0.5f, 1.0f, 1.0f);
		} else if (cascadeIndex == 0) {
			colorMarker = vec3(1.0f, 1.0f, 0.5f);
		}
		float shadowFactor1 = computeShadowFactor(worldPosition, dirLight.shadowMatrix[cascadeIndex], bias);
		float shadowFactor2 = computeShadowFactor(worldPosition, dirLight.shadowMatrix[cascadeIndex + 1], bias);

		float cascadeDist = cascadeSplit[cascadeIndex + 1] - cascadeSplit[cascadeIndex];
		float cascadeBlend = smoothstep(cascadeSplit[cascadeIndex] + (1 - cascadeBlendRange) * cascadeDist, cascadeSplit[cascadeIndex + 1], fragDepth);
		shadowFactor = mix(shadowFactor1, shadowFactor2, cascadeBlend);
	}

	vec3 illuminance = dirLight.color * dirLight.preExposedIlluminance;
	illuminance = illuminance * (1 - shadowFactor);

	specularOutput += computeSpecularBRDF(L, V, N, pixelMaterial) * illuminance * max(dot(N, L), 0.0f);
	diffuseOutput += computeDiffuseBRDF(L, V, N, pixelMaterial) * illuminance * max(dot(N, L), 0.0f);

	vec3 ambient = 0.2f * pixelMaterial.albedo;

	renderTarget1 = vec4(pixelMaterial.albedo, 1.0f);
	renderTarget2 = vec4(specularOutput, pixelMaterial.metallic);
	renderTarget3 = vec4(N * 0.5f + vec3(0.5f), pixelMaterial.roughness);
	vec3 fragViewPos = worldPosition - _camera_cameraPosition;
	float fragSquareDistance = min(dot(fragViewPos, fragViewPos), 0.01f);
	renderTarget4 = vec4(ambient + diffuseOutput + (pixelMaterial.emissive / fragSquareDistance), pixelMaterial.ao);

}