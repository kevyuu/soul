#pragma once

#include "core/type.h"
#include "core/string.h"
#include "gpu/gpu.h"
#include "gpu_program_registry.h"
#include "../../data.h"
#include "../../entt.hpp"
#include "../../camera_manipulator.h"

struct cgltf_data;
struct cgltf_node;
namespace SoulFila {
    using namespace Soul;
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

	enum VertexAttribute : uint8 {
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

	struct Attribute {
        static constexpr uint8 BUFFER_UNUSED = 0xFF;
		uint32 offset = 0;                    //!< attribute offset in bytes
		uint8 stride = 0;                     //!< attribute stride in bytes
		uint8 buffer = BUFFER_UNUSED;                  //!< attribute buffer index
		Soul::GPU::VertexElementType elementType = Soul::GPU::VertexElementType::BYTE;   //!< attribute element type
        Soul::GPU::VertexElementFlags elementFlags = 0;
	};

	struct Primitive {
		Soul::GPU::BufferID vertexBuffers[Soul::GPU::MAX_VERTEX_BINDING];
		Soul::GPU::BufferID indexBuffer;
        Soul::GPU::Topology topology = Soul::GPU::Topology::TRIANGLE_LIST;
        AABB aabb;
		Attribute attributes[VertexAttribute::COUNT];
        MaterialID materialID;
		uint8 vertexBindingCount = 0;
		uint32 activeAttribute = 0;
	};

	struct Armature {
		Soul::Array<EntityID> joints;
		Soul::Array<EntityID> targets;
	};

    struct BoneUBO;
    struct Skin {
        Soul::String name;
        Soul::Array<Soul::Mat4f> invBindMatrices;
        Soul::Array<EntityID> joints;
        Soul::Array<BoneUBO> bones;
    };

	struct Texture {
		Soul::GPU::TextureID gpuHandle;
        Soul::GPU::SamplerDesc samplerDesc;
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
        Soul::Mat3f transform;
        uint8 texCoord = 0;
    };

    struct FrameUBO { // NOLINT(cppcoreguidelines-pro-type-member-init)

        Soul::GLSLMat4f viewFromWorldMatrix;
        Soul::GLSLMat4f worldFromViewMatrix;
        Soul::GLSLMat4f clipFromViewMatrix;
        Soul::GLSLMat4f viewFromClipMatrix;
        Soul::GLSLMat4f clipFromWorldMatrix;
        Soul::GLSLMat4f worldFromClipMatrix;
        Soul::GLSLMat4f lightFromWorldMatrix[CONFIG_MAX_SHADOW_CASCADES];

        // position of cascade splits, in world space (not including the near plane)
        // -Inf stored in unused components
        Soul::Vec4f cascadeSplits;

        Soul::Vec4f resolution; // viewport width, height, 1/width, 1/height

        // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
        // Always add worldOffset in the shader to get the true world-space position of the camera.
        Soul::Vec3f cameraPosition;

        float time; // time in seconds, with a 1 second period

        Soul::Vec4f lightColorIntensity; // directional light

        Soul::Vec4f sun; // cos(sunAngle), sin(sunAngle), 1/(sunAngle*HALO_SIZE-sunAngle), HALO_EXP

        Soul::Vec4f padding0;

        Soul::Vec3f lightDirection;
        uint32 fParamsX; // stride-x

        Soul::Vec3f shadowBias; // unused, normal bias, unused
        float oneOverFroxelDimensionY;

        Soul::Vec4f zParams; // froxel Z parameters

        Soul::Vec2ui32 fParams; // stride-y, stride-z
        Soul::Vec2f origin; // viewport left, viewport bottom

        float oneOverFroxelDimensionX;
        float iblLuminance;
        float exposure;
        float ev100;

        alignas(16) Soul::Vec4f iblSH[9]; // actually float3 entries (std140 requires float4 alignment)

        Soul::Vec4f userTime;  // time(s), (double)time - (float)time, 0, 0

        float iblRoughnessOneLevel;       // level for roughness == 1
        float cameraFar;                  // camera *culling* far-plane distance (projection far is at +inf)
        float refractionLodOffset;

