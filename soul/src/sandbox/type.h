#pragma once

#include "core/hash_map.h"

#include "render/data.h"
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
	Soul::Render::RID rid;

};

struct UIMaterial {
	
	char name[512];
	Soul::Render::RID rid;

	uint32 albedoTexID;
	uint32 normalTexID;
	uint32 metallicTexID;
	uint32 roughnessTexID;
	uint32 aoTexID;
	
	Soul::Vec3f albedo;
	float metallic;
	float roughness;

	bool useAlbedoTex;
	bool useNormalTex;
	bool useMetallicTex;
	bool useRoughnessTex;
	bool useAOTex;

	Soul::Render::TexChannel metallicTextureChannel;
	Soul::Render::TexChannel roughnessTextureChannel;
	Soul::Render::TexChannel aoTextureChannel;
};

struct UIMesh {
	
	char name[512];
	Soul::Render::RID rid;

	Soul::Vec3f scale;
	Soul::Vec3f position;
	Soul::Vec4f rotation;

	uint32 materialID;

};

struct SceneData {

	Soul::Render::System renderSystem;
	Soul::Render::System::Config renderConfig;
	DirLightConfig dirLightConfig;

	Soul::Render::Camera camera;
	Soul::Render::RID sunRID;

	Soul::Array<UITexture> textures;
	Soul::Array<UIMaterial> materials;
	Soul::Array<UIMesh> meshes;

	char objFilePath[1000];
	char mtlDirPath[1000];
	char gltfFilePath[1000];

};