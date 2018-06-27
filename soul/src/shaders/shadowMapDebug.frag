#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D shadowMap;

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


void main()
{             
    float depthValue = texture(shadowMap, TexCoords).r;
    FragColor = vec4(vec3(0.0f, 0.0f, 1.0f), 1.0f);
}