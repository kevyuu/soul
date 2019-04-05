#pragma once

#include "core/type.h"
#include "core/array.h"
#include "core/pool_array.h"
#include "core/packed_array.h"

namespace Soul {namespace Render {
	
	// public constant
	static constexpr int MAX_DIR_LIGHT = 4;
	static constexpr int MAX_POINT_LIGHT = 100;
	static constexpr int MAX_SPOT_LIGHT = 100;

	typedef PoolID MeshRID;
	typedef PoolID DirLightRID;
	typedef PackedID PointLightRID;
	typedef PackedID SpotLightRID;
	typedef GLuint TextureRID;
	typedef uint32 MaterialRID;

	struct VoxelGIConfig {
		Vec3f center = { 0.0f, 0.0f, 0.0f };
		real32 bias = 1.5f;
		real32 diffuseMultiplier = 1.0f;
		real32 specularMultiplier = 1.0f;
		real32 halfSpan = 100;
		uint32 resolution = 64;
	};

	struct ShadowAtlasConfig {
		int32 resolution;
		int8 subdivSqrtCount[4] = { 1, 2, 4, 8 };
	};

	struct Camera {
		Vec3f up;
		Vec3f direction;
		Vec3f position;

		Mat4 projection;
		Mat4 view;

		uint16 viewportWidth;
		uint16 viewportHeight;

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

	};

	struct Vertex {
		Vec3f pos;
		Vec3f normal;
		Vec2f texUV;
		Vec3f binormal;
		Vec3f tangent;
	};

	struct Mesh {
		Mat4 transform;
		uint32 vaoHandle;
		uint32 vboHandle;
		uint32 eboHandle;
		uint32 vertexCount;
		uint32 indexCount;
		MaterialRID materialID;
	};

	enum MaterialFlag {
		MaterialFlag_USE_ALBEDO_TEX = (1UL << 0),
		MaterialFlag_USE_NORMAL_TEX = (1UL << 1),
		MaterialFlag_USE_METALLIC_TEX = (1UL << 2),
		MaterialFlag_USE_ROUGHNESS_TEX = (1UL << 3),
		MaterialFlag_USE_AO_TEX = (1UL << 4),

		MaterialFlag_METALLIC_CHANNEL_RED = (1UL << 8),
		MaterialFlag_METALLIC_CHANNEL_GREEN = (1UL << 9),
		MaterialFlag_METALLIC_CHANNEL_BLUE = (1UL << 10),
		MaterialFlag_METALLIC_CHANNEL_ALPHA = (1UL << 11),

		MaterialFlag_ROUGHNESS_CHANNEL_RED = (1UL << 12),
		MaterialFlag_ROUGHNESS_CHANNEL_GREEN = (1UL << 13),
		MaterialFlag_ROUGHNESS_CHANNEL_BLUE = (1UL << 14),
		MaterialFlag_ROUGHNESS_CHANNEL_ALPHA = (1UL << 15),

		MaterialFlag_AO_CHANNEL_RED = (1UL << 16),
		MaterialFlag_AO_CHANNEL_GREEN = (1UL << 17),
		MaterialFlag_AO_CHANNEL_BLUE = (1UL << 18),
		MaterialFlag_AO_CHANNEL_ALPHA = (1UL << 19)
	};

	struct Material {

		GLuint albedoMap;
		GLuint normalMap;
		GLuint metallicMap;
		GLuint roughnessMap;
		GLuint aoMap;

		Vec3f albedo;
		float metallic;
		float roughness;
		uint32 flags;

	};

	struct Environment {
		GLuint panorama = 0;
		GLuint cubemap = 0;
		GLuint diffuseMap = 0;
		GLuint specularMap = 0;
		GLuint brdfMap = 0;

		float ambientEnergy;
		Vec3f ambientColor;
		bool useSkybox = false;
	};

	struct ShadowKey {
		int16 quadrant;
		int16 subdiv;
		int16 slot;
	};

