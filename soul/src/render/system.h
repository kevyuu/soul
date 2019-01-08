#pragma once
#include "render/type.h"

namespace Soul {

    struct RenderSystem {

		

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

        RenderRID textureCreate(const TextureSpec& spec, unsigned char* data, int dataChannelCount);

        RenderRID dirLightCreate(const DirectionalLightSpec& spec);
        void dirLightSetDirection(RenderRID lightRID, Vec3f direction);
		void dirLightSetColor(RenderRID lightRID, Vec3f color);
		void dirLightSetShadowMapResolution(RenderRID lightRID, int32 resolution);
		void dirLightSetCascadeSplit(RenderRID lightRID, float split1, float split2, float split3);
		void dirLightSetBias(RenderRID lightRID, float bias);

		void voxelGIVoxelize();
		void voxelGIUpdateConfig(const VoxelGIConfig& config);
		/*void voxelGIsetCenter(Vec3f center);
		void voxelGIsetHalfSpan(float haflSpan);
		void voxelGIsetResolution(int resolution);*/

		void shadowAtlasUpdateConfig(const ShadowAtlasConfig& config);

        void envSetAmbientEnergy(float ambientEnergy);
        void envSetAmbientColor(Vec3f ambientColor);
		void envSetPanorama(RenderRID panoramaTex);
		void envSetSkybox(const SkyboxSpec& spec);

        RenderRID materialCreate(const MaterialSpec& spec);

        RenderRID meshCreate(const MeshSpec& spec);

        void render(const Camera& camera);

        //-------------------------------------------
        // private
        //-------------------------------------------
        RenderDatabase _database;

		void _shadowAtlasInit();
		void _shadowAtlasCleanup();
		void _shadowAtlasFreeSlot(ShadowKey shadowKey);

        void _updateShadowMatrix();
        void _flushUBO();
        
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

        ShadowKey _shadowAtlasGetSlot(RenderRID lightID, int texReso);

    };
}