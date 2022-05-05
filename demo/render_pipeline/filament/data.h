#pragma once

#include "core/geometry.h"
#include "core/type.h"
#include "core/string.h"
#include "gpu/gpu.h"
#include "gpu_program_registry.h"
#include "../../data.h"
#include "../../entt.hpp"
#include "../../camera_manipulator.h"
#include "soa.h"
#include "range.h"

struct cgltf_data;
struct cgltf_node;

namespace soul_fila {
    using namespace soul;
	static constexpr uint64 MAX_ENTITY_NAME_LENGTH = 512;
    static constexpr uint64 MAX_MORPH_TARGETS = 4;
    
    // This value is limited by UBO size, ES3.0 only guarantees 16 KiB.
    // Values <= 256, use less CPU and GPU resources.
    constexpr size_t CONFIG_MAX_LIGHT_COUNT = 256;
    constexpr size_t CONFIG_MAX_LIGHT_INDEX = CONFIG_MAX_LIGHT_COUNT - 1;

    // The maximum number of spot lights in a scene that can cast shadows.
    // Light space coordinates are computed in the vertex shader and interpolated across fragments.
    // Thus, each additional shadow-casting spot light adds 4 additional varying components. Higher
    // values may cause the number of varyings to exceed the driver limit.
    constexpr size_t CONFIG_MAX_SHADOW_CASTING_SPOTS = 2;

    // The maximum number of shadow cascades that can be used for directional lights.
    constexpr size_t CONFIG_MAX_SHADOW_CASCADES = 4;

    // This value is also limited by UBO size, ES3.0 only guarantees 16 KiB.
    // We store 64 bytes per bone.
    constexpr size_t CONFIG_MAX_BONE_COUNT = 256;


    struct Mesh;
    using MeshID = ID<Mesh, uint64>;

    struct Animation;
    using AnimationID = ID<Animation, uint64>;

    struct Skin;
    using SkinID = ID<Skin, uint64>;

    struct Texture;
    using TextureID = ID<Texture, uint64>;

    struct Material;
    using MaterialID = ID<Texture, uint64>;

    using EntityID = entt::entity;
    static constexpr const EntityID ENTITY_ID_NULL = entt::null;
    using Registry = entt::registry;

	enum class VertexAttribute : uint8 {
		// Update hasIntegerTarget() in VertexBuffer when adding an attribute that will
		// be read as integers in the shaders

		POSITION = 0, //!< XYZ position (float3)
		QTANGENTS = 1, //!< tangent, bitangent and normal, encoded as a quaternion (float4)
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

        MORPH_BASE = MORPH_POSITION_0,
        MORPH_BASE_POSITION = MORPH_POSITION_0,
        MORPH_BASE_TANGENTS = MORPH_TANGENTS_0

		// this is limited by driver::MAX_VERTEX_ATTRIBUTE_COUNT
	};

	struct Attribute {
        static constexpr uint8 BUFFER_UNUSED = 0xFF;
		uint32 offset = 0;                    //!< attribute offset in bytes
		uint8 stride = 0;                     //!< attribute stride in bytes
		uint8 buffer = BUFFER_UNUSED;                  //!< attribute buffer index
		soul::gpu::VertexElementType elementType = soul::gpu::VertexElementType::BYTE;   //!< attribute element type
        soul::gpu::VertexElementFlags elementFlags = 0;
	};

	struct Primitive {
		soul::gpu::BufferID vertexBuffers[soul::gpu::MAX_VERTEX_BINDING];
		soul::gpu::BufferID indexBuffer;
        soul::gpu::Topology topology = soul::gpu::Topology::TRIANGLE_LIST;
        AABB aabb;
		Attribute attributes[to_underlying(VertexAttribute::COUNT)];
        MaterialID materialID;
		uint8 vertexBindingCount = 0;
		uint32 activeAttribute = 0;
	};

	struct Armature {
		soul::Array<EntityID> joints;
		soul::Array<EntityID> targets;
	};

    struct BoneUBO;
    struct Skin {
        soul::String name;
        soul::Array<soul::Mat4f> invBindMatrices;
        soul::Array<EntityID> joints;
        soul::Array<BoneUBO> bones;
    };

	struct Texture {
		soul::gpu::TextureID gpuHandle;
        soul::gpu::SamplerDesc samplerDesc;
	};


	struct Visibility {
		uint8_t priority : 3;
		bool castShadows : 1;
		bool receiveShadows : 1;
		bool culling : 1;
		bool skinning : 1;
		bool morphing : 1;
		bool screenSpaceContactShadows : 1;

        Visibility(): priority(0), castShadows(false), receiveShadows(false), culling(false), skinning(false), morphing(false), screenSpaceContactShadows(false) {}
	};


    using MaterialFlagBits = enum {
        MATERIAL_HAS_PBR_METALLIC_ROUGHNESS_BIT = 0x1,
        MATERIAL_HAS_PBR_SPECULAR_GLOSSINESS_BIT = 0x2,
        MATERIAL_HAS_CLEARCOAT = 0x4,
        MATERIAL_HAS_TRANSMISSION = 0x8,
        MATERIAL_HAS_VOLUME = 0x10,
        MATERIAL_HAS_IOR = 0x20,
        MATERIAL_HAS_SPECULAR = 0x40,
        MATERIAL_HAS_SHEEN = 0x80,
        MATERIAL_DOUBLE_SIDED = 0x100,
        MATERIAL_UNLIT = 0x200,
        MATERIAL_ENUM_END_BIT
    };
    using MaterialFlags = uint16;
    static_assert(MATERIAL_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(MaterialFlags), "");

