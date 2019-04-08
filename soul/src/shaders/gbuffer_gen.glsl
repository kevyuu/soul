#ifdef LIB_SHADER
math.lib.glsl
brdf.lib.glsl
light.lib.glsl
camera.lib.glsl
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

uniform mat4 model;

out VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

void main() {
	gl_Position = camera_getProjectionViewMat() * model * vec4(aPos, 1.0f);
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

float computeShadowFactor(vec3 worldPosition, mat4 shadowMatrix, float bias) {
	if (directionalLightCount == 0) return 0;
	
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

	PixelMaterial pixelMaterial = pixelMaterialCreate(material, vs_out.texCoord);
	
	mat3 TBN = mat3(vs_out.worldTangent, vs_out.worldBinormal, vs_out.worldNormal);
    vec3 worldNormal = normalize(TBN * pixelMaterial.normal);
	vec3 worldPosition = vs_out.worldPosition;
	vec3 V = normalize(viewPosition - worldPosition);
	vec3 fragViewCoord = vec4(camera_getViewMat() * vec4(worldPosition, 1.0f)).xyz;
	
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

	for (int i = 0; i < pointLightCount; i++)
	{
		vec3 posToLight = worldPosition - pointLights[i].position;
		vec3 L = normalize(-posToLight);

		// calculate shadow map faces to use
		// TODO: Can we optimize it? by removing branch?
		int shadowIndex;
		float absX = abs(L.x);
		float absY = abs(L.y);
		float absZ = abs(L.z);
		if (absX > absY && absX > absZ) {
			if (L.x < 0.0f) {
				shadowIndex = 3;	
			} else {
				shadowIndex = 0;
			}
		} else if (absY > absX && absY > absZ) {
			if (L.y < 0.0f) {
				shadowIndex = 4;
			} else {
				shadowIndex = 1;
			}
		} else {
			if (L.z < 0.0f) {
				shadowIndex = 5;
			} else {
				shadowIndex = 2;
			}
		}

		float shadowFactor = computeShadowFactor(worldPosition, pointLights[i].shadowMatrixes[shadowIndex], pointLights[i].bias);
		float distanceAttenuation = light_getDistanceAttenuation(posToLight, 1.0f / pointLights[i].maxDistance);
		vec3 radiance = pointLights[i].color * distanceAttenuation * (1 - shadowFactor);

		specularOutput += computeSpecularBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);
		diffuseOutput += computeDiffuseBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);

	}

	for (int i = 0; i < spotLightCount; i++)
	{
		vec3 posToLight = worldPosition - spotLights[i].position;
		vec3 L = normalize(-posToLight);
		
		float shadowFactor = computeShadowFactor(worldPosition, spotLights[i].shadowMatrix, spotLights[i].bias);
		float distanceAttenuation = light_getDistanceAttenuation(posToLight, 1.0f / spotLights[i].maxDistance);
		float angleAttenuation = light_getAngleAttenuation(L, spotLights[i].direction, spotLights[i].cosOuter, spotLights[i].cosInner);
		vec3 radiance = spotLights[i].color * distanceAttenuation * angleAttenuation * (1.0f - shadowFactor);

		specularOutput += computeSpecularBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);
		diffuseOutput += computeDiffuseBRDF(L, V, worldNormal, pixelMaterial) * radiance * max(dot(worldNormal, L), 0.0f);
		
	}

	vec3 ambient = ambientFactor * pixelMaterial.albedo;

	renderTarget1 = vec4(pixelMaterial.albedo, 1.0f);
	renderTarget2 = vec4(specularOutput, pixelMaterial.metallic);
    renderTarget3 = vec4(worldNormal * 0.5f + 0.5f, pixelMaterial.roughness);
	renderTarget4 = vec4(ambient + diffuseOutput + (pixelMaterial.emissive * 5.0f), pixelMaterial.ao);

}
#endif // FRAGMENT_SHADER