        // bit 0: directional (sun) shadow enabled
        // bit 1: directional (sun) screen-space contact shadow enabled
        // bit 8-15: screen-space contact shadows ray casting steps
        uint32_t directionalShadows;

        Soul::Vec3f worldOffset; // this is (0,0,0) when camera_at_origin is disabled
        float ssContactShadowDistance;

        // fog
        float fogStart;
        float fogMaxOpacity;
        float fogHeight;
        float fogHeightFalloff;         // falloff * 1.44269
        Soul::Vec3f fogColor;
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

        Soul::Vec2f clipControl;
        Soul::Vec2f padding1;

        float vsmExponent;
        float vsmDepthScale;
        float vsmLightBleedReduction;
        float vsmReserved0;

        // bring PerViewUib to 2 KiB
        Soul::Vec4f padding2[59];
    };

    struct alignas(256) PerRenderableUBO {
        Soul::GLSLMat4f worldFromModelMatrix;
        Soul::GLSLMat3f worldFromModelNormalMatrix; // this gets expanded to 48 bytes during the copy to the UBO
        alignas(16) Soul::Vec4f morphWeights;
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
        Soul::Vec4f positionFalloff;   // { float3(pos), 1/falloff^2 }
        Soul::Vec4f colorIntensity;    // { float3(col), intensity }
        Soul::Vec4f directionIES;      // { float3(dir), IES index }
        Soul::Vec2f spotScaleOffset;   // { scale, offset }
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
        Soul::GLSLMat4f spotLightFromWorldMatrix[CONFIG_MAX_SHADOW_CASTING_SPOTS];
        Soul::Vec4f directionShadowBias[CONFIG_MAX_SHADOW_CASTING_SPOTS]; // light direction, normal bias
    };

    // This is not the UBO proper, but just an element of a bone array.
    struct PerRenderableUibBone {
        Soul::Quaternionf q = { 1, 0, 0, 0 };
        Soul::Vec4f t = {};
        Soul::Vec4f s = { 1, 1, 1, 0 };
        Soul::Vec4f ns = { 1, 1, 1, 0 };
    };

    struct MaterialUBO {
        Soul::Vec4f baseColorFactor;
        Soul::Vec3f emissiveFactor;
        float pad1;
        Soul::Vec3f specularFactor;
        float pad2;
        Soul::Vec3f sheenColorFactor;
        float pad3;
        
        float roughnessFactor;
        float metallicFactor;
        float glossinessFactor;
        float normalScale;
        
        float transmissionFactor;
        float sheenRoughnessFactor;
        bool enableDiagnostics;
        float pad4;

        float aoStrength;
        float clearCoatFactor;
        float clearCoatRoughnessFactor;
        float clearCoatNormalScale;
        
        float _maskThreshold;
        bool _doubleSided;
        float _specularAntiAliasingVariance;
        float _specularAntiAliasingThreshold;

        Soul::GLSLMat3f baseColorUvMatrix;
        Soul::GLSLMat3f metallicRoughnessUvMatrix;
        Soul::GLSLMat3f normalUvMatrix;
        Soul::GLSLMat3f occlusionUvMatrix;
        Soul::GLSLMat3f emissiveUvMatrix;
        Soul::GLSLMat3f clearCoatUvMatrix;
        Soul::GLSLMat3f clearCoatRoughnessMatrix;
        Soul::GLSLMat3f clearCoatNormalUvMatrix;
        Soul::GLSLMat3f sheenColorUvMatrix;
        Soul::GLSLMat3f sheenRoughnessUvMatrix;
        Soul::GLSLMat3f transmissionUvMatrix;
    };

    struct BoneUBO {
        Soul::Quaternionf q = { 1.0f, 0.0f, 0.0f, 0.0f };
        Soul::Vec4f t;
        Soul::Vec4f s = { 1.0f, 0.0f, 0.0f, 0.0f };
        Soul::Vec4f ns = { 1.0f, 1.0f, 1.0f, 0.0f };
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

        MaterialTextures() = default;
    };

	struct Material {
        GPUProgramSetID programSetID;
        MaterialUBO buffer;
        MaterialTextures textures;
        AlphaMode alphaMode;
        float alphaCutoff;
	};

