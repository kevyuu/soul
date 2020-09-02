#pragma once

#include "../../data.h"

using namespace Soul;

namespace DeferredPipeline {

	struct Camera {
		Vec3f up;
		Vec3f direction;
		Vec3f position;

		Mat4 projection;
		Mat4 view;

		uint16 viewportWidth;
		uint16 viewportHeight;

		float aperture = 1.0f;
		float shutterSpeed = 1.0f;
		float sensitivity = 1.0f;
		float exposure = 1.0f;

		bool exposureFromSetting = false;

		union {
			struct {
				float fov;
				float aspectRatio;
				float zNear;
				float zFar;
			} perspective;

			struct {
				float left;
				float right;
				float top;
				float bottom;
				float zNear;
				float zFar;
			} ortho;
		};

		void updateExposure() {
			if (exposureFromSetting) {
				float ev100 = log2((aperture * aperture) / shutterSpeed * 100.0 / sensitivity);
				exposure = 1.0f / (pow(2.0f, ev100) * 1.2f);
			}
		}

	};

	enum EntityType
	{
		EntityType_GROUP,
		EntityType_MESH,
		EntityType_DIRLIGHT,
		EntityType_POINTLIGHT,
		EntityType_SPOTLIGHT,

		EntityType_Count
	};

	struct EntityID
	{
		PoolID index;
		uint16 type;

		bool operator==(const EntityID& rhs)
		{
			return this->type == rhs.type && this->index == rhs.index;
		}

		bool operator!=(const EntityID& rhs)
		{
			return this->type != rhs.type || this->index != rhs.index;
		}
	};

	struct GroupEntity;

	struct Entity
	{
		static constexpr uint32 MAX_NAME_LENGTH = 1024;
		EntityID entityID;
		char name[MAX_NAME_LENGTH];

		GroupEntity* parent = nullptr;
		Entity* prev = nullptr;
		Entity* next = nullptr;

		Transform localTransform;
		Transform worldTransform;
	};

	struct GroupEntity : Entity
	{
		Entity* first;
	};

	struct MeshEntity : Entity
	{
		uint32 meshID;
		uint32 materialID;
	};

	struct Mesh
	{
		GPU::BufferID vertexBufferID;
		GPU::BufferID indexBufferID;
		uint16 indexCount;
	};

	struct DirectionalLight
	{
		static constexpr uint32 SHADOW_MAP_RESOLUTION = 2048 * 2;
		Vec3f direction = Vec3f(0.0f, -1.0f, 0.0f);
		Vec3f color = { 1.0f, 1.0f, 1.0f };
		float illuminance = 10; // in lx;
		float split[3] = { 0.1f, 0.3f, 0.6f };
		float bias = 0.001f;
		Mat4 shadowMatrixes[4];