	struct DirLight {
		Mat4 shadowMatrixes[4];
		Vec3f direction;
		Vec3f color;
		int32 resolution;
		float split[3];
		ShadowKey shadowKey;
		float bias;
	};

	struct PointLight {
		Mat4 shadowMatrixes[6];
		ShadowKey shadowKeys[6];
		Vec3f position;
		float bias;
		Vec3f color;
		float maxDistance;
	};

	struct SpotLight
	{
		Mat4 shadowMatrix;
		Vec3f position;
		float bias;
		Vec3f direction;
		float angleOuter;
		float cosOuter;
		Vec3f color;
		float cosInner;
		ShadowKey shadowKey;
		float maxDistance;
	};

	// Resource spec

	enum TexChannel {

		TexChannel_RED,
		TexChannel_GREEN,
		TexChannel_BLUE,
		TexChannel_ALPHA,

		TexChannel_COUNT

	};

	enum TexFilter {

		TexFilter_NEAREST,
		TexFilter_LINEAR,
		TexFilter_NEAREST_MIPMAP_NEAREST,
		TexFilter_LINEAR_MIPMAP_NEAREST,
		TexFilter_NEAREST_MIPMAP_LINEAR,
		TexFilter_LINEAR_MIPMAP_LINEAR,

		TexFilter_COUNT
	};

	static constexpr GLuint _GL_FILTER_MAP[TexFilter_COUNT] = {
		GL_NEAREST,
		GL_LINEAR,
		GL_NEAREST_MIPMAP_NEAREST,
		GL_LINEAR_MIPMAP_NEAREST,
		GL_NEAREST_MIPMAP_LINEAR,
		GL_LINEAR_MIPMAP_LINEAR
	};

	enum TexWrap {

		TexWrap_CLAMP_TO_EDGE,
		TexWrap_MIRRORED_REPEAT,
		TexWrap_REPEAT,

		TexWrap_COUNT
	};

	static constexpr GLuint _GL_WRAP_MAP[TexWrap_COUNT] = {
		GL_CLAMP_TO_EDGE,
		GL_MIRRORED_REPEAT,
		GL_REPEAT
	};

	enum TexReso {

		TexReso_1 = 1,
		TexReso_2 = 2,
		TexReso_4 = 4,
		TexReso_8 = 8,
		TexReso_16 = 16,
		TexReso_32 = 32,
		TexReso_64 = 64,
		TexReso_128 = 128,
		TexReso_256 = 256,
		TexReso_512 = 512,
		TexReso_1024 = 1024,
		TexReso_2048 = 2048,
		TexReso_4096 = 4096,
		TexReso_8192 = 8192

	};

	enum PixelFormat {

		PixelFormat_RED,
		PixelFormat_RG,
		PixelFormat_RGB,
		PixelFormat_RGBA,
		PixelFormat_DEPTH_COMPONENT,
		
		PixelFormat_COUNT

	};

	static constexpr GLuint _GL_PIXEL_FORMAT_MAP[PixelFormat_COUNT] = {
		GL_RED,
		GL_RG,
		GL_RGB,
		GL_RGBA,
		GL_DEPTH_COMPONENT,
	};

	struct TexSpec {

		int32 width;
		int32 height;
		uint8 mipLevel;
		
		PixelFormat pixelFormat;
		
		TexFilter filterMin;
		TexFilter filterMag;

		TexWrap wrapS;
		TexWrap wrapT;

	};

	struct DirectionalLightSpec {
		Vec3f direction = Vec3f(0.0f, -1.0f, 0.0f);
		Vec3f color = { 10.0f, 10.0f, 10.0f };
		float split[3] = { 0.1f, 0.3f, 0.6f };
		int32 shadowMapResolution = TexReso_2048;
		float bias = 0.001f;
	};

	struct PointLightSpec 
	{
		Vec3f position = Vec3f(0.0f, 0.0f, 0.0f);
		float bias = 0.001f;
		Vec3f color = { 10.0f, 10.0f, 10.0f };
		float maxDistance = 10;
		int32 shadowMapResolution = TexReso_2048;
	};

