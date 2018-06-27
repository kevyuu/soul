#define M_PI 3.1415926535897932384626433832795

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout(location = 0) in vec2 aPos;

out VS_OUT {
    vec2 texCoord;
} vs_out;

void main() {
    gl_Position = vec4(aPos, 0.0f, 1.0f);
    vs_out.texCoord = (aPos - vec2(-1.0f, -1.0f)) / 2.0f;
}
#endif //VERTEX SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

#define MAX_STEP 64

uniform sampler2D reflectionPosBuffer;
uniform sampler2D lightBuffer;

uniform sampler2D renderMap1;
uniform sampler2D renderMap2;
uniform sampler2D renderMap3;
uniform sampler2D renderMap4;
uniform sampler2D depthMap;
uniform sampler2D FGMap;
uniform sampler3D voxelLightBuffer;

uniform vec2 screenDimension;
uniform vec3 cameraPosition;
uniform mat4 invProjectionView;

uniform int voxelFrustumReso;
uniform float voxelFrustumHalfSpan;
uniform vec3 voxelFrustumCenter;

in VS_OUT {
	vec2 texCoord;
} vs_out;

out vec3 reflection;

struct PixelMaterial {
	vec3 f0;
	vec3 albedo;
	vec3 normal;
	float metallic;
	float roughness;
	float ao;
};

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


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

vec3 computeSpecularBRDF(vec3 L, vec3 V, vec3 N, PixelMaterial pixelMaterial) {
    vec3 H = normalize(L + V);

    float NDF = DistributionGGX(N, H, pixelMaterial.roughness);
    float G = GeometrySmith(N, V, L, pixelMaterial.roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), pixelMaterial.f0);

    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);

    vec3 specular = (F * G * NDF) / max(denominator, 0.001);

    return specular;

}

vec3 voxelConeTrace(sampler3D voxelLightBuffer, 
	vec3 position, vec3 direction, float coneTangent,
	float voxelSize, 
	vec3 voxelFrustumMax, 
	vec3 voxelFrustumMin) {
	vec3 startPos = position + direction * voxelSize;

	vec3 color = vec3(0.0f);
	float alpha = 0.0f;
	vec3 samplePos = startPos;
	while (all(lessThanEqual(samplePos, voxelFrustumMax)) && all(greaterThanEqual(samplePos, voxelFrustumMin)) && alpha < 0.99f) {
		float radius = max(voxelSize / 2, coneTangent * length(samplePos - position));
		float mip = min(log2(2 * radius / voxelSize), 7);

		vec3 samplePosUVW = (samplePos - voxelFrustumMin) / (voxelFrustumMax - voxelFrustumMin);
		vec4 sampleColor = textureLod(voxelLightBuffer, samplePosUVW, mip);
		color += (1 - alpha) * sampleColor.rgb;
		alpha += (1 - alpha) * sampleColor.a;

		samplePos += radius * direction;
	}
	
	return color + (1 - alpha) * vec3(0.01, 0.01, 0.04);

}

const vec3 diffuseConeDirections[] =
{
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 0.5f, 0.866025f),
	vec3(0.823639f, 0.5f, 0.267617f),
	vec3(0.509037f, 0.5f, -0.7006629f),
	vec3(-0.50937f, 0.5f, -0.7006629f),
	vec3(-0.823639f, 0.5f, 0.267617f)
};

const float diffuseConeWeights[] =
{
	M_PI / 4.0f,
	3.0f * M_PI / 20.0f,
	3.0f * M_PI / 20.0f,
	3.0f * M_PI / 20.0f,
	3.0f * M_PI / 20.0f,
	3.0f * M_PI / 20.0f,
};

vec3 voxelCalculateDiffuseIrradiance(sampler3D voxelLightBuffer,
	vec3 position,
	vec3 normal,
	float voxelSize,
	vec3 voxelFrustumMax,
	vec3 voxelFrustumMin) {

	vec3 ref = vec3(0.0f, 1.0f, 0.0f);

	if (abs(dot(normal, ref)) == 1.0f) {
		ref = vec3(0.0f, 0.0f, 1.0f);
	}

	vec3 tangent = cross(normal, ref);
	vec3 bitangent = cross(normal, tangent);
		
	mat3 rotationMat = mat3(tangent, normal, bitangent);

	vec3 color = vec3(0.0f);
	for (int i = 0; i < 6; i++) {
		vec3 L = normalize(rotationMat * diffuseConeDirections[i]);
		color += voxelConeTrace(voxelLightBuffer,
			position + normal * voxelSize,
			L,
			0.57735f,
			voxelSize,
			voxelFrustumMax,
			voxelFrustumMin) * max(dot(normal, L), 0.0f) ;
	}

	color *= 2 * M_PI / 6;

	return color;
}
vec3 getSpecularDominantDir(vec3 N, vec3 R, float roughness)
{
	float smoothness = clamp(1 - roughness, 0.0f, 1.0f);
	float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
	// The result is not normalized as we fetch in a cubemap
	return mix(N, R, lerpFactor);
}

float cone_cosine(float r)
{
	/* Using phong gloss
	 * roughness = sqrt(2/(gloss+2)) */
	float gloss = -2 + 2 / (r * r);
	/* Drobot 2014 in GPUPro5 */
	// return cos(2.0 * sqrt(2.0 / (gloss + 2)));
	/* Uludag 2014 in GPUPro5 */
	// return pow(0.244, 1 / (gloss + 1));
	/* Jimenez 2016 in Practical Realtime Strategies for Accurate Indirect Occlusion*/
	return exp2(-3.32193 * r * r);
}