    struct TransformComponent {
        Soul::Mat4f local;
        Soul::Mat4f world;
        EntityID parent;
        EntityID firstChild;
        EntityID next;
        EntityID prev;

        void renderUI();
    };

    struct Mesh {
        AABB aabb;
        Soul::Array<Primitive> primitives;
    };

	struct RenderComponent {
		Visibility visibility;
        MeshID meshID;
        SkinID skinID;
        Soul::Vec4f morphWeights;
	};

	struct SpotParams {
		float radius = 0;
		float outerClamped = 0;
		float cosOuterSquared = 1;
		float sinInverse = std::numeric_limits<float>::infinity();
		float luminousPower = 0;
		Soul::Vec2f scaleOffset;
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
        Soul::Vec3f position;
        Soul::Vec3f direction;
        Soul::Vec3f color;
        ShadowParams shadowParams;
        SpotParams spotParams;
        float sunAngularRadius;
        float sunHaloSize;
        float sunHaloFalloff;
        float intensity;
        float falloff;
	};

    struct CameraComponent {
    private:
        Soul::Mat4f projection;                // projection matrix (infinite far)
        Soul::Mat4f projectionForCulling;      // projection matrix (with far plane)
        Soul::Vec2f scaling = { 1.0f, 1.0f };  // additional scaling applied to projection
        Soul::Vec2f shiftCS = { 0.0f, 0.0f };  // additional translation applied to projection

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
        Soul::Mat4f getProjectionMatrix() const;
        Soul::Mat4f getCullingProjectionMatrix() const;
        void setScaling(Vec2f scaling);
        float getNear() const { return near; }
        float getCullingFar() const { return far; }
        float getAperture() const { return aperture; }
        float getShutterSpeed() const { return shutterSpeed; }
        float getSensitivity() const { return sensitivity; }
        float getFocalLength() const { return (SENSOR_SIZE * projection.elem[1][1]) * 0.5f; }
        float getFocusDistance() const { return focusDistance; }
        Soul::Vec2f getScaling() const;
    };

    struct NameComponent {
        Soul::String name;
        void renderUI();
    };

    struct CameraInfo {

        static float ComputeEv100(float aperture, float shutterSpeed, float sensitivity) noexcept {
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

        CameraInfo(const TransformComponent& transform, const CameraComponent& camera) {
            projection = camera.getProjectionMatrix();
            cullingProjection = camera.getCullingProjectionMatrix();
            model = transform.world;
            view = mat4Inverse(transform.world);
            zn = camera.getNear();
            zf = camera.getCullingFar();
            ev100 = ComputeEv100(camera.getAperture(), camera.getShutterSpeed(), camera.getSensitivity());
            f = camera.getFocalLength();
            A = f / camera.getAperture();
            d = std::max(zn, camera.getFocusDistance());
        }

        Soul::Mat4f projection;         // projection matrix for drawing (infinite zfar)
        Soul::Mat4f cullingProjection;  // projection matrix for culling
        Soul::Mat4f model;              // camera model matrix
        Soul::Mat4f view;               // camera view matrix
        float zn{};                     // distance (positive) to the near plane
        float zf{};                     // distance (positive) to the far plane
        float ev100{};                  // exposure
        float f{};                      // focal length [m]
        float A{};                      // f-number or f / aperture diameter [m]
        float d{};                      // focus distance [m]
        Soul::Vec3f worldOffset;
        Soul::Vec3f getPosition() const noexcept { return Soul::Vec3f(model.elem[0][3], model.elem[1][3], model.elem[2][3]); }
        Soul::Vec3f getForwardVector() const noexcept { return unit(Soul::Vec3f(model.elem[0][2], model.elem[1][2], model.elem[2][2]) * -1.0f); }

    };

    struct AnimationSampler {
        Soul::Array<float> times;
        Soul::Array<float> values;
        enum { LINEAR, STEP, CUBIC } interpolation = LINEAR;
    };

    struct AnimationChannel {
        uint32 samplerIdx;
        EntityID entity;
        enum { TRANSLATION, ROTATION, SCALE, WEIGHTS } transformType;
    };