		void updateShadowMatrixes(Camera& camera)
		{
			float zNear = camera.perspective.zNear;
			float zFar = camera.perspective.zFar;
			float zDepth = zFar - zNear;
			float fov = camera.perspective.fov;
			float aspectRatio = camera.perspective.aspectRatio;
			Vec3f upVec = Vec3f(0.0f, 1.0f, 0.0f);
			if (abs(dot(upVec, direction)) == 1.0f)
			{
				upVec = Vec3f(1.0f, 0.0f, 0.0f);
			}
			Mat4 lightRot = mat4View(Vec3f(0, 0, 0), direction, upVec);

			Mat4 viewMat = mat4View(camera.position, camera.position + camera.direction, camera.up);


			float splitOffset[5] = { 0, split[0], split[1], split[2], 1 };
			float splitNDCWidth = 1.0f;
			for (int i = 0; i < 4; i++)
			{
				Vec3f frustumCorners[8] = {
					   Vec3f(-1.0f, -1.0f, -1), Vec3f(1.0f, -1.0f, -1), Vec3f(1.0f, 1.0f, -1), Vec3f(-1.0f, 1.0f, -1),
					   Vec3f(-1.0f, -1.0f, 1), Vec3f(1.0f, -1.0f, 1), Vec3f(1.0f, 1.0f, 1), Vec3f(-1.0f, 1.0f, 1)
				};

				Mat4 projectionMat = mat4Perspective(fov, aspectRatio, zNear + splitOffset[i] * zDepth, zNear + splitOffset[i + 1] * zDepth);
				Mat4 projectionViewMat = projectionMat * viewMat;
				Mat4 invProjectionViewMat = mat4Inverse(projectionViewMat);

				Vec3f worldFrustumCenter = Vec3f(0, 0, 0);

				for (int k = 0; k < 8; k++) {
					Vec4f frustumCorner = invProjectionViewMat * Vec4f(frustumCorners[k], 1.0f);
					frustumCorners[k] = frustumCorner.xyz() / frustumCorner.w;
					worldFrustumCenter += frustumCorners[k];
				}
				worldFrustumCenter *= (1.0f / 8.0f);

				float cascadeDepth = (splitOffset[i + 1] - splitOffset[i]) * zDepth;
				float cascadeFarDistance = zNear + splitOffset[i + 1] * zDepth;
				float cascadeFarWidth = tan(camera.perspective.fov / 2) * 2 * cascadeFarDistance;
				float cascadeFarHeight = cascadeFarWidth / camera.perspective.aspectRatio;

				float radius = sqrt(cascadeFarWidth * cascadeFarWidth + cascadeDepth * cascadeDepth + cascadeFarHeight * cascadeFarHeight);

				float texelPerUnit = DirectionalLight::SHADOW_MAP_RESOLUTION / (radius * 4.0f);
				Mat4 texelScaleLightRot = mat4Scale(Soul::Vec3f(texelPerUnit, texelPerUnit, texelPerUnit)) * lightRot;

				Vec3f lightTexelFrustumCenter = texelScaleLightRot * worldFrustumCenter;
				lightTexelFrustumCenter.x = (float)floor(lightTexelFrustumCenter.x);
				lightTexelFrustumCenter.y = (float)floor(lightTexelFrustumCenter.y);
				worldFrustumCenter = mat4Inverse(texelScaleLightRot) * lightTexelFrustumCenter;

				int xSplit = i % 2;
				int ySplit = i / 2;

				float bottomSplitNDC = -1 + (ySplit * splitNDCWidth);
				float leftSplitNDC = -1 + (xSplit * splitNDCWidth);

				Mat4 atlasMatrix;
				atlasMatrix.elem[0][0] = splitNDCWidth / 2;
				atlasMatrix.elem[0][3] = leftSplitNDC + (splitNDCWidth * 0.5f);
				atlasMatrix.elem[1][1] = splitNDCWidth / 2;
				atlasMatrix.elem[1][3] = bottomSplitNDC + (splitNDCWidth * 0.5f);
				atlasMatrix.elem[2][2] = 1;
				atlasMatrix.elem[3][3] = 1;

				/*Vec3f sceneBoundCorners[8] = {
					db.sceneBound.min,
					Vec3f(db.sceneBound.min.x, db.sceneBound.min.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.min.x, db.sceneBound.max.y, db.sceneBound.min.z),
					Vec3f(db.sceneBound.min.x, db.sceneBound.max.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.min.y, db.sceneBound.min.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.min.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.max.y, db.sceneBound.min.z),
					db.sceneBound.max
				};

				float shadowMapFar = dot(direction, (sceneBoundCorners[0] - worldFrustumCenter));
				float shadowMapNear = shadowMapFar;

				for (int j = 1; j < 8; j++) {
					float cornerDist = dot(direction, sceneBoundCorners[j] - worldFrustumCenter);
					if (cornerDist > shadowMapFar) shadowMapFar = cornerDist;
					if (cornerDist < shadowMapNear) shadowMapNear = cornerDist;
				}*/
				float shadowMapFar = 500;
				float shadowMapNear = -500;

				shadowMatrixes[i] = atlasMatrix * mat4Ortho(-radius, radius, -radius, radius, shadowMapNear, shadowMapFar) *
					mat4View(worldFrustumCenter, worldFrustumCenter + direction, upVec);
			}
		}
	};

	enum class TexChannel : uint8
	{
		RED,
		GREEN,
		BLUE,
		ALPHA,
		COUNT
	};

	struct SceneMaterial
	{
		char name[1024];

		PoolID albedoTexID;
		PoolID normalTexID;
		PoolID metallicTexID;
		PoolID roughnessTexID;
		PoolID aoTexID;
		PoolID emissiveTexID;

		Vec3f albedo;
		float metallic;
		float roughness;
		Vec3f emissive;

		bool useAlbedoTex;
		bool useNormalTex;
		bool useMetallicTex;
		bool useRoughnessTex;
		bool useAOTex;
		bool useEmissiveTex;

		TexChannel metallicTextureChannel;
		TexChannel roughnessTextureChannel;
		TexChannel aoTextureChannel;
	};

	struct SceneTexture
	{
		char name[1024];
		GPU::TextureID rid;
	};

	class Scene : public Demo::Scene {
	public:
		GPU::System* gpuSystem;

		EntityID rootEntityID;

		Pool<GroupEntity> groupEntities;
		Array<MeshEntity> meshEntities;
		Array<Mesh> meshes;
		Array<SceneMaterial> materials;

		Array<SceneTexture> textures;
		DirectionalLight dirLight;

		struct VoxelGIConfig {
			Vec3f center = { 0.0f, 0.0f, 0.0f };
			real32 bias = 1.5f;
			real32 diffuseMultiplier = 1.0f;
			real32 specularMultiplier = 1.0f;
			real32 halfSpan = 15;
			uint32 resolution = 128;
		} voxelGIConfig;

		GPU::BufferID materialBuffer = GPU::BUFFER_ID_NULL;

		Camera camera;

		EntityID _createEntity(EntityID parentID, EntityType entityType, const char* name, Transform localTransform);
		Entity* _entityPtr(EntityID entityID);

		Scene(GPU::System* gpuSystem) : gpuSystem(gpuSystem) {}

		virtual void importFromGLTF(const char* path);
		virtual void cleanup();
		virtual bool handleInput();
		virtual Vec2ui32 getViewport() { return { camera.viewportWidth, camera.viewportHeight }; }
		virtual void setViewport(Vec2ui32 viewport) { camera.viewportWidth = viewport.x; camera.viewportHeight = viewport.y; }
	};
}