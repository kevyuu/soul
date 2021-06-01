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

	enum VertexAttribute : uint8_t {
		// Update hasIntegerTarget() in VertexBuffer when adding an attribute that will
		// be read as integers in the shaders

		POSITION = 0, //!< XYZ position (float3)
		TANGENTS = 1, //!< tangent, bitangent and normal, encoded as a quaternion (float4)
		COLOR = 2, //!< vertex color (float4)
		UV0 = 3, //!< texture coordinates (float2)
		UV1 = 4, //!< texture coordinates (float2)
		BONE_INDICES = 5, //!< indices of 4 bones, as unsigned integers (uvec4)
		BONE_WEIGHTS = 6, //!< weights of the 4 bones (normalized float4)
		// -- we have 1 unused slot here --
		CUSTOM0 = 8,
		CUSTOM1 = 9,
		CUSTOM2 = 10,
		CUSTOM3 = 11,
		CUSTOM4 = 12,
		CUSTOM5 = 13,
		CUSTOM6 = 14,
		CUSTOM7 = 15,
		COUNT = 16,

		// Aliases for vertex morphing.
		MORPH_POSITION_0 = CUSTOM0,
		MORPH_POSITION_1 = CUSTOM1,
		MORPH_POSITION_2 = CUSTOM2,
		MORPH_POSITION_3 = CUSTOM3,
		MORPH_TANGENTS_0 = CUSTOM4,
		MORPH_TANGENTS_1 = CUSTOM5,
		MORPH_TANGENTS_2 = CUSTOM6,
		MORPH_TANGENTS_3 = CUSTOM7,

		// this is limited by driver::MAX_VERTEX_ATTRIBUTE_COUNT
	};

	enum class ElementType : uint8_t {
		BYTE,
		BYTE2,
		BYTE3,
		BYTE4,
		UBYTE,
		UBYTE2,
		UBYTE3,
		UBYTE4,
		SHORT,
		SHORT2,
		SHORT3,
		SHORT4,
		USHORT,
		USHORT2,
		USHORT3,
		USHORT4,
		INT,
		UINT,
		FLOAT,
		FLOAT2,
		FLOAT3,
		FLOAT4,
		HALF,
		HALF2,
		HALF3,
		HALF4,
		COUNT
	};

	struct Vertex
	{
		Vec3f pos;
		Vec3f normal;
		Vec2f texUV;
		Vec3f binormal;
		Vec3f tangent;
	};

	struct Attribute {
		//! attribute is normalized (remapped between 0 and 1)
		static constexpr uint8_t FLAG_NORMALIZED = 0x1;
		//! attribute is an integer
		static constexpr uint8_t FLAG_INTEGER_TARGET = 0x2;
		uint32_t offset = 0;                    //!< attribute offset in bytes
		uint8_t stride = 0;                     //!< attribute stride in bytes
		uint8_t buffer = 0xFF;                  //!< attribute buffer index
		ElementType type = ElementType::BYTE;   //!< attribute element type
		uint8_t flags = 0x0;                    //!< attribute flags
	};

	struct Primitive {
		GPU::BufferID vertexBuffers[GPU::MAX_VERTEX_BINDING];
		GPU::BufferID indexBuffer;
		GPU::Topology topology;
		Attribute attributes[VertexAttribute::COUNT];
		uint32 vertexCount;
		uint8 vertexBindingCount;
		uint32 activeAttribute;
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
					frustumCorners[k] = frustumCorner.xyz / frustumCorner.w;
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

	struct Material {

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