    struct TextureView {
        TextureID textureID;
        soul::Mat3f transform;
        uint8 texCoord = 0;
    };

    /**
	 * How transparent objects are handled
	 */
    enum class TransparencyMode : uint8_t {
        //! the transparent object is drawn honoring the raster state
        DEFAULT,
        /**
         * the transparent object is first drawn in the depth buffer,
         * then in the color buffer, honoring the culling mode, but ignoring the depth test function
         */
         TWO_PASSES_ONE_SIDE,

         /**
          * the transparent object is drawn twice in the color buffer,
          * first with back faces only, then with front faces; the culling
          * mode is ignored. Can be combined with two-sided lighting
          */
          TWO_PASSES_TWO_SIDES
    };

    struct FrameUBO { // NOLINT(cppcoreguidelines-pro-type-member-init)

        soul::GLSLMat4f viewFromWorldMatrix;
        soul::GLSLMat4f worldFromViewMatrix;
        soul::GLSLMat4f clipFromViewMatrix;
        soul::GLSLMat4f viewFromClipMatrix;
        soul::GLSLMat4f clipFromWorldMatrix;
        soul::GLSLMat4f worldFromClipMatrix;
        soul::GLSLMat4f lightFromWorldMatrix[CONFIG_MAX_SHADOW_CASCADES];

        // position of cascade splits, in world space (not including the near plane)
        // -Inf stored in unused components
        soul::Vec4f cascadeSplits;

        soul::Vec4f resolution; // viewport width, height, 1/width, 1/height

        // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
        // Always add worldOffset in the shader to get the true world-space position of the camera.
        soul::Vec3f cameraPosition;

        float time; // time in seconds, with a 1 second period

        soul::Vec4f lightColorIntensity; // directional light

        soul::Vec4f sun; // cos(sunAngle), sin(sunAngle), 1/(sunAngle*HALO_SIZE-sunAngle), HALO_EXP

        soul::Vec4f padding0;

        soul::Vec3f lightDirection;
        uint32 fParamsX; // stride-x

        soul::Vec3f shadowBias; // unused, normal bias, unused
        float oneOverFroxelDimensionY;

        soul::Vec4f zParams; // froxel Z parameters

        soul::Vec2ui32 fParams; // stride-y, stride-z
        soul::Vec2f origin; // viewport left, viewport bottom

        float oneOverFroxelDimensionX;
        float iblLuminance;
        float exposure;
        float ev100;

        alignas(16) soul::Vec4f iblSH[9]; // actually float3 entries (std140 requires float4 alignment)

        soul::Vec4f userTime;  // time(s), (double)time - (float)time, 0, 0

        float iblRoughnessOneLevel;       // level for roughness == 1
        float cameraFar;                  // camera *culling* far-plane distance (projection far is at +inf)
        float refractionLodOffset;

        // bit 0: directional (sun) shadow enabled
        // bit 1: directional (sun) screen-space contact shadow enabled
        // bit 8-15: screen-space contact shadows ray casting steps
        uint32_t directionalShadows;

        soul::Vec3f worldOffset; // this is (0,0,0) when camera_at_origin is disabled
        float ssContactShadowDistance;

        // fog
        float fogStart;
        float fogMaxOpacity;
        float fogHeight;
        float fogHeightFalloff;         // falloff * 1.44269
        soul::Vec3f fogColor;
        float fogDensity;               // (density/falloff)*exp(-falloff*(camera.y - fogHeight))
        float fogInscatteringStart;
        float fogInscatteringSize;
        float fogColorFromIbl;

        // bit 0-3: cascade count
        // bit 4: visualize cascades
        // bit 8-11: cascade has visible shadows
        uint32_t cascades;

        float aoSamplingQualityAndEdgeDistance;     // 0: bilinear, !0: bilateral edge distance
        float aoReserved1;
        float aoReserved2;
        float aoReserved3;

        soul::Vec2f clipControl;
        soul::Vec2f padding1;

        float vsmExponent;
        float vsmDepthScale;
        float vsmLightBleedReduction;
        float vsmReserved0;

        // bring PerViewUib to 2 KiB
        soul::Vec4f padding2[59];
    };

    struct alignas(256) PerRenderableUBO {
        soul::GLSLMat4f worldFromModelMatrix;
        soul::GLSLMat3f worldFromModelNormalMatrix; // this gets expanded to 48 bytes during the copy to the UBO
        alignas(16) soul::Vec4f morphWeights;
        // TODO: we can pack all the boolean bellow
        int32_t skinningEnabled; // 0=disabled, 1=enabled, ignored unless variant & SKINNING_OR_MORPHING
        int32_t morphingEnabled; // 0=disabled, 1=enabled, ignored unless variant & SKINNING_OR_MORPHING
        uint32_t screenSpaceContactShadows; // 0=disabled, 1=enabled, ignored unless variant & SKINNING_OR_MORPHING                           // 0
        // TODO: We need a better solution, this currently holds the average local scale for the renderable
        float userData;

        static uint32_t packFlags(bool skinning, bool morphing, bool contactShadows) noexcept {
            return (skinning ? 1 : 0) |
                (morphing ? 2 : 0) |
                (contactShadows ? 4 : 0);
        }
    };

    struct LightUBO {
        soul::Vec4f positionFalloff;   // { float3(pos), 1/falloff^2 }
        soul::Vec4f colorIntensity;    // { float3(col), intensity }
        soul::Vec4f directionIES;      // { float3(dir), IES index }
        soul::Vec2f spotScaleOffset;   // { scale, offset }
        uint32_t    shadow = 0;            // { shadow bits (see ShadowInfo) }
        uint32_t    type = 0;              // { 0=point, 1=spot }
    };