	struct SpotLightSpec
	{
		Vec3f position = Vec3f(0.0f, 0.0f, 0.0f);
		Vec3f direction = Vec3f(0.0f, -1.0f, 0.0f);
		Vec3f color = {10.0f, 10.0f, 10.0f};
		uint32 shadowMapResolution = TexReso_256;
		float bias = 0.05f;
		float angleInner = 0.3f; // in radian
		float angleOuter = 0.5f; // in radian
		float maxDistance = 2.0f; // in meter

	};

	struct MaterialSpec {
		TextureRID albedoMap;
		TextureRID normalMap;
		TextureRID metallicMap;
		TextureRID roughnessMap;
		TextureRID aoMap;

		bool useAlbedoTex;
		bool useNormalTex;
		bool useMetallicTex;
		bool useRoughnessTex;
		bool useAOTex;

		Vec3f albedo;
		float metallic;
		float roughness;

		TexChannel metallicChannel;
		TexChannel roughnessChannel;
		TexChannel aoChannel;
	};

	struct MeshSpec {
		Mat4 transform;
		Vertex* vertexes;
		uint32* indices;
		uint32 vertexCount;
		uint32 indexCount;
		MaterialRID material;
	};

	struct SkyboxSpec {
		union {
			struct {
				unsigned char* x_positive;
				unsigned char* x_negative;
				unsigned char* y_positive;
				unsigned char* y_negative;
				unsigned char* z_positive;
				unsigned char* z_negative;
			} data;

			unsigned char* faces[6] = { nullptr };
		};

		int width;
		int height;
	};

	struct EnvironmentSpec {
		GLuint panorama;
	};

	//-------------------------------------------
	// Private
	//-------------------------------------------

	struct Constant {

		static constexpr int CAMERA_DATA_BINDING_POINT = 0;
		static constexpr char CAMERA_DATA_NAME[] = "CameraData";

		static constexpr int LIGHT_DATA_BINDING_POINT = 1;
		static constexpr char LIGHT_DATA_NAME[] = "LightData";

		static constexpr int VOXEL_GI_DATA_BINDING_POINT = 2;
		static constexpr char VOXEL_GI_DATA_NAME[] = "VoxelGIData";

	};

	struct ShadowAtlas {
		static constexpr uint8 MAX_LIGHT = 64;
		int32 resolution;
		int8 subdivSqrtCount[4];
		GLuint texHandle;
		GLuint framebuffer;

		// TODO(kevyuu): Implement BitSet data structure and use it instead of bool
		bool slots[MAX_LIGHT];
	};

	struct CameraDataUBO {
		Mat4 projection;
		Mat4 view;
		Mat4 projectionView;
		Mat4 invProjectionView;

		Mat4 prevProjection;
		Mat4 prevView;
		Mat4 prevProjectionView;

		Vec3f position;
		float pad;
	};

	struct DirectionalLightUBO {
		Mat4 shadowMatrixes[4];
		Vec3f direction;
		float bias;
		Vec3f color;
		float pad2;
		float cascadeDepths[4];
	};

	struct PointLightUBO
	{
		Mat4 shadowMatrixes[6];
		Vec3f position;
		float bias;
		Vec3f color;
		float maxDistance;
	};

	struct SpotLightUBO
	{
		Mat4 shadowMatrix;
		Vec3f position;
		float cosInner;
		Vec3f direction;
		float cosOuter;
		Vec3f color;
		float maxDistance;
		Vec3f pad;
		float bias;
	};

	struct LightDataUBO {
		
		DirectionalLightUBO dirLights[MAX_DIR_LIGHT];
		Vec3f pad1;
		int dirLightCount;

		PointLightUBO pointLights[MAX_POINT_LIGHT];
		Vec3f pad2;
		int pointLightCount;
		
		SpotLightUBO spotLights[MAX_SPOT_LIGHT];
		Vec3f pad3;
		int spotLightCount;

	};

