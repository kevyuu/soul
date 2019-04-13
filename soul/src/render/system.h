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
		void envSetEmissiveScale(float emissiveScale);
		void envSetPanorama(const float* data, int width, int height);
		void envSetSkybox(const SkyboxSpec& spec);
		void envSetExposure(float exposure);

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

		PointLightRID pointLightCreate(const PointLightSpec& spec);
		void pointLightDestroy(PointLightRID lightRID);
		PointLight* pointLightPtr(PointLightRID lightRID);
		void pointLightSetPosition(PointLightRID lightRID, Vec3f position);
		void pointLightSetMaxDistance(PointLightRID lightRID, float maxDistance);
		void pointLightSetColor(PointLightRID lightRID, Vec3f color);
		void pointLightSetBias(PointLightRID lightRID, float bias);

		SpotLightRID spotLightCreate(const SpotLightSpec& spec);
		void spotLightDestroy(SpotLightRID spotLightRID);
		SpotLight* spotLightPtr(SpotLightRID spotLightRID);
		void spotLightSetPosition(SpotLightRID spotLightRID, Vec3f position);
		void spotLightSetDirection(SpotLightRID spotLightRID, Vec3f direction);
		void spotLightSetAngleInner(SpotLightRID spotLightRID, float angle); // angle in radian
		void spotLightSetAngleOuter(SpotLightRID spotLightRID, float angle); // angle in radian
		void spotLightSetMaxDistance(SpotLightRID spotLightRID, float maxDistance);
		void spotLightSetColor(SpotLightRID spotLightRID, Vec3f color);
		void spotLightSetBias(SpotLightRID, float bias);

		void postProcessUpdateGlow(const GlowConfig& config);

		void wireframePush(MeshRID meshRID);

		void render(const Camera& camera);

		//-------------------------------------------
		// private
		//-------------------------------------------
		Database _db;

		void _shadowAtlasInit();
		void _shadowAtlasCleanup();
		ShadowKey _shadowAtlasGetSlot(int texReso);
		void _shadowAtlasFreeSlot(ShadowKey shadowKey);

		void _dirLightUpdateShadowMatrix();
		void _pointLightUpdateShadowMatrix();
		void _spotLightUpdateShadowMatrix();

		void _flushUBO();
		void _flushCameraUBO();
		void _flushLightUBO();
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

	};

}}