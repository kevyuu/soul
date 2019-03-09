#pragma once

#include "core/type.h"
#include "core/array.h"

namespace Soul {

    typedef unsigned int RenderRID;

    enum TexReso {
        TR_1 = 1,
        TR_2 = 2,
        TR_4 = 4,
        TR_8 = 8,
        TR_16 = 16,
        TR_32 = 32,
        TR_64 = 64,
        TR_128 = 128,
        TR_256 = 256,
        TR_512 = 512,
        TR_1024 = 1024,
        TR_2048 = 2048,
        TR_4096 = 4096,
        TR_8192 = 8192
    };

    enum PixelFormat {
        PF_RED,
        PF_RG,
        PF_RGB,
        PF_RGBA,
        PF_DEPTH_COMPONENT,
        PF_COUNT
    };

	struct VoxelGIConfig {
		Vec3f center = { 0.0f, 0.0f, 0.0f };
		real32 bias = 1.5f;
		real32 diffuseMultiplier = 1.0f;
		real32 specularMultiplier = 1.0f;
		real32 halfSpan = 100;
		uint32 resolution = 64;
	};

    struct Camera {
        Vec3f up;
        Vec3f direction;
        Vec3f position;
        Mat4 projection;

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
        RenderRID materialID;
    };

	enum MaterialFlag {
		MaterialFlag_USE_ALBEDO_TEX = (1UL << 0),
		MaterialFlag_USE_NORMAL_TEX = (1UL << 1),
		MaterialFlag_USE_METALLIC_TEX = (1UL << 2),
		MaterialFlag_USE_ROUGHNESS_TEX = (1UL << 3),

		MaterialFlag_METALLIC_CHANNEL_RED = (1UL << 8),
		MaterialFlag_METALLIC_CHANNEL_GREEN = (1UL << 9),
		MaterialFlag_METALLIC_CHANNEL_BLUE = (1UL << 10),
		MaterialFlag_METALLIC_CHANNEL_ALPHA = (1UL << 11),

		MaterialFlag_ROUGHNESS_CHANNEL_RED = (1UL << 12),
		MaterialFlag_ROUGHNESS_CHANNEL_GREEN = (1UL << 13),
		MaterialFlag_ROUGHNESS_CHANNEL_BLUE = (1UL << 14),
		MaterialFlag_ROUGHNESS_CHANNEL_ALPHA = (1UL << 15)
	};

    struct Material {

        RenderRID albedoMap;
        RenderRID normalMap;
        RenderRID metallicMap;
        RenderRID roughnessMap;

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
    };

    struct ShadowKey {
        int16 quadrant;
        int16 subdiv;
		int16 slot;
    };

    struct DirectionalLight {
        Mat4 shadowMatrix[4];
        Vec3f direction;
        Vec3f color;
		int32 resolution;
		float split[3];
        ShadowKey shadowKey;
		float bias;
    };

	enum TextureChannel {
		TextureChannel_RED,
		TextureChannel_GREEN,
		TextureChannel_BLUE,
		TextureChannel_ALPHA,
		TC_COUNT
	};

    // Resource spec
    struct TextureSpec {
        int32 width;
        int32 height;
        int8 mipLevel;
        PixelFormat pixelFormat;
        GLint minFilter;
        GLint magFilter;
    };

    struct DirectionalLightSpec {
        Vec3f direction;
		Vec3f color = { 100.0f, 100.0f, 100.0f };
        float split[3] = { 0.1f, 0.3f, 0.6f };
        int32 shadowMapResolution = 2048;
		float bias = 0.05f;;
    };

    struct MaterialSpec {
        RenderRID albedoMap;
        RenderRID normalMap;
        RenderRID metallicMap;
        RenderRID roughnessMap;

		bool useAlbedoTex;
		bool useNormalTex;
		bool useMetallicTex;
		bool useRoughnessTex;

		Vec3f albedo;
		float metallic;
		float roughness;

		TextureChannel metallicChannel;
		TextureChannel roughnessChannel;
    };

    struct MeshSpec {
        Mat4 transform;
        Vertex* vertexes;
        uint32* indices;
        uint32 vertexCount;
        uint32 indexCount;
        RenderRID material;
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

            unsigned char* faces[6] = {nullptr};
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

