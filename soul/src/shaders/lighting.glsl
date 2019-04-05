#ifdef LIB_SHADER
math.lib.glsl
brdf.lib.glsl
light.lib.glsl
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

layout(std140) uniform SceneData {
	mat4 projection;
	mat4 view;
};

uniform sampler2D shadowMap;
uniform sampler2D renderMap1;
uniform sampler2D renderMap2;
uniform sampler2D renderMap3;

uniform vec3 viewPosition;

in VS_OUT {
	vec2 texCoord;
} vs_out;

out vec4 fragColor;

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

    pixelMaterial.albedo = texture(renderMap1, vs_out.texCoord).rgb;
    pixelMaterial.ao = texture(renderMap1, vs_out.texCoord).a;
	vec3 worldPosition = texture(renderMap2, vs_out.texCoord).rgb;
	pixelMaterial.metallic = texture(renderMap2, vs_out.texCoord).a;
	vec3 N = texture(renderMap3, vs_out.texCoord).rgb * 2.0f - 1.0f;
	pixelMaterial.roughness = texture(renderMap3, vs_out.texCoord).a;

	vec3 V = normalize(viewPosition - worldPosition);

	vec3 F0 = vec3(0.04);
    F0 = mix(F0, pixelMaterial.albedo, pixelMaterial.metallic);
	pixelMaterial.f0 = F0;

	vec3 fragViewCoord = vec4(view * vec4(worldPosition, 1.0f)).xyz;

	fragColor = vec4(0, 0, 0, 0);
	vec3 Lo = vec3(0);

	vec3 specularOutput = vec3(0.0f);
	vec3 diffuseOutput = vec3(0.0f);

	for (int i = 0; i < directionalLightCount; i++) {
		vec3 L = normalize(directionalLights[i].direction * -1);

		vec3 radiance = directionalLights[i].color;

		float cascadeSplit[5] = {
			0,
			directionalLights[i].cascadeDepths.x,
			directionalLights[i].cascadeDepths.y,
			directionalLights[i].cascadeDepths.z,
			directionalLights[i].cascadeDepths.w
		};

		float shadowFactor = 0.0f;

		float fragDepth = fragViewCoord.z * -1;

		int cascadeIndex = -1;

		for (int i = 0; i < 5; i++) {
			if (fragDepth > cascadeSplit[i]) cascadeIndex = i;
		}

		float cascadeBlendRange = 1.0f;
		float bias = directionalLights[i].bias;

		if (cascadeIndex >= 3) {
			shadowFactor = 0;
		}
		if (cascadeIndex == 3) {
			shadowFactor = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix[cascadeIndex], bias);
		}
		else {
			float shadowFactor1 = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix[cascadeIndex], bias);
			float shadowFactor2 = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix[cascadeIndex + 1], bias);

			float cascadeDist = cascadeSplit[cascadeIndex + 1] - cascadeSplit[cascadeIndex];
			float cascadeBlend = smoothstep(cascadeSplit[cascadeIndex] + (1 - cascadeBlendRange) * cascadeDist, cascadeSplit[cascadeIndex + 1], fragDepth);
			shadowFactor = mix(shadowFactor1, shadowFactor2, cascadeBlend);
		}

		radiance = radiance * (1.0f - shadowFactor);

		specularOutput += computeSpecularBRDF(L, V, worldNormal, pixelMaterial) * radiance *  max(dot(worldNormal, L), 0.0f);
		diffuseOutput += computeDiffuseBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);
	}

    vec3 ambient = vec3(0.06f) * pixelMaterial.albedo * pixelMaterial.ao;

	vec3 color = ambient + Lo;
    fragColor = vec4(color, 1.0f);

}
#endif // FRAGMENT_SHADER