#define M_PI 3.1415926535897932384626433832795

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aBinormal;
layout (location = 4) in vec3 aTangent;

layout(std140) uniform SceneData {
	mat4 projection;
	mat4 view;
};

uniform mat4 model;

out VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vs_out.worldPosition = vec3((model * vec4(aPos, 1.0f)).xyz);
	mat3 rotMat = mat3(transpose(inverse(model)));
	vs_out.worldNormal = rotMat * aNormal;
	vs_out.worldTangent = rotMat * aTangent;
	vs_out.worldBinormal = rotMat * aBinormal;
	vs_out.texCoord = aTexCoord;
}
#endif //VERTEX SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

struct Material {
	sampler2D albedoMap;
	sampler2D normalMap;
	sampler2D metallicMap;
	sampler2D roughnessMap;
	sampler2D aoMap;
};

struct PixelMaterial {
	vec3 f0;
	vec3 albedo;
	vec3 normal;
	float metallic;
	float roughness;
	float ao;
};

struct OmniLight {
	vec3 position;
	vec3 color;
};

struct DirectionalLight {
	mat4 shadowMatrix1;
	mat4 shadowMatrix2;
	mat4 shadowMatrix3;
	mat4 shadowMatrix4;
	vec3 direction;
	float pad1;
	vec3 color;
	float pad2;
	vec4 cascadeDepths;
};

struct SpotLight {
	vec3 position;
	vec3 color;
	float maxAngle;
};

layout(std140) uniform SceneData {
	mat4 projection;
	mat4 view;
};

layout (std140) uniform LightData {
	DirectionalLight directionalLights[4];
	int directionalLightCount;
};

uniform Material material;
uniform sampler2D shadowMap;
uniform vec3 viewPosition;

in VS_OUT {
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

float computeShadowFactor(vec3 worldPosition, mat4 shadowMatrix) {
	if (directionalLightCount == 0) return 0;

	vec4 lightSpacePosition = shadowMatrix * vec4(worldPosition, 1.0f);
	vec4 shadowCoords = lightSpacePosition / lightSpacePosition.w;
	shadowCoords = shadowCoords * 0.5 + 0.5;

	float currentDepth = shadowCoords.z;

	float bias = 0.005;
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

	PixelMaterial pixelMaterial;

	pixelMaterial.albedo = pow(texture(material.albedoMap, vs_out.texCoord).rgb, vec3(2.2));
	pixelMaterial.normal = normalize(vec3(texture(material.normalMap, vs_out.texCoord).rgb * 2 - 1.0f));
	pixelMaterial.metallic = texture(material.metallicMap, vs_out.texCoord).r;
	pixelMaterial.roughness = texture(material.roughnessMap, vs_out.texCoord).r;
	pixelMaterial.ao = texture(material.aoMap, vs_out.texCoord).r;
	pixelMaterial.f0 = mix(vec3(0.04f), pixelMaterial.albedo, pixelMaterial.metallic);

	mat3 BTN = mat3(vs_out.worldBinormal, vs_out.worldTangent, vs_out.worldNormal);
    vec3 worldNormal = normalize(BTN * pixelMaterial.normal);
	vec3 worldPosition = vs_out.worldPosition;
	vec3 V = normalize(viewPosition - worldPosition);
	vec3 fragViewCoord = vec4(view * vec4(worldPosition, 1.0f)).xyz;
	
	vec3 specularOutput = vec3(0.0f);
	vec3 diffuseOutput = vec3(0.0f);

	for (int i = 0; i < directionalLightCount; i++) {
		vec3 L = normalize(directionalLights[i].direction * -1);

		vec3 radiance = directionalLights[i].color;

		vec4 cascadeDepths = directionalLights[i].cascadeDepths;

		float shadowFactor = 0.0f;

		float fragDepth = fragViewCoord.z * -1;

		if (fragDepth < cascadeDepths.x) {
			shadowFactor = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix1);
		} else if (fragDepth < cascadeDepths.y) {
			shadowFactor = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix2);
		} else if (fragDepth < cascadeDepths.z) {
			shadowFactor = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix3);
		} else if (fragDepth < cascadeDepths.w) {
			shadowFactor = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix4);
		}

		radiance = radiance * (1.0f - shadowFactor);

		specularOutput += computeSpecularBRDF(L, V, worldNormal, pixelMaterial) * radiance *  max(dot(worldNormal, L), 0.0f);
		diffuseOutput += computeDiffuseBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);
		
	}

	vec3 ambient = vec3(0.0f) * pixelMaterial.albedo * pixelMaterial.ao;

	renderTarget1 = vec4(pixelMaterial.albedo, pixelMaterial.ao);
	renderTarget2 = vec4(specularOutput, pixelMaterial.metallic);
    renderTarget3 = vec4(worldNormal * 0.5f + 0.5f, pixelMaterial.roughness);
	renderTarget4 = vec4(ambient + diffuseOutput, 1.0f);

}
#endif // FRAGMENT_SHADER