	struct VoxelGIDataUBO {
		Vec3f frustumCenter;
		int resolution;
		float frustumHalfSpan;
		float bias;
		float diffuseMultiplier;
		float specularMultiplier;
	};

	static const GLuint s_formatMap[PixelFormat_COUNT] = {
			GL_RED,
			GL_RG,
			GL_RGB,
			GL_RGBA,
			GL_DEPTH_COMPONENT
	};

	struct Database;

	struct RenderPass {
		virtual void init(Database& database) = 0;
		virtual void execute(Database& database) = 0;
		virtual void shutdown(Database& database) = 0;

	};

	struct ShadowMapRP : public RenderPass {

		GLuint program;
		int32 modelLoc;
		int32 shadowMatrixLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct Texture2DDebugRP : public RenderPass {

		GLuint program;
		GLuint texDebugLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);

	};

	struct PanoramaToCubemapRP : public RenderPass {

		GLuint renderTarget;
		GLuint renderBuffer;
		GLuint program;

		GLuint projectionLoc;
		GLuint viewLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct DiffuseEnvmapFilterRP : public RenderPass {

		GLuint renderTarget;
		GLuint renderBuffer;
		GLuint program;

		GLuint projectionLoc;
		GLuint viewLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct GBufferGenRP : public RenderPass {

		GLuint predepthProgram;
		GLuint gBufferGenProgram;

		GLint modelUniformLoc;

		GLint albedoMapLoc;
		GLint normalMapLoc;
		GLint metallicMapLoc;
		GLint roughnessMapLoc;
		GLint aoMapLoc;

		GLint materialFlagsLoc;

		GLint albedoLoc;
		GLint metallicLoc;
		GLint roughnessLoc;

		GLint shadowMapLoc;

		GLint viewPositionLoc;
		GLint ambientFactorLoc;

		GLint predepthModelUniformLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);

	};

	struct LightingRP : public RenderPass {
		GLuint program;

		GLint shadowMapUniformLoc;
		GLint renderMap1UniformLoc;
		GLint renderMap2UniformLoc;
		GLint renderMap3UniformLoc;
		GLint viewPositionUniformLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);

	};

	struct SSRTraceRP : public RenderPass {
		GLuint program;

		GLint renderMap1UniformLoc;
		GLint renderMap2UniformLoc;
		GLint renderMap3UniformLoc;
		GLint depthMapLoc;

		GLint screenDimensionLoc;