    struct Animation {
        Soul::String name;
        float duration = 0.0f;
        Soul::Array<AnimationSampler> samplers;
        Soul::Array<AnimationChannel> channels;
    };

    struct IBL
    {
        GPU::TextureID reflectionTex;
        GPU::TextureID irradianceTex;
        Vec3f irradianceCoefs[9] = {{65504.0f, 65504.0f, 65504.0f}};
        Mat3f rotation = mat3Identity();
        float intensity = 30000.0f;
    };

	struct Scene final : Demo::Scene {
        
        Scene(Soul::GPU::System* gpuSystem, GPUProgramRegistry* programRegistry) :
            _gpuSystem(gpuSystem), _programRegistry(programRegistry),
            _cameraMan({0.1f, 0.001f, Soul::Vec3f(0.0f, 1.0f, 0.0f)})
        {}
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = delete;
        Scene& operator=(Scene&&) = delete;
        ~Scene() override = default;

        void importFromGLTF(const char* path) override;
        void cleanup() override {}
        void renderPanels() override;
        bool update(const Demo::Input& input) override;
        Vec2ui32 getViewport() override { return _viewport; }
        void setViewport(Vec2ui32 viewport) override { _viewport = viewport; }

        const Soul::Array<Texture>& textures() const { return _textures; }
        const Soul::Array<Mesh>& meshes() const { return _meshes; }
        const Soul::Array<Material>& materials() const { return _materials; }
        const Soul::Array<Skin>& skins() const { return _skins; }
        const IBL getIBL() const { return ibl; }

        CameraInfo getActiveCamera() const {
            auto [transform, camera] = _registry.get<TransformComponent, CameraComponent>(_activeCamera);
            return CameraInfo(transform, camera);
        }

        template <typename T>
        void forEachRenderable(T func) {
            auto view = _registry.view<TransformComponent, RenderComponent>();
            for (auto entity : view) {
                auto [transform, renderComp] = view.get<TransformComponent, RenderComponent>(entity);
                func(transform, renderComp);
            }
        }

    private:

        struct CGLTFNodeKey_ {
            const cgltf_node* node;

            CGLTFNodeKey_(const cgltf_node* node) : node(node) {}
            
            inline bool operator==(const CGLTFNodeKey_& other) const {
                return (memcmp(this, &other, sizeof(CGLTFNodeKey_)) == 0);
            }

            inline bool operator!=(const CGLTFNodeKey_& other) const {
                return (memcmp(this, &other, sizeof(CGLTFNodeKey_)) != 0);
            }

            SOUL_NODISCARD uint64 hash() const {
                return uint64(node);
            }
        };

        void createEntity_(Soul::HashMap<CGLTFNodeKey_, EntityID>* nodeMap, const cgltf_data* asset, const cgltf_node* node, EntityID parent);
        void renderEntityTreeNode_(EntityID entityID);
        void createRendereable_(const cgltf_data* asset, const cgltf_node* node, EntityID entity);
        void createLight_(const cgltf_data* asset, const cgltf_node* node, EntityID entity);
        void createCamera_(const cgltf_data* asset, const cgltf_node* node, EntityID entity);
        void setActiveAnimation_(AnimationID animationID);
        void setActiveCamera_(EntityID camera);
        void updateWorldTransform_(EntityID entityID);
        void updateBones_();

        EntityID _rootEntity = ENTITY_ID_NULL;
        Registry _registry;
        Soul::GPU::System* _gpuSystem;
        GPUProgramRegistry* _programRegistry;

        Soul::Array<Texture> _textures;
        Soul::Array<Material> _materials;
        Soul::Array<Mesh> _meshes;
        Soul::Array<Skin> _skins;
        Soul::Array<Animation> _animations;
        Soul::AABB _boundingBox;
        IBL ibl;

        EntityID _selectedEntity = ENTITY_ID_NULL;
        EntityID _activeCamera = ENTITY_ID_NULL;
        AnimationID _activeAnimation;
        float _animationDelta = 0.0f;
        Array<uint64> _channelCursors;
        bool _resetAnimation = false;
        
        CameraManipulator _cameraMan;

        Vec2ui32 _viewport;
	
    };

}