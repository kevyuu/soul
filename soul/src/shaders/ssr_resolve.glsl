#ifdef LIB_SHADER
math.lib.glsl
brdf.lib.glsl
sampling.lib.glsl
camera.lib.glsl
voxel_gi.lib.glsl
#endif

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

uniform sampler2D reflectionPosBuffer;
uniform sampler2D lightBuffer;

uniform sampler2D renderMap1;
uniform sampler2D renderMap2;
uniform sampler2D renderMap3;
uniform sampler2D renderMap4;
uniform sampler2D depthMap;
uniform sampler2D FGMap;

uniform sampler3D voxelLightBuffer;

uniform samplerCube diffuseEnvTex;
uniform samplerCube specularEnvTex;

uniform vec2 screenDimension;
uniform vec3 cameraPosition;

in VS_OUT {
	vec2 texCoord;
} vs_out;

out vec3 reflection;

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

vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}

void main() {

	voxel_gi_init();

    const vec2[4] sampleOffsets = vec2[](
        vec2(0, 0),
        vec2(-1, -1),
        vec2(1, -1),
        vec2(0, 1)
    );

    PixelMaterial pixelMaterial;

    pixelMaterial.albedo = texture(renderMap1, vs_out.texCoord).rgb;
    float ndcDepth = texture(depthMap, vs_out.texCoord).r * 2.0f - 1.0f;
	vec4 worldPositionHomo = camera_getInvProjectionViewMat() * vec4(vs_out.texCoord * 2.0f - 1.0f, ndcDepth, 1.0f);
	vec3 worldPosition = (worldPositionHomo / worldPositionHomo.w).xyz;
    pixelMaterial.metallic = texture(renderMap2, vs_out.texCoord).a;
    vec3 N = texture(renderMap3, vs_out.texCoord).rgb * 2.0f - 1.0f;
	N = normalize(N);
    pixelMaterial.roughness = texture(renderMap3, vs_out.texCoord).a;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, pixelMaterial.albedo, pixelMaterial.metallic);
    pixelMaterial.f0 = F0;
	pixelMaterial.ao = texture(renderMap4, vs_out.texCoord).a;

	vec3 specularOutput = texture(renderMap2, vs_out.texCoord).rgb;
	vec3 diffuseAmbientOutput = texture(renderMap4, vs_out.texCoord).rgb;
	vec3 directColor = specularOutput + diffuseAmbientOutput * pixelMaterial.ao;

	vec3 V = normalize(camera_getPosition() - worldPosition);

    vec3 reflectionColor = vec3(0);
    vec3 weightSum = vec3(0);

	float roughnessSquared = pixelMaterial.roughness * pixelMaterial.roughness;
    float NdotV = max(dot(N, V), 0);
	float cone_cos = cone_cosine(roughnessSquared);
	float cone_tan = sqrt(1 - cone_cos * cone_cos) / cone_cos;
	cone_tan *= mix(clamp(dot(N, -V) * 2.0, 0.0f, 1.0f), 1.0, pixelMaterial.roughness); /* Elongation fit */

	int numValidSample = 0;

    for (int i = 0; i < 4; i++) {

        vec2 sampleUV = sampleOffsets[i] / screenDimension + vs_out.texCoord;

        vec2 sampleReflectionPos = texture(reflectionPosBuffer, sampleUV).rg;

        if (sampleReflectionPos.xy == vec2(0, 0)) {
            continue;
        }

		numValidSample += 1;

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

        float pdf = DistributionGGX(N, H, sampleRoughness) * max(dot(N, H), 1e-8);
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

	reflectionAlpha = numValidSample == 4 ? reflectionAlpha : 0.0f;

	vec3 kD = 1.0f - F;
	kD *= 1.0 - pixelMaterial.metallic;

	vec3 diffuseEnv = kD * texture(diffuseEnvTex, N).rgb * pixelMaterial.albedo;

	vec4 diffuseVoxelTrace = voxel_gi_diffuseConeTrace(voxelLightBuffer, worldPosition, N);
	vec3 diffuseVoxelColor = voxel_gi_getDiffuseMultiplier() * kD * diffuseVoxelTrace.rgb * pixelMaterial.albedo / M_PI;
	vec3 diffuseIndirectColor = diffuseVoxelColor + (1 - diffuseVoxelTrace.a) * diffuseEnv;

	vec3 L = normalize(getSpecularDominantDir(N, reflect(-V, N), pixelMaterial.roughness));
	vec3 H = normalize((L + V) / 2.0f);
	float pdf = DistributionGGX(N, H, pixelMaterial.roughness) * max(dot(N, H), 0);

	const float MAX_SPECULAR_ENV_LOD = 4;
	vec3 specularEnv = textureLod(specularEnvTex, L, pixelMaterial.roughness * MAX_SPECULAR_ENV_LOD).rgb * FG;

	vec4 specularVoxel = voxel_gi_coneTrace(
		voxelLightBuffer,
		worldPosition,
		L,
		cone_tan
	);
	
	vec3 specularIndirectColor = voxel_gi_getSpecularMultiplier() * specularVoxel.xyz * computeSpecularBRDF(L, V, N, pixelMaterial) * max(dot(N, L), 0) / max(pdf, 1e-8);
	specularIndirectColor += ((1 - specularVoxel.a) * specularEnv);
	
	vec3 color = directColor + (diffuseIndirectColor + specularIndirectColor) * pixelMaterial.ao;

	color = ACESFilm(color);
	color = pow(color, vec3(1.0f / 2.2));

	reflection = color;
}
#endif // FRAGMENT_SHADER