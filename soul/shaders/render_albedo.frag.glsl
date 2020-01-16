#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MaterialFlag_USE_ALBEDO_TEX (1u << 0)
#define MaterialFlag_USE_NORMAL_TEX (1u << 1)
#define MaterialFlag_USE_METALLIC_TEX (1u << 2)
#define MaterialFlag_USE_ROUGHNESS_TEX (1u << 3)
#define MaterialFlag_USE_AO_TEX (1u << 4)
#define MaterialFlag_USE_EMISSIVE_TEX (1u << 5)

#define MaterialFlag_METALLIC_CHANNEL_RED (1u << 8)
#define MaterialFlag_METALLIC_CHANNEL_GREEN (1u << 9)
#define MaterialFlag_METALLIC_CHANNEL_BLUE (1u << 10)
#define MaterialFlag_METALLIC_CHANNEL_ALPHA (1u << 11)

#define MaterialFlag_ROUGHNESS_CHANNEL_RED (1u << 12)
#define MaterialFlag_ROUGHNESS_CHANNEL_GREEN (1u << 13)
#define MaterialFlag_ROUGHNESS_CHANNEL_BLUE (1u << 14)
#define MaterialFlag_ROUGHNESS_CHANNEL_ALPHA (1u << 15)

#define MaterialFlag_AO_CHANNEL_RED (1u << 16)
#define MaterialFlag_AO_CHANNEL_GREEN (1u << 17)
#define MaterialFlag_AO_CHANNEL_BLUE (1u << 18)
#define MaterialFlag_AO_CHANNEL_ALPHA (1u << 19)

layout(set = 2, binding = 0) uniform sampler2D albedoMap;

struct Material {

	vec3 albedo;
	float metallic;
	vec3 emissive;
	float roughness;

	uint flags;
};

layout(set = 1, binding = 0) uniform PerMaterial {
	Material material;
};

struct PixelMaterial {
	vec3 f0;
	vec3 albedo;
	vec3 normal;
	float metallic;
	float roughness;
	float ao;
	vec3 emissive;
};

layout(location = 0) in VS_OUT {
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

layout(location = 0) out vec4 renderTarget1;

bool bitTest(uint flags, uint mask) {
	return ((flags & mask) == mask);
}

PixelMaterial pixelMaterialCreate(Material material, vec2 texCoord) {

	PixelMaterial pixelMaterial;

	pixelMaterial.albedo = pow(material.albedo, vec3(2.2));
	if (bitTest(material.flags, MaterialFlag_USE_ALBEDO_TEX)) {
		pixelMaterial.albedo *= pow(texture(albedoMap, texCoord).rgb, vec3(2.2));
	}

	return pixelMaterial;

}


void main() {
    PixelMaterial pixelMaterial = pixelMaterialCreate(material, vs_out.texCoord);
    renderTarget1 = vec4(pixelMaterial.albedo, 1.0f);
}