#ifdef LIB_SHADER
math.lib.glsl
brdf.lib.glsl
#endif


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
};

struct OmniLight {
	vec3 position;
	vec3 color;
};

struct DirectionalLight {
	mat4 shadowMatrix[4];
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
uniform sampler2DShadow shadowMap;
uniform vec3 viewPosition;
uniform vec3 ambientFactor;

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

float computeShadowFactor(vec3 worldPosition, mat4 shadowMatrix) {
	if (directionalLightCount == 0) return 0;
	
	float bias = 0.008f;

	vec4 lightSpacePosition = shadowMatrix * vec4(worldPosition, 1.0f);
	
	float refDepth = lightSpacePosition.z - bias;	
	vec4 shadowCoords = lightSpacePosition / lightSpacePosition.w;
	refDepth = refDepth / lightSpacePosition.w;
	refDepth = refDepth * 0.5f + 0.5f;
	shadowCoords = shadowCoords * 0.5 + 0.5;

	float shadowFactor = 1;

    float shadowFactorPCF = 0.0f;
    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            shadowFactorPCF += textureProj(shadowMap, vec4(shadowCoords.xy + vec2(i, j) * texelSize, refDepth, 1.0f)).r;
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
	pixelMaterial.f0 = mix(vec3(0.04f), pixelMaterial.albedo, pixelMaterial.metallic);

	mat3 TBN = mat3(vs_out.worldTangent, vs_out.worldBinormal, vs_out.worldNormal);
    vec3 worldNormal = normalize(TBN * pixelMaterial.normal);
	vec3 worldPosition = vs_out.worldPosition;
	vec3 V = normalize(viewPosition - worldPosition);
	vec3 fragViewCoord = vec4(view * vec4(worldPosition, 1.0f)).xyz;
	
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
		
		float cascadeBlendRange = 0.5f;

		if (cascadeIndex >= 3) {
			shadowFactor = 0;
		}
		if (cascadeIndex == 3) {
			shadowFactor = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix[cascadeIndex]);
		}
		else {
			float shadowFactor1 = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix[cascadeIndex]);
			float shadowFactor2 = computeShadowFactor(worldPosition, directionalLights[i].shadowMatrix[cascadeIndex + 1]);

			float cascadeDist = cascadeSplit[cascadeIndex + 1] - cascadeSplit[cascadeIndex];
			float cascadeBlend = smoothstep(cascadeSplit[cascadeIndex] + (1 - cascadeBlendRange) * cascadeDist, cascadeSplit[cascadeIndex + 1], fragDepth);
			shadowFactor = mix(shadowFactor1, shadowFactor2, cascadeBlend);
		}

		radiance = radiance * (1.0f - shadowFactor);

		specularOutput += computeSpecularBRDF(L, V, worldNormal, pixelMaterial) * radiance *  max(dot(worldNormal, L), 0.0f);
		diffuseOutput += computeDiffuseBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);
		
	}

	vec3 ambient = ambientFactor * pixelMaterial.albedo;

	renderTarget1 = vec4(pixelMaterial.albedo, 1.0f);
	renderTarget2 = vec4(specularOutput, pixelMaterial.metallic);
    renderTarget3 = vec4(worldNormal * 0.5f + 0.5f, pixelMaterial.roughness);
	renderTarget4 = vec4(ambient + diffuseOutput, 1.0f);

}
#endif // FRAGMENT_SHADER