    struct LightsUBO {
        LightUBO lights[CONFIG_MAX_LIGHT_COUNT];
    };

    struct FroxelRecordsUBO {
        Vec4ui32 records[1024];
    };

    // UBO for punctual (spot light) shadows.
    struct ShadowUBO {
        soul::GLSLMat4f spotLightFromWorldMatrix[CONFIG_MAX_SHADOW_CASTING_SPOTS];
        soul::Vec4f directionShadowBias[CONFIG_MAX_SHADOW_CASTING_SPOTS]; // light direction, normal bias
    };

    // This is not the UBO proper, but just an element of a bone array.
    struct PerRenderableUibBone {
        soul::Quaternionf q = { 1, 0, 0, 0 };
        soul::Vec4f t = {};
        soul::Vec4f s = { 1, 1, 1, 0 };
        soul::Vec4f ns = { 1, 1, 1, 0 };
    };

    struct alignas(16) MaterialUBO {
        soul::GLSLMat3f baseColorUvMatrix;
        soul::GLSLMat3f metallicRoughnessUvMatrix;
        soul::GLSLMat3f normalUvMatrix;
        soul::GLSLMat3f occlusionUvMatrix;
        soul::GLSLMat3f emissiveUvMatrix;
        soul::GLSLMat3f clearCoatUvMatrix;
        soul::GLSLMat3f clearCoatRoughnessMatrix;
        soul::GLSLMat3f clearCoatNormalUvMatrix;
        soul::GLSLMat3f sheenColorUvMatrix;
        soul::GLSLMat3f sheenRoughnessUvMatrix;
        soul::GLSLMat3f transmissionUvMatrix;
        soul::GLSLMat3f volumeThicknessUvMatrix;

        soul::Vec4f baseColorFactor;
        soul::Vec3f emissiveFactor;
        float pad1;
        soul::Vec3f specularFactor;
        float pad2;
        soul::Vec3f sheenColorFactor;
        float pad3;

        soul::Vec3f volumeAbsorption;
        float volumeThicknessFactor;
        Vec4f pad4;
        Vec4f pad5;
        Vec4f pad6;

        float roughnessFactor;
        float metallicFactor;
        float glossinessFactor;
        float normalScale;
        
        float transmissionFactor;
        float sheenRoughnessFactor;
        bool enableDiagnostics;
        float ior;

        float aoStrength;
        float clearCoatFactor;
        float clearCoatRoughnessFactor;
        float clearCoatNormalScale;
        
        float _maskThreshold;
        bool _doubleSided;
        float _specularAntiAliasingVariance;
        float _specularAntiAliasingThreshold;

    };

    struct BoneUBO {
        soul::Quaternionf q = { 1.0f, 0.0f, 0.0f, 0.0f };
        soul::Vec4f t;
        soul::Vec4f s = { 1.0f, 0.0f, 0.0f, 0.0f };
        soul::Vec4f ns = { 1.0f, 1.0f, 1.0f, 0.0f };
    };

    struct BonesUBO {
        BoneUBO bones[CONFIG_MAX_BONE_COUNT];
    };


    struct MaterialTextures {
        TextureID baseColorTexture;
        TextureID metallicRoughnessTexture;
        TextureID normalTexture;
        TextureID occlusionTexture;
        TextureID emissiveTexture;
        TextureID clearCoatTexture;
        TextureID clearCoatRoughnessTexture;
        TextureID clearCoatNormalTexture;
        TextureID sheenColorTexture;
        TextureID sheenRoughnessTexture;
        TextureID transmissionTexture;
        TextureID volumeThicknessTexture;

        MaterialTextures() = default;
    };

	struct Material {
        GPUProgramSetID programSetID;
        MaterialUBO buffer;
        MaterialTextures textures;
        TransparencyMode transparencyMode = TransparencyMode::DEFAULT;
        AlphaMode alphaMode = AlphaMode::OPAQUE;
        gpu::CullMode cullMode = gpu::CullMode::BACK;
	};

    struct TransformComponent {
        soul::Mat4f local;
        soul::Mat4f world;
        EntityID parent;
        EntityID firstChild;
        EntityID next;
        EntityID prev;

        void renderUI();
    };

    struct Mesh {
        AABB aabb;
        soul::Array<Primitive> primitives;
    };

	struct RenderComponent {
		Visibility visibility;
        MeshID meshID;
        SkinID skinID;
        soul::Vec4f morphWeights;
        uint8 layer;
	};

	struct SpotParams {
		float radius = 0;
		float outerClamped = 0;
		float cosOuterSquared = 1;
		float sinInverse = std::numeric_limits<float>::infinity();
		float luminousPower = 0;
		soul::Vec2f scaleOffset;
	};

	enum class IntensityUnit {
		LUMEN_LUX,  // intensity specified in lumens (for punctual lights) or lux (for directional)
		CANDELA     // intensity specified in candela (only applicable to punctual lights)
	};

    /**
     * Control the quality / performance of the shadow map associated to this light
     */
    struct ShadowOptions {
        /** Size of the shadow map in texels. Must be a power-of-two. */
        uint32_t mapSize = 1024;

        /**
         * Number of shadow cascades to use for this light. Must be between 1 and 4 (inclusive).
         * A value greater than 1 turns on cascaded shadow mapping (CSM).
         * Only applicable to Type.SUN or Type.DIRECTIONAL lights.
         *
         * When using shadow cascades, cascadeSplitPositions must also be set.
         *
         * @see ShadowOptions::cascadeSplitPositions
         */
        uint8_t shadowCascades = 1;