void main() {

    const vec2[4] sampleOffsets = vec2[](
        vec2(0, 0),
        vec2(-1, -1),
        vec2(1, -1),
        vec2(0, 1)
    );

    PixelMaterial pixelMaterial;

    pixelMaterial.albedo = texture(renderMap1, vs_out.texCoord).rgb;
    pixelMaterial.ao = texture(renderMap1, vs_out.texCoord).a;
    float ndcDepth = texture(depthMap, vs_out.texCoord).r * 2.0f - 1.0f;
	vec4 worldPositionHomo = invProjectionView * vec4(vs_out.texCoord * 2.0f - 1.0f, ndcDepth, 1.0f);
	vec3 worldPosition = (worldPositionHomo / worldPositionHomo.w).xyz;
    pixelMaterial.metallic = texture(renderMap2, vs_out.texCoord).a;
    vec3 N = texture(renderMap3, vs_out.texCoord).rgb * 2.0f - 1.0f;
    pixelMaterial.roughness = texture(renderMap3, vs_out.texCoord).a;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, pixelMaterial.albedo, pixelMaterial.metallic);
    pixelMaterial.f0 = F0;

	vec3 specularOutput = texture(renderMap2, vs_out.texCoord).rgb;
	vec3 diffuseAmbientOutput = texture(renderMap4, vs_out.texCoord).rgb;
	vec3 directColor = specularOutput + diffuseAmbientOutput;

    vec3 V = normalize(cameraPosition - worldPosition);

    vec3 reflectionColor = vec3(0);
    vec3 weightSum = vec3(0);

	float roughnessSquared = pixelMaterial.roughness * pixelMaterial.roughness;
    float NdotV = max(dot(N, V), 0);
	float cone_cos = cone_cosine(roughnessSquared);
	float cone_tan = sqrt(1 - cone_cos * cone_cos) / cone_cos;
	cone_tan *= mix(clamp(dot(N, -V) * 2.0, 0.0f, 1.0f), 1.0, pixelMaterial.roughness); /* Elongation fit */

    for (int i = 0; i < 4; i++) {

        vec2 sampleUV = sampleOffsets[i] / screenDimension + vs_out.texCoord;

        vec2 sampleReflectionPos = texture(reflectionPosBuffer, sampleUV).rg;

        if (sampleReflectionPos.xy == vec2(0, 0)) {
            continue;
        }

        float circleRadius = cone_tan * length(sampleReflectionPos - vs_out.texCoord);
        float mip = max(log2(circleRadius * max(screenDimension.x, screenDimension.y)), 0);

        vec3 radiance = textureLod(lightBuffer, sampleReflectionPos, mip).rgb;

        float attenuateConst = 1.0f;
        float attenuationLinear = 0.0001f;
        float attenuationExp = 0.0001f;

        vec3 reflectionWorldPos = texture(renderMap2, sampleReflectionPos).rgb;
        float distance = length(reflectionWorldPos);
        float attenuation = attenuateConst + attenuationLinear * distance + attenuationExp * distance * distance;
        radiance = radiance / attenuation;
        vec3 L = normalize(reflectionWorldPos - worldPosition);
        vec3 H = normalize((V + L) / 2);

        float sampleRoughness = texture(renderMap3, sampleUV).a;

        float pdf = DistributionGGX(N, H, sampleRoughness) * max(dot(N, H), 0.0f);
        vec3 weight = computeSpecularBRDF(L, V, N, pixelMaterial) * max(dot(N, L), 0.0f) / pdf;
        reflectionColor += radiance * weight;
        weightSum += weight;

    }

    vec2 FGFactor = texture(FGMap, vec2(max(dot(N, V), 0.0f), pixelMaterial.roughness)).rg;
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), pixelMaterial.f0, pixelMaterial.roughness);
    vec3 FG = F * FGFactor.x + FGFactor.y;

    vec2 reflectionPos = texture(reflectionPosBuffer, vs_out.texCoord).rg;
    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - reflectionPos.xy));

	float reflectionAlpha = 0.0f;
    if (weightSum != vec3(0)) {
       reflectionColor = reflectionColor * FG * clamp(1.0f - (dCoords.x + dCoords.y), 0.0f, 1.0f) / weightSum;
       reflectionColor = max(reflectionColor, vec3(0));
	   reflectionAlpha = clamp(1.0f - (dCoords.x + dCoords.y), 0.0f, 1.0f);
    }

	vec3 voxelFrustumMax = voxelFrustumCenter + vec3(voxelFrustumHalfSpan);
	vec3 voxelFrustumMin = voxelFrustumCenter - vec3(voxelFrustumHalfSpan);
	float voxelSize = 2 * voxelFrustumHalfSpan / float(voxelFrustumReso);

	vec3 kD = 1.0f - F;

	vec3 diffuseIndirectColor = kD * voxelCalculateDiffuseIrradiance(
		voxelLightBuffer,
		worldPosition,
		N,
		voxelSize,
		voxelFrustumMax,
		voxelFrustumMin
	) * pixelMaterial.albedo / M_PI;

	vec3 L = normalize(getSpecularDominantDir(N, reflect(-V, N), pixelMaterial.roughness));
	L = reflect(-V, N);
	vec3 H = normalize((L + V) / 2.0f);
	float pdf = 1.5f * DistributionGGX(N, H, pixelMaterial.roughness) * max(dot(N, H), 0.0f);
	vec3 specularIndirectColor = voxelConeTrace(
		voxelLightBuffer,
		worldPosition,
		L,
		cone_tan,
		voxelSize,
		voxelFrustumMax,
		voxelFrustumMin
	) * computeSpecularBRDF(L, V, N, pixelMaterial) * max(dot(N, L), 0.0f) / pdf;

    vec3 color = directColor + diffuseIndirectColor + reflectionColor + (1 - reflectionAlpha) * specularIndirectColor;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    reflection = color;
}
#endif // FRAGMENT_SHADER