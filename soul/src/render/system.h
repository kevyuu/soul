#pragma once
#include "render/data.h"

namespace Soul { namespace Render {

	struct System {

		struct Config {
			int32 materialPoolSize = 1000;
			int32 meshPoolSize = 1000;
			int32 targetWidthPx = 1920;
			int32 targetHeightPx = 1080;

			VoxelGIConfig voxelGIConfig;
			ShadowAtlasConfig shadowAtlasConfig;
		};

		PanoramaToCubemapRP panoramaToCubemapRP;
		DiffuseEnvmapFilterRP diffuseEnvmapFilterRP;
		SpecularEnvmapFilterRP specularEnvmapFilterRP;
		BRDFMapRP brdfMapRP;
		VoxelizeRP voxelizeRP;

		void init(const Config& config);
		void shutdown();

		void shaderReload();

		TextureRID textureCreate(const TexSpec& spec, unsigned char* data, int dataChannelCount);

		void voxelGIVoxelize();
		void voxelGIUpdateConfig(const VoxelGIConfig& config);
		/*void voxelGIsetCenter(Vec3f center);
		void voxelGIsetHalfSpan(float haflSpan);
		void voxelGIsetResolution(int resolution);*/

		void shadowAtlasUpdateConfig(const ShadowAtlasConfig& config);

		void envSetAmbientEnergy(float ambientEnergy);
		void envSetAmbientColor(Vec3f ambientColor);
		void envSetPanorama(const float* data, int width, int height);
		void envSetSkybox(const SkyboxSpec& spec);

		MaterialRID materialCreate(const MaterialSpec& spec);
		void materialUpdate(MaterialRID rid, const MaterialSpec& spec);
		void materialSetMetallicTextureChannel(MaterialRID rid, TexChannel textureChannel);
		void materialSetRoughnessTextureChannel(MaterialRID rid, TexChannel textureChannel);
		void materialSetAOTextureChannel(MaterialRID rid, TexChannel textureChannel);
		void _materialUpdateFlag(MaterialRID rid, const MaterialSpec& spec);

		MeshRID meshCreate(const MeshSpec& spec);
		void meshDestroy(MeshRID rid);
		Mesh* meshPtr(MeshRID rid);
		void meshSetTransform(MeshRID rid, const Transform& transform);
		void meshSetTransform(MeshRID rid, const Mat4& transform);

		DirLightRID dirLightCreate(const DirectionalLightSpec& spec);
		void dirLightDestroy(DirLightRID lightRID);
		DirLight* dirLightPtr(DirLightRID lightRID);
		void dirLightSetDirection(DirLightRID lightRID, Vec3f direction);
		void dirLightSetColor(DirLightRID lightRID, Vec3f color);
		void dirLightSetShadowMapResolution(DirLightRID lightRID, int32 resolution);
		void dirLightSetCascadeSplit(DirLightRID lightRID, float split1, float split2, float split3);
		void dirLightSetBias(DirLightRID lightRID, float bias);

		void wireframePush(MeshRID meshRID);

		void render(const Camera& camera);

		//-------------------------------------------
		// private
		//-------------------------------------------
		Database _db;

		void _shadowAtlasInit();
		void _shadowAtlasCleanup();
		void _shadowAtlasFreeSlot(ShadowKey shadowKey);

		void _updateShadowMatrix();
		void _flushUBO();
		void _flushVoxelGIUBO();

		void _utilVAOInit();
		void _utilVAOCleanup();

		void _brdfMapInit();
		void _brdfMapCleanup();

		void _gBufferInit();
		void _gBufferCleanup();

		void _effectBufferInit();
		void _effectBufferCleanup();

		void _lightBufferInit();
		void _lightBufferCleanup();

		void _voxelGIBufferInit();
		void _voxelGIBufferCleanup();

		void _velocityBufferInit();
		void _velocityBufferCleanup();

		ShadowKey _shadowAtlasGetSlot(uint32 lightID, int texReso);

	};

}}