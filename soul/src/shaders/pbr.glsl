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
	vec3 viewPosition;
	vec2 texCoord;
	mat3 BTN;
} vs_out;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vs_out.worldPosition = vec3((model * vec4(aPos, 1.0f)).xyz);
	vs_out.worldNormal = mat3(transpose(inverse(model))) * aNormal;
	vs_out.viewPosition = vec3(view * model * vec4(aPos, 1.0f)).xyz;
	vs_out.texCoord = aTexCoord;
	mat3 rotMat = mat3(model);
	vec3 T = normalize(rotMat * aTangent);
	vec3 B = normalize(rotMat * aBinormal);
	vec3 N = normalize(rotMat * aNormal);
	vs_out.BTN = mat3(B, T, N);
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

struct Environment {
    vec3 ambientColor;
    float ambientEnergy;
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

layout (std140) uniform LightData {
	DirectionalLight directionalLights[4];
	int directionalLightCount;
};

uniform Material material;
uniform Environment environment;
uniform sampler2D shadowMap;
uniform sampler2D brdfMap;
uniform samplerCube diffuseMap;
uniform samplerCube specularMap;

uniform vec3 viewPosition;

in VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 viewPosition;
	vec2 texCoord;
	mat3 BTN;
} vs_out;

out vec4 fragColor;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

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

vec3 computeOutgoingRadiance(vec3 L, vec3 V, vec3 N, PixelMaterial pixelMaterial, vec3 radiance) {
	float PI = 3.14;
	vec3 H = normalize(L + V);

	float NDF = DistributionGGX(N, H, pixelMaterial.roughness);
	float G = GeometrySmith(N, V, L, pixelMaterial.roughness);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), pixelMaterial.f0);

	vec3 ks = F;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - pixelMaterial.metallic;

	float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);

	vec3 specular = (F * G * NDF) / max(denominator, 0.001);
	vec3 diffuse = kd * pixelMaterial.albedo / PI;

	return (specular + diffuse) * radiance * max(dot(N, L), 0.0);
}

float random(vec3 seed, int i) {
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float computeShadowFactor(mat4 shadowMatrix) {
	if (directionalLightCount == 0) return 0;

	vec4 lightSpacePosition = shadowMatrix * vec4(vs_out.worldPosition, 1.0f);
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

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    vec3 res = (x*(a*x+b))/(x*(c*x+d)+e);
    return clamp(res, 0.0f, 1.0f);
}

void main() {

	float PI = 3.14;

	fragColor = vec4(0, 0, 0, 0);
	vec3 V = normalize(viewPosition - vs_out.worldPosition);

	PixelMaterial pixelMaterial;

	pixelMaterial.albedo = pow(texture(material.albedoMap, vs_out.texCoord).rgb, vec3(2.2));
	pixelMaterial.normal = vec3(texture(material.normalMap, vs_out.texCoord).rgb * 2 - 1.0f);
	pixelMaterial.metallic = texture(material.metallicMap, vs_out.texCoord).r;
	pixelMaterial.roughness = texture(material.roughnessMap, vs_out.texCoord).r;
	pixelMaterial.ao = texture(material.aoMap, vs_out.texCoord).r;
	vec3 F0 = vec3(0.04);
    F0 = mix(F0, pixelMaterial.albedo, pixelMaterial.metallic);
	pixelMaterial.f0 = F0;

	vec3 N = normalize(vs_out.BTN * pixelMaterial.normal);


	vec3 Lo = vec3(0);

	for (int i = 0; i < directionalLightCount; i++) {
		vec3 L = directionalLights[i].direction * -1;
		vec3 H = normalize(L + V);
		vec3 radiance = directionalLights[i].color;

		vec4 cascadeDepths = directionalLights[i].cascadeDepths;

		float shadowFactor = 0.0f;

		float fragDepth = vs_out.viewPosition.z * -1;

		if (fragDepth < cascadeDepths.x) {
			shadowFactor = computeShadowFactor(directionalLights[i].shadowMatrix1);
		} else if (fragDepth < cascadeDepths.y) {
			shadowFactor = computeShadowFactor(directionalLights[i].shadowMatrix2);
		} else if (fragDepth < cascadeDepths.z) {
			shadowFactor = computeShadowFactor(directionalLights[i].shadowMatrix3);
		} else if (fragDepth < cascadeDepths.w) {
			shadowFactor = computeShadowFactor(directionalLights[i].shadowMatrix4);
		}

		radiance = radiance * (1.0f - shadowFactor);
		Lo += computeOutgoingRadiance(L, V, N, pixelMaterial, radiance);
	}

    vec3 ambient = environment.ambientColor * pixelMaterial.albedo * pixelMaterial.ao * environment.ambientEnergy;
	vec3 color = ambient + Lo;

    float exposure = 3.0;
    vec3 mapped = vec3(1.0) - exp(-color * exposure);
    color = pow(mapped, vec3(1.0/2.2));

    fragColor = vec4(color, 1.0f);
}
#endif // FRAGMENT_SHADER