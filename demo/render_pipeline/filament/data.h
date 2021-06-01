#pragma once

#include "core/type.h"
#include "core/math.h"
#include "gpu/gpu.h"
#include "../../data.h"
#include "../../entt.hpp"

struct cgltf_data;
struct cgltf_node;

namespace SoulFila {

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

	struct Armature;
	using ArmatureID = ID<Armature, PoolID>;

	struct Primitive;
	using PrimitiveID = ID<Primitive, PoolID>;

    struct Mesh;
    using MeshID = ID<Mesh, uint64>;

    struct Skin;
    using SkinID = ID<Skin, uint64>;
    static constexpr const SkinID SKIN_ID_NULL = SkinID(SOUL_UTYPE_MAX(SkinID::id));

    struct Texture;
    using TextureID = ID<Texture, uint64>;
    static constexpr const TextureID TEXTURE_ID_NULL = TextureID(SOUL_UTYPE_MAX(TextureID::id));

    struct Material;
    using MaterialID = ID<Texture, uint64>;
    static constexpr const MaterialID MATERIAL_ID_NULL = MaterialID(SOUL_UTYPE_MAX(MaterialID::id));

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
		uint32 offset = 0;                    //!< attribute offset in bytes
		uint8 stride = 0;                     //!< attribute stride in bytes
		uint8 buffer = 0xFF;                  //!< attribute buffer index
		Soul::GPU::ElementType type = Soul::GPU::ElementType::BYTE;   //!< attribute element type
	};



	struct Primitive {
		Soul::GPU::BufferID vertexBuffers[Soul::GPU::MAX_VERTEX_BINDING];
		Soul::GPU::BufferID indexBuffer = Soul::GPU::BUFFER_ID_NULL;
        Soul::GPU::Topology topology = Soul::GPU::Topology::TRIANGLE_LIST;
		Attribute attributes[VertexAttribute::COUNT];
        MaterialID materialID;
		uint32 vertexCount = 0;
		uint8 vertexBindingCount = 0;
		uint32 activeAttribute = 0;
	};

	struct Armature {
		Soul::Array<EntityID> joints;
		Soul::Array<EntityID> targets;
	};

	struct Texture {
		Soul::GPU::TextureID gpuHandle;
	};

	struct AABB {
		Soul::Vec3f center;
		Soul::Vec3f halfExtent;
	};

	struct Visibility {
		uint8_t priority : 3;
		bool castShadows : 1;
		bool receiveShadows : 1;
		bool culling : 1;
		bool skinning : 1;
		bool morphing : 1;
		bool screenSpaceContactShadows : 1;
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

    enum class AlphaMode : uint8 {
        OPAQUE,
        MASK,
        BLEND
    };

    struct TextureView {
        TextureID textureID;
        Soul::Mat3 transform;
        uint8 texCoord;
    };

	struct Material {
        MaterialFlags flags;

        struct {
            TextureView baseColorTexture;
            TextureView metallicRoughnessTexture;

            Soul::Vec4f baseColorFactor;
            float metallicFactor;
            float roughnessFactor;
        } metallicRoughness;

        struct {
            TextureView diffuseTexture;
            TextureView specularGlossinessTexture;

            Soul::Vec4f diffuseFactor;
            Soul::Vec3f specularFactor;
            float glossinessFactor;
        } specularGlossiness;

        struct {
            TextureView texture;
            TextureView roughnessTexture;
            TextureView normalTexture;

            float factor;
            float roughnessFactor;
        } clearcoat;
        
        float ior;

        struct {
            TextureView texture;
            TextureView colorTexture;
            Soul::Vec3f colorFactor;
            float factor;
        } specular;

        struct {
            TextureView colorTexture;
            TextureView roughnessTexture;
            Soul::Vec3f colorFactor;
            float roughnessFactor;
        } sheen;

        struct {
            TextureView texture;
            float factor;
        } transmission;

        struct {
            TextureView thicknessTexture;
            Soul::Vec3f attenuationColor;
            float attenuationDistance;
            float thicknessFactor;
        } volume;

        TextureView normalTexture;
        TextureView occlusionTexture;
        TextureView emmisiveTexture;
        Soul::Vec3f emmisiveFactor;
        AlphaMode alphaMode;
        float alphaCutoff;
	};

    struct TransformComponent {
        Soul::Mat4 local;
        Soul::Mat4 world;
        EntityID parent;
        EntityID firstChild;
        EntityID next;
        EntityID prev;
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

    struct Skin {

    };

	struct SpotParams {
		float radius = 0;
		float outerClamped = 0;
		float cosOuterSquared = 1;
		float sinInverse = std::numeric_limits<float>::infinity();
		float luminousPower = 0;
		Soul::Vec2f scaleOffset = {};
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
        float maxShadowDistance = 0.3;

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
    };

    struct LightType {
        LightRadiationType type : 3;
        bool shadowCaster : 1;
        bool lightCaster : 1;
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
        Soul::Mat4 projection;                // projection matrix (infinite far)
        Soul::Mat4 projectionForCulling;      // projection matrix (with far plane)
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
        void setOrthoProjection(double left, double right, double bottom, double top, double zNear, double zFar);
        void setPerspectiveProjection(double fovRadian, double aspect, double near, double far);
        Soul::Mat4 getProjectionMatrix();
        Soul::Mat4 getCullingProjectionMatrix();
        void setScaling(Vec2f scaling);
        Soul::Vec2f getScaling();
    };

	struct Scene : Demo::Scene {
        
        Scene(Soul::GPU::System* gpuSystem);

        virtual void importFromGLTF(const char* path);
        virtual void cleanup() { SOUL_NOT_IMPLEMENTED(); }
        virtual bool handleInput() { SOUL_NOT_IMPLEMENTED(); }
        virtual Vec2ui32 getViewport() { SOUL_NOT_IMPLEMENTED(); }
        virtual void setViewport(Vec2ui32 viewport) { SOUL_NOT_IMPLEMENTED(); }

    private:
        
        void _createEntity(const cgltf_data* asset, const cgltf_node* node, EntityID parent);
        void _createRenderable(const cgltf_data* asset, const cgltf_node* node, EntityID entity);
        void _createLight(const cgltf_data* asset, const cgltf_node* node, EntityID entity);
        void _createCamera(const cgltf_data* asset, const cgltf_node* node, EntityID entity);

        EntityID _rootEntity;
        Registry _registry;
        Soul::GPU::System* _gpuSystem;

        Soul::Array<Soul::GPU::TextureID> _textures;
        Soul::Array<Soul::GPU::BufferID> _buffers;
        Soul::Array<Material> _materials;
        Soul::Array<Mesh> _meshes;
        Soul::Array<Skin> _skins;
	};

}