    struct RenderConstant {
        
		static constexpr int CAMERA_DATA_BINDING_POINT = 0;
		static constexpr char CAMERA_DATA_NAME[] = "CameraData";

		static constexpr int LIGHT_DATA_BINDING_POINT = 1;
		static constexpr char LIGHT_DATA_NAME[] = "LightData";

		static constexpr int VOXEL_GI_DATA_BINDING_POINT = 2;
		static constexpr char VOXEL_GI_DATA_NAME[] = "VoxelGIData";

		
        static constexpr int MAX_DIRECTIONAL_LIGHTS = 4;
    };

    struct ShadowAtlas {
        static constexpr uint8 MAX_LIGHT = 64;
        int32 resolution;
        int8 subdivSqrtCount[4];
        GLuint texHandle;
		GLuint framebuffer;
        RenderRID slots[MAX_LIGHT];
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
    };

    struct DirectionalLightUBO {
        Mat4 shadowMatrix[4];
        Vec3f direction;
		float bias;
        Vec3f color;
        float pad2;
        float cascadeDepths[4];
    };

    struct LightDataUBO {
        DirectionalLightUBO dirLights[RenderConstant::MAX_DIRECTIONAL_LIGHTS];
        int dirLightCount;
		float pad[3];
    };

	struct VoxelGIDataUBO {
		Vec3f frustumCenter;
		int resolution;
		float frustumHalfSpan;
		float bias;
		float diffuseMultiplier;
		float specularMultiplier;
	};

    static const GLuint s_formatMap[PF_COUNT] = {
            GL_RED,
            GL_RG,
            GL_RGB,
            GL_RGBA,
            GL_DEPTH_COMPONENT
    };

    struct RenderDatabase;

    struct RenderPass {
        virtual void init(RenderDatabase& database) = 0;
        virtual void execute(RenderDatabase& database) = 0;
        virtual void shutdown(RenderDatabase& database) = 0;

    };

    struct ShadowMapRP: public RenderPass {

        RenderRID shader;
        int32 modelLoc;
        int32 shadowMatrixLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
    };

    struct Texture2DDebugRP : public RenderPass {

        RenderRID shader;
		GLuint texDebugLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);

    };

    struct PanoramaToCubemapRP: public RenderPass {

        RenderRID renderTarget;
        GLuint renderBuffer;
        RenderRID shader;

		GLuint projectionLoc;
		GLuint viewLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
    };

    struct DiffuseEnvmapFilterRP: public RenderPass {

        RenderRID renderTarget;
        RenderRID renderBuffer;
        RenderRID shader;

        GLuint projectionLoc;
        GLuint viewLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
    };

    struct GBufferGenRP: public RenderPass {

        RenderRID predepthShader;
        RenderRID gBufferShader;

        GLint modelUniformLoc;

        GLint albedoMapLoc;
        GLint normalMapLoc;
        GLint metallicMapLoc;
        GLint roughnessMapLoc;

		GLint materialFlagsLoc;
		
		GLint albedoLoc;
		GLint metallicLoc;
		GLint roughnessLoc;
		
		GLint shadowMapLoc;
		GLint viewPositionLoc;
		GLint ambientFactorLoc;