        /**
         * The split positions for shadow cascades.
         *
         * Cascaded shadow mapping (CSM) partitions the camera frustum into cascades. These values
         * determine the planes along the camera's Z axis to split the frustum. The camera near
         * plane is represented by 0.0f and the far plane represented by 1.0f.
         *
         * For example, if using 4 cascades, these values would set a uniform split scheme:
         * { 0.25f, 0.50f, 0.75f }
         *
         * For N cascades, N - 1 split positions will be read from this array.
         *
         * Filament provides utility methods inside LightManager::ShadowCascades to help set these
         * values. For example, to use a uniform split scheme:
         *
         * ~~~~~~~~~~~{.cpp}
         *   LightManager::ShadowCascades::computeUniformSplits(options.splitPositions, 4);
         * ~~~~~~~~~~~
         *
         * @see ShadowCascades::computeUniformSplits
         * @see ShadowCascades::computeLogSplits
         * @see ShadowCascades::computePracticalSplits
         */
        float cascadeSplitPositions[3] = { 0.25f, 0.50f, 0.75f };

        /** Constant bias in world units (e.g. meters) by which shadows are moved away from the
         * light. 1mm by default.
         */
        float constantBias = 0.001f;

        /** Amount by which the maximum sampling error is scaled. The resulting value is used
         * to move the shadow away from the fragment normal. Should be 1.0.
         */
        float normalBias = 1.0f;

        /** Distance from the camera after which shadows are clipped. this is used to clip
         * shadows that are too far and wouldn't contribute to the scene much, improving
         * performance and quality. This value is always positive.
         * Use 0.0f to use the camera far distance.
         */
        float shadowFar = 0.0f;

        /** Optimize the quality of shadows from this distance from the camera. Shadows will
         * be rendered in front of this distance, but the quality may not be optimal.
         * This value is always positive. Use 0.0f to use the camera near distance.
         * The default of 1m works well with many scenes. The quality of shadows may drop
         * rapidly when this value decreases.
         */
        float shadowNearHint = 1.0f;

        /** Optimize the quality of shadows in front of this distance from the camera. Shadows
         * will be rendered behind this distance, but the quality may not be optimal.
         * This value is always positive. Use std::numerical_limits<float>::infinity() to
         * use the camera far distance.
         */
        float shadowFarHint = 100.0f;

        /**
         * Controls whether the shadow map should be optimized for resolution or stability.
         * When set to true, all resolution enhancing features that can affect stability are
         * disabling, resulting in significantly lower resolution shadows, albeit stable ones.
         */
        bool stable = false;

        /**
         * Constant bias in depth-resolution units by which shadows are moved away from the
         * light. The default value of 0.5 is used to round depth values up.
         * Generally this value shouldn't be changed or at least be small and positive.
         */
        float polygonOffsetConstant = 0.5f;

        /**
         * Bias based on the change in depth in depth-resolution units by which shadows are moved
         * away from the light. The default value of 2.0 works well with SHADOW_SAMPLING_PCF_LOW.
         * Generally this value is between 0.5 and the size in texel of the PCF filter.
         * Setting this value correctly is essential for LISPSM shadow-maps.
         */
        float polygonOffsetSlope = 2.0f;

        /**
         * Whether screen-space contact shadows are used. This applies regardless of whether a
         * Renderable is a shadow caster.
         * Screen-space contact shadows are typically useful in large scenes.
         * (off by default)
         */
        bool screenSpaceContactShadows = false;

        /**
         * Number of ray-marching steps for screen-space contact shadows (8 by default).
         *
         * CAUTION: this parameter is ignored for all lights except the directional/sun light,
         *          all other lights use the same value set for the directional/sun light.
         *
         */
        uint8_t stepCount = 8;

        /**
         * Maximum shadow-occluder distance for screen-space contact shadows (world units).
         * (30 cm by default)
         *
         * CAUTION: this parameter is ignored for all lights except the directional/sun light,
         *          all other lights use the same value set for the directional/sun light.
         *
         */
        float maxShadowDistance = 0.3f;

        /**
         * Options available when the View's ShadowType is set to VSM.
         *
         * @warning This API is still experimental and subject to change.
         * @see View::setShadowType
         */
        struct {
            /**
             * The number of MSAA samples to use when rendering VSM shadow maps.
             * Must be a power-of-two and greater than or equal to 1. A value of 1 effectively turns
             * off MSAA.
             * Higher values may not be available depending on the underlying hardware.
             */
            uint8_t msaaSamples = 1;

            /**
             * Blur width for the VSM blur. Zero do disable.
             * The maximum value is 125.
             */
            float blurWidth = 0.0f;
        } vsm;
    };

    struct ShadowCascades {
        /**
         * Utility method to compute ShadowOptions::cascadeSplitPositions according to a uniform
         * split scheme.
         *
         * @param splitPositions    a float array of at least size (cascades - 1) to write the split
         *                          positions into
         * @param cascades          the number of shadow cascades, at most 4
         */
        static void computeUniformSplits(float* splitPositions, uint8_t cascades);

        /**
         * Utility method to compute ShadowOptions::cascadeSplitPositions according to a logarithmic
         * split scheme.
         *
         * @param splitPositions    a float array of at least size (cascades - 1) to write the split
         *                          positions into
         * @param cascades          the number of shadow cascades, at most 4
         * @param near              the camera near plane
         * @param far               the camera far plane
         */
        static void computeLogSplits(float* splitPositions, uint8_t cascades,
            float near, float far);

