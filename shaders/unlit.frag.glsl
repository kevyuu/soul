#version 450
#extension GL_ARB_separate_shader_objects : enable

struct SunLight {
	mat4 shadowMatrix[4];
	vec3 direction;
	float bias;
	vec3 color;
	float pad1;
	vec4 cascadeDepths;
};

struct Camera {
	mat4 projection;
	mat4 view;
	mat4 projectionView;
	mat4 invProjectionView;

	vec3 position;
};

layout(set = 0, binding = 0, std140) uniform SceneData{
	Camera camera;
	SunLight light;
	uint sunlightCount;
};

layout(set = 0, binding =1) uniform sampler2D shadowMap;

layout (location = 0) in VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
} vs_out;

layout(location = 0) out vec4 outColor;

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

	float cascadeSplit[5] = {
		0,
		light.cascadeDepths.x,
		light.cascadeDepths.y,
		light.cascadeDepths.z,
		light.cascadeDepths.w
	};

	vec3 worldPosition = vs_out.worldPosition;

	vec3 fragViewCoord = vec4(camera.view * vec4(worldPosition, 1.0f)).xyz;

	float fragDepth = -1 * fragViewCoord.z;

	int cascadeIndex = -1;
	float shadowFactor = 0;

	for (int i = 0; i < 5; i++) {
		if (fragDepth > cascadeSplit[i]) cascadeIndex = i;
	}

	float cascadeBlendRange = 1.0f;
	float bias = light.bias;

	vec3 colorMarker = vec3(1, 0.2f, 0.2f);
	if (cascadeIndex >= 3) {
		colorMarker = vec3(1, 0.2f, 0.2f);
		shadowFactor = 0;
	}
	if (cascadeIndex == 3) {
		colorMarker = vec3(0.5f, 1, 0.5f);
		shadowFactor = computeShadowFactor(worldPosition, light.shadowMatrix[cascadeIndex], bias);
	}
	else {
		if (cascadeIndex == 2) {
			colorMarker = vec3(0.5f, 0.5f, 1.0f);
		} else if (cascadeIndex == 1) {
			colorMarker = vec3(0.5f, 1.0f, 1.0f);
		} else if (cascadeIndex == 0) {
			colorMarker = vec3(1.0f, 0.0f, 0.0f);
		}
		float shadowFactor1 = computeShadowFactor(worldPosition, light.shadowMatrix[cascadeIndex], bias);
		float shadowFactor2 = computeShadowFactor(worldPosition, light.shadowMatrix[cascadeIndex + 1], bias);

		float cascadeDist = cascadeSplit[cascadeIndex + 1] - cascadeSplit[cascadeIndex];
		float cascadeBlend = smoothstep(cascadeSplit[cascadeIndex] + (1 - cascadeBlendRange) * cascadeDist, cascadeSplit[cascadeIndex + 1], fragDepth);
		shadowFactor = mix(shadowFactor1, shadowFactor2, cascadeBlend);
	}


    vec3 objectColor = vec3(0.6f, 0.6f, 0.6f);
    vec3 ambient = 0.1f * light.color;
    
    float diff = max(dot(vs_out.worldNormal, light.direction * -1), 0.0f);
    vec3 diffuse = diff * light.color;


    vec3 viewDir = normalize(camera.position - vs_out.worldPosition);
    vec3 reflectDir = reflect(light.direction, vs_out.worldNormal);
    float specularStrength = 0.5f;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    vec3 specular = specularStrength * spec * light.color;

	vec3 illuminance = (1 - shadowFactor) * (diffuse + specular);
    vec3 result = (ambient + illuminance) * objectColor * colorMarker;

    outColor = vec4(result, 1.0f);
}