        GLint predepthModelUniformLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);

    };

    struct LightingRP: public RenderPass {
        RenderRID shader;

        GLint shadowMapUniformLoc;
        GLint renderMap1UniformLoc;
        GLint renderMap2UniformLoc;
        GLint renderMap3UniformLoc;
        GLint viewPositionUniformLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);

    };

    struct SSRTraceRP: public RenderPass {
    	RenderRID shader;

    	GLint renderMap1UniformLoc;
    	GLint renderMap2UniformLoc;
    	GLint renderMap3UniformLoc;
    	GLint depthMapLoc;

    	GLint screenDimensionLoc;

    	GLint cameraZNearLoc;
    	GLint cameraZFarLoc;


    	void init(RenderDatabase& database);
    	void execute(RenderDatabase& database);
    	void shutdown(RenderDatabase& database);
    };

    struct SSRResolveRP: public RenderPass {
    	RenderRID shader;

    	GLint reflectionPosBufferLoc;
    	GLint lightBufferLoc;
    	GLint renderMap1Loc;
    	GLint renderMap2Loc;
    	GLint renderMap3Loc;
		GLint renderMap4Loc;
		GLint depthMapLoc;
    	GLint FGMapLoc;
		GLint voxelLightBufferLoc;

    	GLint screenDimensionLoc;

    	void init(RenderDatabase& database);
    	void execute(RenderDatabase& database);
    	void shutdown(RenderDatabase& database);
    };

    struct GaussianBlurRP: public RenderPass {
    	RenderRID shaderHorizontal;

    	GLint sourceTexUniformLocHorizontal;
    	GLint targetSizePxUniformLocHorizontal;
    	GLint lodUniformLocHorizontal;

		RenderRID shaderVertical;

		GLint sourceTexUniformLocVertical;
		GLint targetSizePxUniformLocVertical;
		GLint lodUniformLocVertical;

		void init(RenderDatabase& database);
    	void execute(RenderDatabase& database);
    	void shutdown(RenderDatabase& database);
    };

    struct PBRSceneRP: public RenderPass {
        RenderRID predepthShader;
        RenderRID sceneShader;
        RenderRID renderTarget;

        GLint modelUniformLoc;
        GLint viewPosUniformLoc;
        GLint albedoMapPositionLoc;
        GLint normalMapPositionLoc;
        GLint metallicMapPositionLoc;
        GLint roughnessMapPositionLoc;
        GLint aoMapPositionLoc;

        GLint ambientEnergyLoc;
        GLint ambientColorLoc;

        GLint predepthModelUniformLoc;
        GLint predepthViewUniformLoc;
        GLint predepthProjectionUniformLoc;


        GLint shadowMapLoc;
        GLint brdfMapLoc;
        GLint diffuseMapLoc;
        GLint specularMapLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
    };

    struct SkyboxRP: public RenderPass {
        GLuint shader;

        GLint projectionLoc;
        GLint viewLoc;
        GLint skyboxLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
    };

    struct SpecularEnvmapFilterRP: public RenderPass {
        RenderRID renderTarget;
        RenderRID renderBuffer;
        RenderRID shader;

        GLuint projectionLoc;
        GLuint viewLoc;
        GLuint roughenssLoc;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
    };

    struct BRDFMapRP: public RenderPass {
        RenderRID framebuffer;
        RenderRID renderBuffer;
        RenderRID shader;

        void init(RenderDatabase& database);
        void execute(RenderDatabase& database);
        void shutdown(RenderDatabase& database);
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

		void init(RenderDatabase& database);
		void execute(RenderDatabase& database);
		void shutdown(RenderDatabase& database);

	};

	struct VoxelDebugRP : public RenderPass {
		GLuint program;

		GLint voxelBufferLoc;

		GLuint dummyVAO;

		void init(RenderDatabase& database);
		void execute(RenderDatabase& database);
		void shutdown(RenderDatabase& database);
	};

	struct VoxelLightInjectRP : public RenderPass {
		GLuint program;

		GLint voxelAlbedoBufferLoc;
		GLint voxelNormalBufferLoc;
		GLint lightVoxelBufferLoc;
		
		void init(RenderDatabase& database);
		void execute(RenderDatabase& database);
		void shutdown(RenderDatabase& database);
	};

	struct VoxelMipmapGenRP : public RenderPass {
		GLuint program;

		void init(RenderDatabase& database);
		void execute(RenderDatabase& database);
		void shutdown(RenderDatabase& database);
	};

	struct VelocityBufferGenRP : public RenderPass {
		GLuint program;

		GLint depthMapLoc;
		GLint invCurProjectionViewLoc;
		GLint prevProjectionViewLoc;

		void init(RenderDatabase& database);
		void execute(RenderDatabase& database);
		void shutdown(RenderDatabase& database);
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

	struct AABB {
		Vec3f min;
		Vec3f max;
	};

    struct RenderDatabase {

        static constexpr int MAX_DIR_LIGHT = 4;

		uint32 frameIdx;

        uint32 targetWidthPx;
        uint32 targetHeightPx;

        Array<Material> materialBuffer;
        Array<Mesh> meshBuffer;

        Environment environment;
        DirectionalLight dirLights[MAX_DIR_LIGHT];
        int dirLightCount;

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

		AABB sceneBound;
    };

}