        /**
         * Utility method to compute ShadowOptions::cascadeSplitPositions according to a practical
         * split scheme.
         *
         * The practical split scheme uses uses a lambda value to interpolate between the logarithmic
         * and uniform split schemes. Start with a lambda value of 0.5f and adjust for your scene.
         *
         * See: Zhang et al 2006, "Parallel-split shadow maps for large-scale virtual environments"
         *
         * @param splitPositions    a float array of at least size (cascades - 1) to write the split
         *                          positions into
         * @param cascades          the number of shadow cascades, at most 4
         * @param near              the camera near plane
         * @param far               the camera far plane
         * @param lambda            a float in the range [0, 1] that interpolates between log and
         *                          uniform split schemes
         */
        static void computePracticalSplits(float* splitPositions, uint8_t cascades,
            float near, float far, float lambda);
    };

	struct ShadowParams {
		ShadowOptions options;
	};

    //! Denotes the type of the light being created.
    enum class LightRadiationType : uint8_t {
        SUN,            //!< Directional light that also draws a sun's disk in the sky.
        DIRECTIONAL,    //!< Directional light, emits light in a given direction.
        POINT,          //!< Point light, emits light from a position, in all directions.
        FOCUSED_SPOT,   //!< Physically correct spot light.
        SPOT,           //!< Spot light with coupling of outer cone and illumination disabled.
        COUNT
    };

    struct LightType {
        LightRadiationType type : 3;
        bool shadowCaster : 1;
        bool lightCaster : 1;
        LightType() : type(LightRadiationType::COUNT), shadowCaster(false), lightCaster(false){}
        LightType(LightRadiationType type, bool shadowCaster, bool lightCaster) : type(type), shadowCaster(shadowCaster), lightCaster(lightCaster) {}
    };

	struct LightComponent {
        LightType lightType;
        soul::Vec3f position;
        soul::Vec3f direction;
        soul::Vec3f color;
        ShadowParams shadowParams;
        SpotParams spotParams;
        float sunAngularRadius;
        float sunHaloSize;
        float sunHaloFalloff;
        float intensity;
        float squaredFallOffInv;
	};

    struct CameraComponent {
    private:
        soul::Mat4f projection;                // projection matrix (infinite far)
        soul::Mat4f projectionForCulling;      // projection matrix (with far plane)
        soul::Vec2f scaling = { 1.0f, 1.0f };  // additional scaling applied to projection
        soul::Vec2f shiftCS = { 0.0f, 0.0f };  // additional translation applied to projection

        float near{};
        float far{};
        // exposure settings
        float aperture = 16.0f;
        float shutterSpeed = 1.0f / 125.0f;
        float sensitivity = 100.0f;
        float focusDistance = 0.0f;
    public:
        static constexpr const float SENSOR_SIZE = 0.024f;    // 24mm

        void setLensProjection(float focalLengthInMilimeters, float aspect, float inNear, float inFar);
        void setOrthoProjection(float left, float right, float bottom, float top, float inNear, float inFar);
        void setPerspectiveProjection(float fovRadian, float aspect, float inNear, float inFar);
        soul::Mat4f getProjectionMatrix() const;
        soul::Mat4f getCullingProjectionMatrix() const;
        void setScaling(Vec2f scaling);
        float getNear() const { return near; }
        float getCullingFar() const { return far; }
        float getAperture() const { return aperture; }
        float getShutterSpeed() const { return shutterSpeed; }
        float getSensitivity() const { return sensitivity; }
        float getFocalLength() const { return (SENSOR_SIZE * projection.elem[1][1]) * 0.5f; }
        float getFocusDistance() const { return focusDistance; }
        soul::Vec2f getScaling() const;
    };

    struct NameComponent {
        soul::String name;
        void renderUI();
    };

    struct CameraInfo {

        static float compute_ev100(float aperture, float shutterSpeed, float sensitivity) noexcept {
            // With N = aperture, t = shutter speed and S = sensitivity,
            // we can compute EV100 knowing that:
            //
            // EVs = log2(N^2 / t)
            // and
            // EVs = EV100 + log2(S / 100)
            //
            // We can therefore find:
            //
            // EV100 = EVs - log2(S / 100)
            // EV100 = log2(N^2 / t) - log2(S / 100)
            // EV100 = log2((N^2 / t) * (100 / S))
            //
            // Reference: https://en.wikipedia.org/wiki/Exposure_value
            return std::log2((aperture * aperture) / shutterSpeed * 100.0f / sensitivity);
        }


        CameraInfo() noexcept = default;

        CameraInfo(const TransformComponent& transform, const CameraComponent& camera, const soul::Mat4f& world_origin_transform) {
            projection = camera.getProjectionMatrix();
            cullingProjection = camera.getCullingProjectionMatrix();
            model = world_origin_transform * transform.world;
            view = mat4Inverse(model);
            zn = camera.getNear();
            zf = camera.getCullingFar();
            ev100 = compute_ev100(camera.getAperture(), camera.getShutterSpeed(), camera.getSensitivity());
            f = camera.getFocalLength();
            A = f / camera.getAperture();
            d = std::max(zn, camera.getFocusDistance());
            worldOffset = transform.world.columns(3).xyz;
            worldOrigin = world_origin_transform;
        }