		GLint cameraZNearLoc;
		GLint cameraZFarLoc;


		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct SSRResolveRP : public RenderPass {
		GLuint program;

		GLint reflectionPosBufferLoc;
		GLint lightBufferLoc;
		GLint renderMap1Loc;
		GLint renderMap2Loc;
		GLint renderMap3Loc;
		GLint renderMap4Loc;
		GLint depthMapLoc;
		GLint FGMapLoc;
		GLint voxelLightBufferLoc;

		GLint diffuseEnvTexLoc;
		GLint specularEnvTexLoc;

		GLint screenDimensionLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct GaussianBlurRP : public RenderPass {
		GLuint horizontalProgram;

		GLint sourceTexUniformLocHorizontal;
		GLint targetSizePxUniformLocHorizontal;
		GLint lodUniformLocHorizontal;

		GLuint verticalProgram;

		GLint sourceTexUniformLocVertical;
		GLint targetSizePxUniformLocVertical;
		GLint lodUniformLocVertical;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct SkyboxRP : public RenderPass {
		GLuint program;

		GLint projectionLoc;
		GLint viewLoc;
		GLint skyboxLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct SpecularEnvmapFilterRP : public RenderPass {
		GLuint renderTarget;
		GLuint renderBuffer;
		GLuint shader;

		GLuint projectionLoc;
		GLuint viewLoc;
		GLuint roughenssLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct BRDFMapRP : public RenderPass {
		GLuint framebuffer;
		GLuint renderBuffer;
		GLuint program;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct VoxelizeRP : public RenderPass {
		GLuint program;

		GLint projectionViewLoc[3];
		GLint inverseProjectionViewLoc[3];

		GLint modelLoc;
		GLint albedoMapLoc;
		GLint normalMapLoc;
		GLint metallicMapLoc;
		GLint roughnessMapLoc;

		GLint voxelAlbedoBufferLoc;
		GLint voxelNormalBufferLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);

	};

	struct VoxelDebugRP : public RenderPass {
		GLuint program;

		GLint voxelBufferLoc;

		GLuint dummyVAO;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct VoxelLightInjectRP : public RenderPass {
		GLuint program;

		GLint voxelAlbedoBufferLoc;
		GLint voxelNormalBufferLoc;
		GLint lightVoxelBufferLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct VoxelMipmapGenRP : public RenderPass {
		GLuint program;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct VelocityBufferGenRP : public RenderPass {
		GLuint program;

		GLint depthMapLoc;
		GLint invCurProjectionViewLoc;
		GLint prevProjectionViewLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);
	};

	struct WireframeRP : public RenderPass {
		GLuint program;

		GLint modelUniformLoc;

		void init(Database& database);
		void execute(Database& database);
		void shutdown(Database& database);

	};

	struct GBuffer {
		GLuint frameBuffer;

		GLuint depthBuffer;

		GLuint renderBuffer1;
		GLuint renderBuffer2;
		GLuint renderBuffer3;
		GLuint renderBuffer4;

	};

	struct LightBuffer {
		GLuint frameBuffer;
		GLuint colorBuffer;
	};

	struct EffectBuffer {
		struct MipChain {

			struct Mipmap {
				GLuint frameBuffer = 0;
				int width;
				int height;
			};

			Array<Mipmap> mipmaps;
			GLuint colorBuffer = 0;
			int numLevel;

		};

		MipChain lightMipChain[2];

		struct SSRTraceBuffer {
			GLuint frameBuffer = 0;
			GLuint traceBuffer = 0;
		} ssrTraceBuffer;

		struct SSRResolveBuffer {
			GLuint frameBuffer = 0;
			GLuint resolveBuffer = 0;
		} ssrResolveBuffer;

		GLuint depthBuffer = 0;

	};

	struct VelocityBuffer {
		GLuint tex;
		GLuint frameBuffer;
	};

	struct VoxelGIBuffer {

		GLuint gVoxelAlbedoTex = 0;
		GLuint gVoxelNormalTex = 0;
		GLuint gVoxelOccupancyTex = 0;
		GLuint lightVoxelTex = 0;

	};

	struct Database {

		uint32 frameIdx;

		uint32 targetWidthPx;
		uint32 targetHeightPx;

		Array<Material> materialBuffer;

		PoolArray<uint32> meshIndexes;
		Array<MeshRID> meshRIDs;
		Array<Mesh> meshBuffer;

		PoolArray<uint32> dirLightIndexes;
		DirLightRID dirLightRIDs[MAX_DIR_LIGHT];
		DirLight dirLights[MAX_DIR_LIGHT];
		int dirLightCount;

		PackedArray<PointLight> pointLights;
		PackedArray<SpotLight> spotLights;
		
		Environment environment;
			
		ShadowAtlas shadowAtlas;

		GLuint cameraDataUBOHandle;
		CameraDataUBO cameraDataUBO;

		GLuint lightDataUBOHandle;
		LightDataUBO lightDataUBO;

		GLuint voxelGIDataUBOHandle;
		VoxelGIDataUBO voxelGIDataUBO;

		Camera camera;
		Camera prevCamera;

		GBuffer gBuffer;
		EffectBuffer effectBuffer;
		LightBuffer lightBuffer;
		VelocityBuffer velocityBuffer;

		VoxelGIConfig voxelGIConfig;
		VoxelGIBuffer voxelGIBuffer;

		// util geometry
		GLuint cubeVAO;
		GLuint cubeVBO;

		GLuint quadVAO;
		GLuint quadVBO;

		Array<RenderPass*> renderPassList;
		Array<Mesh*> wireframeMeshes;

		AABB sceneBound;
	};
}}
