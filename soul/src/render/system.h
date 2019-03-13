#pragma once
#include "render/data.h"

namespace Soul { namespace Render {

	struct System {

		struct ShadowAtlasConfig {
			int32 resolution;
			int8 subdivSqrtCount[4] = { 1, 2, 4, 8 };
		};

		struct Config {
			int32 materialPoolSize = 128;
			int32 meshPoolSize = 128;
			int32 shadowAtlasRes;
			int32 targetWidthPx = 1920;
			int32 targetHeightPx = 1080;
			int8 subdivSqrtCount[4] = { 1, 2, 4, 8 };

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

		RID textureCreate(const TexSpec& spec, unsigned char* data, int dataChannelCount);

		RID dirLightCreate(const DirectionalLightSpec& spec);
		void dirLightSetDirection(RID lightRID, Vec3f direction);
		void dirLightSetColor(RID lightRID, Vec3f color);
		void dirLightSetShadowMapResolution(RID lightRID, int32 resolution);
		void dirLightSetCascadeSplit(RID lightRID, float split1, float split2, float split3);
		void dirLightSetBias(RID lightRID, float bias);

		void voxelGIVoxelize();
		void voxelGIUpdateConfig(const VoxelGIConfig& config);
		/*void voxelGIsetCenter(Vec3f center);
		void voxelGIsetHalfSpan(float haflSpan);
		void voxelGIsetResolution(int resolution);*/

		void shadowAtlasUpdateConfig(const ShadowAtlasConfig& config);

		void envSetAmbientEnergy(float ambientEnergy);
		void envSetAmbientColor(Vec3f ambientColor);
		void envSetPanorama(RID panoramaTex);
		void envSetSkybox(const SkyboxSpec& spec);

		RID materialCreate(const MaterialSpec& spec);
		void materialUpdate(RID rid, const MaterialSpec& spec);
		void materialSetMetallicTextureChannel(RID rid, TexChannel textureChannel);
		void materialSetRoughnessTextureChannel(RID rid, TexChannel textureChannel);
		void materialSetAOTextureChannel(RID rid, TexChannel textureChannel);
		void _materialUpdateFlag(RID rid, const MaterialSpec& spec);

		RID meshCreate(const MeshSpec& spec);
		void meshSetTransform(RID rid, Vec3f position, Vec3f scale, Vec4f rotation);

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

		ShadowKey _shadowAtlasGetSlot(RID lightID, int texReso);

	};

}}