        soul::Mat4f projection;         // projection matrix for drawing (infinite zfar)
        soul::Mat4f cullingProjection;  // projection matrix for culling
        soul::Mat4f model;              // camera model matrix
        soul::Mat4f view;               // camera view matrix
        float zn{};                     // distance (positive) to the near plane
        float zf{};                     // distance (positive) to the far plane
        float ev100{};                  // exposure
        float f{};                      // focal length [m]
        float A{};                      // f-number or f / aperture diameter [m]
        float d{};                      // focus distance [m]
        soul::Vec3f worldOffset;
        soul::Mat4f worldOrigin;
        soul::Vec3f get_position() const noexcept { return soul::Vec3f(model.elem[0][3], model.elem[1][3], model.elem[2][3]); }
        soul::Vec3f get_forward_vector() const noexcept { return unit(soul::Vec3f(model.elem[0][2], model.elem[1][2], model.elem[2][2]) * -1.0f); }
        soul::Frustum get_culling_frustum() const noexcept { return Frustum(cullingProjection * view);  }
    };

    struct AnimationSampler {
        soul::Array<float> times;
        soul::Array<float> values;
        enum { LINEAR, STEP, CUBIC } interpolation = LINEAR;
    };

    struct AnimationChannel {
        uint32 samplerIdx;
        EntityID entity;
        enum { TRANSLATION, ROTATION, SCALE, WEIGHTS } transformType;
    };

    struct Animation {
        soul::String name;
        float duration = 0.0f;
        soul::Array<AnimationSampler> samplers;
        soul::Array<AnimationChannel> channels;
    };

    struct IBL
    {
        gpu::TextureID reflectionTex;
        gpu::TextureID irradianceTex;
        Vec3f irradianceCoefs[9] = {{65504.0f, 65504.0f, 65504.0f}};
        Vec3f mBands[9] = {};

        Mat3f rotation = mat3Identity();
        float intensity = 30000.0f;
    };

    struct DFG
    {
        gpu::TextureID tex;
        static constexpr size_t LUT_SIZE = 128;
        // this lookup table is generated with cmgen
        static const uint16_t LUT[];
    };

    using VisibleMask = uint8;
    static constexpr size_t VISIBLE_RENDERABLE_BIT = 0u;
    static constexpr size_t VISIBLE_DIR_SHADOW_RENDERABLE_BIT = 1u;
    static constexpr size_t VISIBLE_SPOT_SHADOW_RENDERABLE_N_BIT(size_t n) { return n + 2; }

    static constexpr uint8_t VISIBLE_RENDERABLE = 1u << VISIBLE_RENDERABLE_BIT;
    static constexpr uint8_t VISIBLE_DIR_SHADOW_RENDERABLE = 1u << VISIBLE_DIR_SHADOW_RENDERABLE_BIT;
    static constexpr uint8_t VISIBLE_SPOT_SHADOW_RENDERABLE_N(size_t n) {
        return 1u << VISIBLE_SPOT_SHADOW_RENDERABLE_N_BIT(n);
    }

    // ORing of all the VISIBLE_SPOT_SHADOW_RENDERABLE bits
    static constexpr uint8_t VISIBLE_SPOT_SHADOW_RENDERABLE =
        (0xFFu >> (sizeof(uint8_t) * 8u - CONFIG_MAX_SHADOW_CASTING_SPOTS)) << 2u;

    enum class RenderablesIdx : uint8
    {
        RENDERABLE_ENTITY_ID,
        WORLD_TRANSOFRM,
        REVERSED_WINDING_ORDER,
        VISIBILITY_STATE,
        SKIN_ID,
        WORLD_AABB_CENTER,
        VISIBLE_MASK,
        MORPH_WEIGHTS,

        LAYERS,
        WORLD_AABB_EXTENT,
        PRIMITIVES,
        SUMMED_PRIMITIVE_COUNT,

        USER_DATA
    };

    using Renderables = SoaPool<
        RenderablesIdx,
        EntityID,
        soul::Mat4f,
        bool,
        Visibility,
        SkinID,
        Vec3f,
        uint8,
        Vec4f,

		uint8,
        Vec3f,
		const Array<Primitive>*,
        uint32,

        float
    >;

    enum class LightsIdx : uint8
    {
	    POSITION_RADIUS,
        DIRECTION,
        ENTITY_ID,
        VISIBLE_MASK,
        SCREEN_SPACE_Z_RANGE,
        SHADOW_INFO
    };

    struct ShadowInfo {
        // These are per-light values.
        // They're packed into 32 bits and stored in the Lights uniform buffer.
        // They're unpacked in the fragment shader and used to calculate punctual shadows.
        bool castsShadows = false;      // whether this light casts shadows
        bool contactShadows = false;    // whether this light casts contact shadows
        uint8_t index = 0;              // an index into the arrays in the Shadows uniform buffer
        uint8_t layer = 0;              // which layer of the shadow texture array to sample from

        //  -- LSB -------------
        //  castsShadows     : 1
        //  contactShadows   : 1
        //  index            : 4
        //  layer            : 4
        //  -- MSB -------------
        uint32_t pack() const {
            return uint8_t(castsShadows) << 0u |
                uint8_t(contactShadows) << 1u |
                index << 2u |
                layer << 6u;
        }
    };

    using Lights = SoaPool <
        LightsIdx,
        soul::Vec4f,
        soul::Vec3f,
        EntityID,
        VisibleMask,
        Vec2f,
        ShadowInfo
    >;

    using RenderFlags = uint8_t;
    static constexpr RenderFlags HAS_SHADOWING = 0x01;
    static constexpr RenderFlags HAS_DIRECTIONAL_LIGHT = 0x02;
    static constexpr RenderFlags HAS_DYNAMIC_LIGHTING = 0x04;
    static constexpr RenderFlags HAS_FOG = 0x10;
    static constexpr RenderFlags HAS_VSM = 0x20;

