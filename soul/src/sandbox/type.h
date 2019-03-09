#pragma once

#include "core/hash_map.h"

#include "render/type.h"
#include "render/system.h"

struct DirLightConfig {
	Soul::Vec3f dir;
	Soul::Vec3f color;
	int32 resolution;
	float split[3];
	float energy;
	float bias;
};

struct UITexture {
	
	char name[512];
	Soul::RenderRID rid;

};

struct UIMaterial {
	
	char name[512];
	Soul::RenderRID rid;

	uint32 albedoTexID;
	uint32 normalTexID;
	uint32 metallicTexID;
	uint32 roughnessTexID;
	
	Soul::Vec3f albedo;
	float metallic;
	float roughness;

	bool useAlbedoTex;
	bool useNormalTex;
	bool useMetallicTex;
	bool useRoughnessTex;

	Soul::TextureChannel metallicTextureChannel;
	Soul::TextureChannel roughnessTextureChannel;
};

struct UIMesh {
	
	char name[512];
	Soul::RenderRID rid;

	Soul::Vec3f scale;
	Soul::Vec3f position;
	Soul::Vec4f rotation;

	uint32 materialID;

};

struct SceneData {

	Soul::RenderSystem renderSystem;
	Soul::RenderSystem::Config renderConfig;
	DirLightConfig dirLightConfig;

	Soul::Camera camera;
	Soul::RenderRID sunRID;

	Soul::Array<UITexture> textures;
	Soul::Array<UIMaterial> materials;
	Soul::Array<UIMesh> meshes;

	char objFilePath[1000];
	char mtlDirPath[1000];
	char gltfFilePath[1000];

};