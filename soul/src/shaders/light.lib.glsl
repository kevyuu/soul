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

layout(std140) uniform LightData{
	DirectionalLight directionalLights[4];
	int directionalLightCount;
	float pad1;
	float pad2;
	float pad3;
};