    struct RenderData
    {
        using RenderRange = Range<uint32>;

        Renderables renderables;
        Lights lights;
        RenderRange visibleRenderables;
        RenderRange directionalShadowCasters;
        RenderRange spotLightShadowCasters;
        RenderRange merged;
        CameraInfo cameraInfo;

        FrameUBO frameUBO;
        LightsUBO lightsUBO;
        ShadowUBO shadowUBO;
        soul::Array<MaterialUBO> materialUBOs;
        soul::Array<PerRenderableUBO> renderableUBOs;
        soul::Array<BonesUBO> bonesUBOs;

        RenderFlags flags;

        gpu::TextureID stubTexture;
        gpu::TextureID stubTextureUint;
        gpu::TextureID stubTextureArray;

        gpu::BufferID fullscreenVb;
        gpu::BufferID fullscreenIb;

        void clear()
        {
            renderables.clear();
            lights.clear();
            visibleRenderables = RenderRange();
            directionalShadowCasters = RenderRange();
            spotLightShadowCasters = RenderRange();
            merged = RenderRange();
            cameraInfo = CameraInfo();
            frameUBO = FrameUBO();
            lightsUBO = LightsUBO();
            shadowUBO = ShadowUBO();
            materialUBOs.clear();
            renderableUBOs.clear();
            bonesUBOs.clear();
            flags = 0;
        }
    };

    struct LightDesc
    {
        LightType type;
        Vec3f position;
        float falloff = 1.0f;
        Vec3f linearColor;
        float intensity = 100000.0f;
        IntensityUnit intensityUnit = IntensityUnit::LUMEN_LUX;
        Vec3f direction = { 0.0f, -1.0f, 0.0f };
        Vec2f spotInnerOuter = { soul::Fconst::PI, soul::Fconst::PI };
        float sunAngle = 0.00951f; // 0.545° in radians
        float sunHaloSize = 10.0f;
        float sunHaloFalloff = 80.0f;
        ShadowOptions shadowOptions;
    };

    template <typename Func>
    concept mesh_generator = std::invocable<Func, soul_size, Mesh&> && std::same_as<void, std::invoke_result_t<Func, soul_size, Mesh&>>;

    template <typename Func>
    concept animation_generator = std::invocable<Func, soul_size, Animation&> && std::same_as<void, std::invoke_result_t<Func, soul_size, Animation&>>;


	class Scene final : Demo::Scene {
	public:
        static constexpr soul_size DIRECTIONAL_LIGHTS_COUNT = 1;

        Scene(soul::gpu::System* gpuSystem, GPUProgramRegistry* programRegistry);
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = delete;
        Scene& operator=(Scene&&) = delete;
        ~Scene() override = default;

        void import_from_gltf(const char* path) override final;
        void cleanup() override {}
        void renderPanels() override;
        bool update(const Demo::Input& input) override;
        Vec2ui32 get_viewport() const override { return viewport_; }
        void set_viewport(Vec2ui32 viewport) override { viewport_ = viewport; }

        const soul::Array<Texture>& textures() const { return textures_; }
        const soul::Array<Mesh>& meshes() const { return meshes_; } 
        const soul::Array<Material>& materials() const { return materials_; }
        const soul::Array<Skin>& skins() const { return skins_; }
        const IBL& get_ibl() const { return ibl_; }
        const DFG& get_dfg() const { return dfg_; }

        
        template <mesh_generator MeshGenerator>
        void create_meshes(soul_size count, MeshGenerator generator)
        {
            soul_size old_size = meshes_.size();
            meshes_.resize(old_size + count);
            Slice<Mesh> new_meshes(&meshes_, old_size, old_size + count);
            for (soul_size idx = 0; idx < count; idx++)
            {
                generator(idx, new_meshes[idx]);
            }

            /*soul::runtime::TaskID mesh_gen_parent = soul::runtime::create_task(soul::runtime::TaskID::ROOT(), [](runtime::TaskID){});
            soul::runtime::parallel_for_task_create(mesh_gen_parent, count, 8, [&new_meshes, &generator](soul_size idx)
            {
            	generator(idx, new_meshes[idx]);
            });
            soul::runtime::run_task(mesh_gen_parent);
            soul::runtime::wait_task(mesh_gen_parent);*/
        }

        template <animation_generator AnimationGenerator>
        void create_animations_parallel(soul_size count, AnimationGenerator generator)
        {
            soul_size old_size = animations_.size();
            animations_.resize(old_size + count);
            Slice<Animation> new_animations(&animations_, old_size, old_size + count);
            soul::runtime::TaskID animation_gen_task = soul::runtime::parallel_for_task_create(soul::runtime::TaskID::ROOT(), count, 8, [&new_animations, &generator](soul_size idx)
                {
                    generator(idx, new_animations[idx]);
                });
            soul::runtime::run_and_wait_task(animation_gen_task);
        }

        MaterialID create_material() { return MaterialID(materials_.add({})); }
        Material* get_material_ptr(const MaterialID material_id) { return &materials_[material_id.id]; }

        TextureID create_texture() { return TextureID(textures_.add({})); }
        Texture* get_texture_ptr(const TextureID texture_id) { return &textures_[texture_id.id]; }

        SkinID create_skin() { return SkinID(skins_.add(Skin())); }
        Skin* get_skin_ptr(const SkinID skin_id) { return &skins_[skin_id.id]; }

