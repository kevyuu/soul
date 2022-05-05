#pragma once
#include "core/type.h"
#include "core/geometry.h"
#include "../data.h"

namespace soul_fila
{

	class ShadowMap
	{
	public:
        struct DepthBias
        {
            float constant = 0.0f;
            float slope = 0.0f;
        };

		struct ShadowMapInfo {
            // the smallest increment in depth precision
            // e.g., for 16 bit depth textures, is this 1 / (2^16)
            float zResolution = 0.0f;

            // the dimension of the encompassing texture atlas
            uint16 atlasDimension = 0;

            // the dimension of a single shadow map texture within the atlas
            // e.g., for at atlas size of 1024 split into 4 quadrants, textureDimension would be 512
            uint16 textureDimension = 0;

            // the dimension of the actual shadow map, taking into account the 1 texel border
            // e.g., for a texture dimension of 512, shadowDimension would be 510
            uint16 shadowDimension = 0;

            // whether we're using vsm
            bool vsm = false;
        };

        struct SceneInfo {
            // The following fields are set by computeSceneCascadeParams.

            // light's near/far expressed in light-space, calculated from the scene's content
            // assuming the light is at the origin.
            soul::Vec2f lsNearFar;

            // Viewing camera's near/far expressed in view-space, calculated from the scene's content
        	soul::Vec2f vsNearFar;

            // World-space shadow-casters volume
        	soul::AABB wsShadowCastersVolume;

            // World-space shadow-receivers volume
        	soul::AABB wsShadowReceiversVolume;
        };

        struct LightInfo
        {
            LightType lightType;
            ShadowParams shadowParams;
            Vec3f direction;
            Vec3f position;
            float radius = 0.0f;
        };

        static ShadowMap Create(const ShadowMapInfo& shadowMapInfo, const LightInfo& lightInfo, const CameraInfo& viewCamera, const SceneInfo& sceneInfo, Vec2f csNearFar);
        void computeShadowCameraDirectional(const Vec3f& dir, const CameraInfo& camera, const ShadowParams& params, const SceneInfo& cascadeParams, Vec2f csNearFar) noexcept;
        soul::Mat4f getTextureCoordsMapping();

		soul::Mat4f sampleMatrix;

		soul::Mat4f renderViewMatrix;
        soul::Mat4f renderProjectionMatrix;
        float znear = 0.0f;
        float zfar = 0.0f;
        float texelSizeWs = 0.0f;

		bool hasVisibleShadow = false;

        DepthBias depthBias;
        ShadowMapInfo shadowMapInfo;
	};

    /**
	 * List of available shadow mapping techniques.
	 * @see setShadowType
	 */
    enum class ShadowType : uint8_t {
        PCF,        //!< percentage-closer filtered shadows (default)
        VSM         //!< variance shadows
    };

	class ShadowMapGenPass
	{
    public:

        struct Input
        {
            soul::gpu::BufferNodeID objectsUb;
            soul::gpu::BufferNodeID bonesUb;
            soul::gpu::BufferNodeID materialsUb;
        };

        struct Output
        {
            soul::gpu::TextureNodeID depthTarget;
        };

        using ShadowTechniqueFlagBits = enum {
			SHADOW_TECHNIQUE_SHADOW_MAP_BIT = 0x1,
            SHADOW_TECHNIQUE_SCREEN_SPACE_BIT = 0x2,
            BUFFER_USAGE_ENUM_END_BIT
        };
        using ShadowTechniqueFlags = uint8;

        void init(soul::gpu::System* gpuSystemIn, GPUProgramRegistry* programRegistryIn)
        {
            gpuSystem = gpuSystemIn;
            programRegistry = programRegistryIn;
        }

        ShadowTechniqueFlags prepare(const Scene& scene, const CameraInfo& cameraInfo, Renderables& renderables, Lights& lights, FrameUBO& frameUBO);
        ShadowType getShadowType() const { return shadowType; }

        Output computeRenderGraph(soul::gpu::RenderGraph& renderGraph, const Input& input, const RenderData& renderData, const Scene& scene);

	private:

        void calculateTextureRequirements(const Scene& scene, const Lights& lights);
        ShadowTechniqueFlags prepareCascadeShadowMaps(const Scene& scene, const CameraInfo& cameraInfo, Renderables& renderables, Lights& lights, FrameUBO& frameUBO);

        ShadowType shadowType = ShadowType::PCF;

        /**
         * View-level options for VSM Shadowing.
         * @see setVsmShadowOptions()
         * @warning This API is still experimental and subject to change.
         */
        struct VsmShadowOptions {
            /**
             * Sets the number of anisotropic samples to use when sampling a VSM shadow map. If greater
             * than 0, mipmaps will automatically be generated each frame for all lights.
             *
             * The number of anisotropic samples = 2 ^ vsmAnisotropy.
             */
            uint8_t anisotropy = 0;

            /**
             * Whether to generate mipmaps for all VSM shadow maps.
             */
            bool mipmapping = false;

            /**
             * EVSM exponent.
             * The maximum value permissible is 5.54 for a shadow map in fp16, or 42.0 for a
             * shadow map in fp32. Currently the shadow map bit depth is always fp16.
             */
            float exponent = 5.54f;

            /**
             * VSM minimum variance scale, must be positive.
             */
            float minVarianceScale = 0.5f;

            /**
             * VSM light bleeding reduction amount, between 0 and 1.
             */
            float lightBleedReduction = 0.15f;
        } vsmOptions;

        soul::gpu::TextureFormat textureFormat = soul::gpu::TextureFormat::DEPTH16;
        float textureZResolution = 1.0f / (1u << 16u);

        struct TextureRequirements {
            uint16_t size = 0;
            uint8_t layers = 0;
            uint8_t levels = 0;
        } textureRequirements;

        class CascadeSplits {
        public:
            constexpr static size_t SPLIT_COUNT = CONFIG_MAX_SHADOW_CASCADES + 1;

            struct Params {
                soul::Mat4f proj;
                float near = 0.0f;
                float far = 0.0f;
                size_t cascadeCount = 1;
                std::array<float, SPLIT_COUNT> splitPositions = { 0.0f };
            };

            CascadeSplits() noexcept : CascadeSplits(Params{}) {}
            explicit CascadeSplits(Params const& params) noexcept;

            // Split positions in world-space.
            const float* beginWs() const { return mSplitsWs; }
            const float* endWs() const { return mSplitsWs + mSplitCount; }

            // Split positions in clip-space.
            const float* beginCs() const { return mSplitsCs; }
            const float* endCs() const { return mSplitsCs + mSplitCount; }

        private:
            float mSplitsWs[SPLIT_COUNT];
            float mSplitsCs[SPLIT_COUNT];
            size_t mSplitCount = 0;
        } cascadeSplits;

        soul::Array<ShadowMap> cascadeShadowMaps;

        struct ShaderInfo
        {
            Vec4f cascadeSplits;
            float screenSpaceShadowDistance = 0.0f;
            uint32 directionalShadowMask = 0;
            uint32 cascades = 0;
        } shaderInfo;

        soul::gpu::System* gpuSystem = nullptr;;
        GPUProgramRegistry* programRegistry = nullptr;
	};
}