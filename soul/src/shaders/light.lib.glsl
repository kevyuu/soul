#define MAX_DIR_LIGHT 4
#define MAX_POINT_LIGHT 100
#define MAX_SPOT_LIGHT 100

struct OmniLight {
	vec3 position;
	vec3 color;
};

struct DirectionalLight {
	mat4 shadowMatrix[4];
	vec3 direction;
	float bias;
	vec3 color;
	float preExposedIlluminance;
	vec4 cascadeDepths;
};

struct PointLight
{
	mat4 shadowMatrixes[6];
	vec3 position;
	float bias;
	vec3 color;
	float maxDistance;
};

struct SpotLight {
	mat4 shadowMatrix;
	vec3 position;
	float cosInner;
	vec3 direction;
	float cosOuter;
	vec3 color;
	float maxDistance;
	vec3 pad;
	float bias;
};


layout(std140) uniform LightData{
	
	DirectionalLight directionalLights[MAX_DIR_LIGHT];
	vec3 pad1;
	int directionalLightCount;

	PointLight pointLights[MAX_POINT_LIGHT];
	vec3 pad2;
	int pointLightCount;

	SpotLight spotLights[MAX_SPOT_LIGHT];
	vec3 pad3;
	int spotLightCount;

};

// reference: https://google.github.io/filament/Filament.md.html
float light_getDistanceAttenuation(vec3 posToLight, float invMaxDistance)
{
	float squareDistance = dot(posToLight, posToLight);
	float factor = squareDistance * invMaxDistance * invMaxDistance;
	float smoothFactor = max(1 - factor * factor, 0.0f);
	return smoothFactor * smoothFactor / max(squareDistance, 1e-4);

}

// reference: https://google.github.io/filament/Filament.md.html
float light_getAngleAttenuation(vec3 L, vec3 lightCenterDirection, float cosOuter, float cosInner)
{
	float scale = 1.0f / (cosInner - cosOuter);
	float numerator = dot(-L, lightCenterDirection) - cosOuter;
	float attenuation = clamp(numerator * scale, 0.0f, 1.0f);
	return attenuation * attenuation;
}