        soul_size get_renderable_count() const
        {
            auto group = registry_.group<const TransformComponent, const RenderComponent>();
            return group.size();
        }

        template<typename Func>
        void for_each_renderable(Func func) const
        {
            auto group = registry_.group<const TransformComponent, const RenderComponent>();

            for (auto entity: group)
            {
                func(entity, group.get<const TransformComponent>(entity), group.get<const RenderComponent>(entity));
            }
        }

        soul_size get_light_count() const
        {
            auto group = registry_.group<const LightComponent>(entt::get<const TransformComponent>);
            return group.size();
        }

        template<typename Func>
        void for_each_light(Func func) const
        {
            auto group = registry_.group<const LightComponent>(entt::get<const TransformComponent>);
            for (auto entity: group)
            {
                func(entity, group.get<const TransformComponent>(entity), group.get<const LightComponent>(entity));
            }
        }

        CameraInfo get_active_camera(const soul::Mat4f& world_origin_transform) const
        {
            auto [cameraTransformComp, cameraComp] = registry_.get<const TransformComponent, const CameraComponent>(active_camera_);
            return CameraInfo(cameraTransformComp, cameraComp, world_origin_transform);
        }
        
        const LightComponent& get_light_component(EntityID entityID) const { return registry_.get<LightComponent>(entityID);  }
        uint8 get_visible_layers() const { return visible_layers_; }

        struct FogOptions;
        const FogOptions& get_fog_options() { return fogOptions; }

        EntityID create_light(const LightDesc& light_desc, EntityID parent = ENTITY_ID_NULL);
        void create_dfg(const char* path, const char* name);

        template <typename ComponentType>
        ComponentType& add_component(EntityID entity_id, const ComponentType& component_type)
        {
            return registry_.emplace<ComponentType>(entity_id, component_type);
        }

        template <typename ComponentType>
        ComponentType& get_component(EntityID entity_id)
        {
            return registry_.get<ComponentType>(entity_id);
        }

        EntityID create_entity(const soul::String& name)
        {
            EntityID entity_id = registry_.create();
            add_component<NameComponent>(entity_id, NameComponent(name));
            return entity_id;
        }

        bool is_light(EntityID entity_id) const;
        auto is_directional_light(EntityID entity_id) const -> bool;
        bool is_sun_light(EntityID entity_id) const;
        bool is_spot_light(EntityID entity_id) const;
        void set_light_shadow_options(EntityID entity_id, const ShadowOptions& options);
        void set_light_local_position(EntityID entity_id, Vec3f position);
        void set_light_local_direction(EntityID entity_id, Vec3f direction);
        void set_light_color(EntityID entity_id, Vec3f color);
        void set_light_intensity(EntityID entity_id, float intensity, IntensityUnit unit);
        void set_light_falloff(EntityID entity_id, float falloff);
        void set_light_cone(EntityID entity_id, float inner, float outer);
        void set_light_sun_angular_radius(EntityID entity_id, float angular_radius);
        void set_light_sun_halo_size(EntityID entity_id, float halo_size);
        void set_light_sun_halo_falloff(EntityID entity_id, float halo_falloff);

        EntityID get_root_entity() const { return root_entity_; }

        void update_bounding_box();
        void fit_into_unit_cube();

        EntityID get_default_camera() { return default_camera_; }

        void create_default_sunlight();
        void create_default_camera();
        bool check_resources_validity();

    private:

        void create_root_entity();

        auto render_entity_tree_node(EntityID entityID) -> void;
        void set_active_animation(AnimationID animationID);
        void set_active_camera(EntityID camera);
        void update_world_transform(EntityID entityID);
        void update_bones();

        EntityID root_entity_ = ENTITY_ID_NULL;
        mutable Registry registry_;
        soul::gpu::System* gpu_system_;
        GPUProgramRegistry* program_registry_;

        soul::Array<Texture> textures_;
        soul::Array<Material> materials_;
        soul::Array<Mesh> meshes_;
        soul::Array<Skin> skins_;
        soul::Array<Animation> animations_;
        soul::AABB bounding_box_;
        IBL ibl_;
        DFG dfg_;

        EntityID selected_entity_ = ENTITY_ID_NULL;
        EntityID active_camera_ = ENTITY_ID_NULL;
        EntityID default_camera_ = ENTITY_ID_NULL;

        AnimationID active_animation_;
        float animation_delta_ = 0.0f;
        Array<uint64> channel_cursors_;
        bool reset_animation_ = false;
        
        CameraManipulator camera_man_;

        Vec2ui32 viewport_;

        uint8 visible_layers_ = 0x1;

        /**
		 * Options to control fog in the scene
		 */
        struct FogOptions {
            float distance = 0.0f;              //!< distance in world units from the camera where the fog starts ( >= 0.0 )
            float maximumOpacity = 1.0f;        //!< fog's maximum opacity between 0 and 1
            float height = 0.0f;                //!< fog's floor in world units
            float heightFalloff = 1.0f;         //!< how fast fog dissipates with altitude
            Vec3f color = Vec3f(0.5f);     //!< fog's color (linear), see fogColorFromIbl
            float density = 0.1f;               //!< fog's density at altitude given by 'height'
            float inScatteringStart = 0.0f;     //!< distance in world units from the camera where in-scattering starts
            float inScatteringSize = -1.0f;     //!< size of in-scattering (>0 to activate). Good values are >> 1 (e.g. ~10 - 100).
            bool fogColorFromIbl = false;       //!< Fog color will be modulated by the IBL color in the view direction.
            bool enabled = false;               //!< enable or disable fog
        } fogOptions;
	
    };

}