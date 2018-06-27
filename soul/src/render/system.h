#include "type.h"

namespace Soul {

    struct RenderSystem {

		struct VoxelGIConfig {
			Vec3f center;
			real32 halfSpan;
			uint32 resolution;
		};

        struct Config {
            uint32 materialPoolSize = 128;
            uint32 meshPoolSize = 128;
            TexReso shadowAtlasRes;
            uint8 subdivSqrtCount[4] = {1, 2, 4, 8};
            uint32 targetWidthPx;
            uint32 targetHeightPx;

			VoxelGIConfig voxelGIConfig;
        };


        PanoramaToCubemapRP panoramaToCubemapRP;
        DiffuseEnvmapFilterRP diffuseEnvmapFilterRP;
        SpecularEnvmapFilterRP specularEnvmapFilterRP;
        BRDFMapRP brdfMapRP;

        void init(const Config& config);
        void shutdown();

        RenderRID textureCreate(const TextureSpec& spec, unsigned char* data, int dataChannelCount);

        RenderRID createDirectionalLight(const DirectionalLightSpec& spec);
        void setDirLightDirection(RenderRID lightRID, Vec3f direction);

        void setAmbientEnergy(float ambientEnergy);
        void setAmbientColor(Vec3f ambientColor);

        RenderRID createMaterial(const MaterialSpec& spec);
        RenderRID meshCreate(const MeshSpec& spec);

        void setPanorama(RenderRID panorama);
        void setSkybox(const SkyboxSpec& spec);

        void render(const Camera& camera);

        //-------------------------------------------
        // private
        //-------------------------------------------
        RenderDatabase _database;

        void _updateShadowMatrix();
        void _flushUBO();
        void _initUtilGeometry();
        void _initBRDFMap();
        void _initGBuffer();
        void _initEffectBuffer();
		void _initLightBuffer();
		void _initVoxelGIBuffer();
        ShadowKey _getShadowAtlasSlot(RenderRID lightID, TexReso texReso);

    };
}