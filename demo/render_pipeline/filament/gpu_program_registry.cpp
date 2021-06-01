#include "memory/allocator.h"
#include "gpu/system.h"
#include "core/util.h"

#include "renderer.h"
#include "gpu_program_registry.h"

#include <string>
#include <algorithm>

namespace SoulFila {

    enum class Property : uint8 {
        BASE_COLOR,              //!< float4, all shading models
        ROUGHNESS,               //!< float,  lit shading models only
        METALLIC,                //!< float,  all shading models, except unlit and cloth
        REFLECTANCE,             //!< float,  all shading models, except unlit and cloth
        AMBIENT_OCCLUSION,       //!< float,  lit shading models only, except subsurface and cloth
        CLEAR_COAT,              //!< float,  lit shading models only, except subsurface and cloth
        CLEAR_COAT_ROUGHNESS,    //!< float,  lit shading models only, except subsurface and cloth
        CLEAR_COAT_NORMAL,       //!< float,  lit shading models only, except subsurface and cloth
        ANISOTROPY,              //!< float,  lit shading models only, except subsurface and cloth
        ANISOTROPY_DIRECTION,    //!< float3, lit shading models only, except subsurface and cloth
        THICKNESS,               //!< float,  subsurface shading model only
        SUBSURFACE_POWER,        //!< float,  subsurface shading model only
        SUBSURFACE_COLOR,        //!< float3, subsurface and cloth shading models only
        SHEEN_COLOR,             //!< float3, lit shading models only, except subsurface
        SHEEN_ROUGHNESS,         //!< float3, lit shading models only, except subsurface and cloth
        SPECULAR_COLOR,          //!< float3, specular-glossiness shading model only
        GLOSSINESS,              //!< float,  specular-glossiness shading model only
        EMISSIVE,                //!< float4, all shading models
        NORMAL,                  //!< float3, all shading models only, except unlit
        POST_LIGHTING_COLOR,     //!< float4, all shading models
        CLIP_SPACE_TRANSFORM,    //!< mat4,   vertex shader only
        ABSORPTION,              //!< float3, how much light is absorbed by the material
        TRANSMISSION,            //!< float,  how much light is refracted through the material
        IOR,                     //!< float,  material's index of refraction
        MICRO_THICKNESS,         //!< float, thickness of the thin layer
        BENT_NORMAL,             //!< float3, all shading models only, except unlit
        COUNT,
    };

    constexpr EnumArray<Property, const char*> PROPERTY_NAMES({
        "baseColor",
        "roughness",
        "metallic",
        "reflectance",
        "ambientOcclusion",
        "clearCoat",
        "clearCoatRoughness",
        "clearCoatNormal",
        "anisotropy",
        "anisotropyDirection",
        "thickness",
        "subsurfacePower",
        "subsurfaceColor",
        "sheenColor",
        "sheenRoughness",
        "glossiness",
        "specularColor",
        "emissive",
        "normal",
        "postLightingColor",
        "clipSpaceTransform",
        "absorption",
        "transmission",
        "ior",
        "microThickness",
        "bentNormal"
    });

    constexpr EnumArray<Property, const char*> PROPERTY_DEFINES({
        "MATERIAL_HAS_BASE_COLOR",
        "MATERIAL_HAS_ROUGHNESS",
        "MATERIAL_HAS_METALLIC",
        "MATERIAL_HAS_REFLECTANCE",
        "MATERIAL_HAS_AMBIENT_OCCLUSION",
        "MATERIAL_HAS_CLEAR_COAT",
        "MATERIAL_HAS_CLEAR_COAT_ROUGHNESS",
        "MATERIAL_HAS_CLEAR_COAT_NORMAL",
        "MATERIAL_HAS_ANISOTROPY",
        "MATERIAL_HAS_ANISOTROPY_DIRECTION",
        "MATERIAL_HAS_THICKNESS",
        "MATERIAL_HAS_SUBSURFACE_POWER",
        "MATERIAL_HAS_SUBSURFACE_COLOR",
        "MATERIAL_HAS_SHEEN_COLOR",
        "MATERIAL_HAS_SHEEN_ROUGHNESS",
        "MATERIAL_HAS_GLOSSINESS",
        "MATERIAL_HAS_SPECULAR_COLOR",
        "MATERIAL_HAS_EMISSIVE",
        "MATERIAL_HAS_NORMAL",
        "MATERIAL_HAS_POST_LIGHTING_COLOR",
        "MATERIAL_HAS_CLIP_SPACE_TRANSFORM",
        "MATERIAL_HAS_ABSORPTION",
        "MATERIAL_HAS_TRANSMISSION",
        "MATERIAL_HAS_IOR",
        "MATERIAL_HAS_MICRO_THICKNESS",
        "MATERIAL_HAS_BENT_NORMAL"
    });
    using PropertyBitSet = uint32;

    static bool _TestProperty(PropertyBitSet properties, Property property) {
        return (properties & (1 << uint64(property)));
    }

    static bool _IsPropertyNeedTBN(PropertyBitSet properties) {
        return _TestProperty(properties, Property::ANISOTROPY) ||
            _TestProperty(properties, Property::NORMAL) ||
            _TestProperty(properties, Property::BENT_NORMAL) ||
            _TestProperty(properties, Property::CLEAR_COAT_NORMAL);
    }

    constexpr Demo::ShaderUniformMember FRAME_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("viewFromWorldMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("worldFromViewMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("clipFromViewMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("viewFromClipMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("clipFromWorldMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("worldFromClipMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("lightFromWorldMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH, 4),
        Demo::ShaderUniformMember("cascadeSplits", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH),
        // view
        Demo::ShaderUniformMember("resolution", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH),
        // camera
        Demo::ShaderUniformMember("cameraPosition", Demo::ShaderVarType::FLOAT3, Demo::ShaderPrecision::HIGH),
        // time
        Demo::ShaderUniformMember("time", Demo::ShaderVarType::FLOAT, Demo::ShaderPrecision::HIGH),
        // directional light
        Demo::ShaderUniformMember("lightColorIntensity", Demo::ShaderVarType::FLOAT4),
        Demo::ShaderUniformMember("sun", Demo::ShaderVarType::FLOAT4),
        Demo::ShaderUniformMember("lightPosition", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("padding0",Demo::ShaderVarType::UINT),
        Demo::ShaderUniformMember("lightDirection", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("fParamsX",Demo::ShaderVarType::UINT),
        // shadow
        Demo::ShaderUniformMember("shadowBias",Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("oneOverFroxelDimensionY", Demo::ShaderVarType::FLOAT),
        // froxels
        Demo::ShaderUniformMember("zParams", Demo::ShaderVarType::FLOAT4),
        Demo::ShaderUniformMember("fParams", Demo::ShaderVarType::UINT2),
        Demo::ShaderUniformMember("origin", Demo::ShaderVarType::FLOAT2),
        // froxels (again, for alignment purposes)
        Demo::ShaderUniformMember("oneOverFroxelDimension", Demo::ShaderVarType::FLOAT),
        // ibl
        Demo::ShaderUniformMember("iblLuminance", Demo::ShaderVarType::FLOAT),
        // camera
        Demo::ShaderUniformMember("exposure", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("ev100", Demo::ShaderVarType::FLOAT),
        // ibl
        Demo::ShaderUniformMember("iblSH", Demo::ShaderVarType::FLOAT3),
        // user time
        Demo::ShaderUniformMember("userTime", Demo::ShaderVarType::FLOAT4),
        // ibl max mip level
        Demo::ShaderUniformMember("iblRoughnessOneLevel", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("cameraFar", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("refractionLodOffset", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("directionalShadows", Demo::ShaderVarType::UINT),
        // view
        Demo::ShaderUniformMember("worldOffset", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("ssContactShadowDistance", Demo::ShaderVarType::FLOAT),
        // fog
        Demo::ShaderUniformMember("fogStart", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogMaxOpacity", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogHeight",  Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogHeightFalloff", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogColor", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("fogDensity", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogInscatteringStart", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogInscatteringSize", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("fogColorFromIbl", Demo::ShaderVarType::FLOAT),

        // CSM information
        Demo::ShaderUniformMember("cascades", Demo::ShaderVarType::UINT),

        // SSAO sampling parameters
        Demo::ShaderUniformMember("aoSamplingQualityAndEdgeDistance", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("aoReserved1", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("aoReserved2", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("aoReserved3", Demo::ShaderVarType::FLOAT),

        Demo::ShaderUniformMember("clipControl", Demo::ShaderVarType::FLOAT2),
        Demo::ShaderUniformMember("padding1", Demo::ShaderVarType::FLOAT2),

        // bring PerViewUib to 2 KiB
        Demo::ShaderUniformMember("padding2", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::DEFAULT, 60)

    };
    constexpr Demo::ShaderUniform FRAME_UNIFORM = {
        "FrameUniforms",
        "frameUniforms",
        FRAME_UNIFORM_MEMBER,
        sizeof(FRAME_UNIFORM_MEMBER) / sizeof(FRAME_UNIFORM_MEMBER[0]),
        MATERIAL_UNIFORM_SET,
        BindingPoints::PER_VIEW
    };

    constexpr Demo::ShaderUniformMember OBJECT_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("worldFromModelMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("worldFromModelNormalMatrix", Demo::ShaderVarType::MAT3, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("morphWeights", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("skinningEnabled", Demo::ShaderVarType::INT),
        Demo::ShaderUniformMember("morphingEnabled", Demo::ShaderVarType::INT),
        Demo::ShaderUniformMember("screenSpaceContactShadows", Demo::ShaderVarType::UINT),
        Demo::ShaderUniformMember("padding0", Demo::ShaderVarType::FLOAT)
    };
    constexpr Demo::ShaderUniform OBJECT_UNIFORM = {
        "ObjectUniforms",
        "objectUniforms",
        OBJECT_UNIFORM_MEMBER,
        sizeof(OBJECT_UNIFORM_MEMBER) / sizeof(OBJECT_UNIFORM_MEMBER[0]),
        MATERIAL_UNIFORM_SET,
        BindingPoints::PER_RENDERABLE
    };

    constexpr Demo::ShaderUniformMember LIGHTS_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("lights", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_LIGHT_COUNT)
    };
    constexpr Demo::ShaderUniform LIGHT_UNIFORM = {
        "LightUniforms",
        "lightUniforms",
        LIGHTS_UNIFORM_MEMBER,
        sizeof(LIGHTS_UNIFORM_MEMBER) / sizeof(LIGHTS_UNIFORM_MEMBER[0]),
        MATERIAL_UNIFORM_SET,
        BindingPoints::LIGHTS
    };

    constexpr Demo::ShaderUniformMember SHADOW_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("spotLightFromWorldMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS),
        Demo::ShaderUniformMember("directionShadowBias", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS)
    };
    constexpr Demo::ShaderUniform SHADOW_UNIFORM = {
        "ShaderUniforms",
        "shaderUniforms",
        SHADOW_UNIFORM_MEMBER,
        sizeof(SHADOW_UNIFORM_MEMBER) / sizeof(SHADOW_UNIFORM_MEMBER[0]),
        MATERIAL_UNIFORM_SET,
        BindingPoints::SHADOW
    };

    constexpr Demo::ShaderUniformMember BONES_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("bones", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM, CONFIG_MAX_BONE_COUNT * 4)
    };
    constexpr Demo::ShaderUniform BONES_UNIFORM = {
        "BonesUniforms",
        "bonesUniforms",
        BONES_UNIFORM_MEMBER,
        sizeof(BONES_UNIFORM_MEMBER) / sizeof(BONES_UNIFORM_MEMBER[0]),
        MATERIAL_UNIFORM_SET,
        BindingPoints::PER_RENDERABLE_BONES
    };

    using AttributeBitSet = uint32;
    constexpr EnumArray<VertexAttribute, const char*> ATTRIBUTE_DEFINES({
        nullptr,
        "HAS_ATTRIBUTE_TANGENTS",
        "HAS_ATTRIBUTE_COLOR",
        "HAS_ATTRIBUTE_UV0",
        "HAS_ATTRIBUTE_UV1",
        "HAS_ATTRIBUTE_BONE_INDICES",
        "HAS_ATTRIBUTE_BONE_WEIGHTS",
        nullptr,
        "HAS_ATTRIBUTE_CUSTOM0",
        "HAS_ATTRIBUTE_CUSTOM1",
        "HAS_ATTRIBUTE_CUSTOM2",
        "HAS_ATTRIBUTE_CUSTOM3",
        "HAS_ATTRIBUTE_CUSTOM4",
        "HAS_ATTRIBUTE_CUSTOM5",
        "HAS_ATTRIBUTE_CUSTOM6",
        "HAS_ATTRIBUTE_CUSTOM7"
    });

    constexpr EnumArray<VertexAttribute, const char*> ATTRIBUTE_LOCATION_DEFINES({
        "LOCATION_POSITION",
        "LOCATION_TANGENTS",
        "LOCATION_COLOR",
        "LOCATION_UV0",
        "LOCATION_UV1",
        "LOCATION_BONE_INDICES",
        "LOCATION_BONE_WEIGHTS",
        nullptr,
        "LOCATION_CUSTOM0",
        "LOCATION_CUSTOM1",
        "LOCATION_CUSTOM2",
        "LOCATION_CUSTOM3",
        "LOCATION_CUSTOM4",
        "LOCATION_CUSTOM5",
        "LOCATION_CUSTOM6",
        "LOCATION_CUSTOM7"
    });

    enum class Shading : uint8 {
        UNLIT,                  //!< no lighting applied, emissive possible
        LIT,                    //!< default, standard lighting
        SUBSURFACE,             //!< subsurface lighting model
        CLOTH,                  //!< cloth lighting model
        SPECULAR_GLOSSINESS,    //!< legacy lighting model
        COUNT
    };

    enum class MaterialDomain : uint8 {
        SURFACE = 0, //!< shaders applied to renderables
        POST_PROCESS = 1, //!< shaders applied to rendered buffers
        COUNT
    };

    /**
     * Specular occlusion
     */
    enum class SpecularAmbientOcclusion : uint8 {
        NONE = 0, //!< no specular occlusion
        SIMPLE = 1, //!< simple specular occlusion
        BENT_NORMALS = 2, //!< more accurate specular occlusion, requires bent normals
        COUNT
    };

    /**
     * Refraction
     */
    enum class RefractionMode : uint8 {
        NONE = 0, //!< no refraction
        CUBEMAP = 1, //!< refracted rays go to the ibl cubemap
        SCREEN_SPACE = 2, //!< refracted rays go to screen space
        COUNT
    };

    /**
     * Refraction type
     */
    enum class RefractionType : uint8 {
        SOLID = 0, //!< refraction through solid objects (e.g. a sphere)
        THIN = 1, //!< refraction through thin objects (e.g. window)
        COUNT
    };

    enum class BlendingMode : uint8 {
        //! material is opaque
        OPAQUE,
        //! material is transparent and color is alpha-pre-multiplied, affects diffuse lighting only
        TRANSPARENT,
        //! material is additive (e.g.: hologram)
        ADD,
        //! material is masked (i.e. alpha tested)
        MASKED,
        /**
         * material is transparent and color is alpha-pre-multiplied, affects specular lighting
         * when adding more entries, change the size of FRenderer::CommandKey::blending
         */
        FADE,
        //! material darkens what's behind it
        MULTIPLY,
        //! material brightens what's behind it
        SCREEN,
        COUNT
    };

    struct ProgramSetInfo {
        bool isLit = true;
        bool hasDoubleSidedCapability = false;
        bool hasExternalSamplers = false;
        bool hasShadowMultiplier = false;
        bool specularAntiAliasing = false;
        bool clearCoatIorChange = true;
        bool flipUV = true;
        bool multiBounceAO = false;
        bool multiBounceAOSet = false;
        bool specularAOSet = false;
        SpecularAmbientOcclusion specularAO = SpecularAmbientOcclusion::NONE;
        RefractionMode refractionMode = RefractionMode::NONE;
        RefractionType refractionType = RefractionType::SOLID;
        AttributeBitSet requiredAttributes = 0;
        BlendingMode blendingMode = BlendingMode::OPAQUE;
        BlendingMode postLightingBlendingMode = BlendingMode::TRANSPARENT;
        Shading shading = Shading::LIT;
        Soul::Array<Demo::ShaderUniformMember> uib;
        Soul::Array<Demo::ShaderSampler> sib;
        std::string materialCode;
        std::string materialVertexCode;
        PropertyBitSet properties = 0;
    };

    bool operator==(const GPUProgramKey& k1, const GPUProgramKey& k2) {
        return
            (k1.doubleSided == k2.doubleSided) &&
            (k1.unlit == k2.unlit) &&
            (k1.hasVertexColors == k2.hasVertexColors) &&
            (k1.hasBaseColorTexture == k2.hasBaseColorTexture) &&
            (k1.hasNormalTexture == k2.hasNormalTexture) &&
            (k1.hasOcclusionTexture == k2.hasOcclusionTexture) &&
            (k1.hasEmissiveTexture == k2.hasEmissiveTexture) &&
            (k1.useSpecularGlossiness == k2.useSpecularGlossiness) &&
            (k1.alphaMode == k2.alphaMode) &&
            (k1.hasMetallicRoughnessTexture == k2.hasMetallicRoughnessTexture) &&
            (k1.metallicRoughnessUV == k2.metallicRoughnessUV) &&
            (k1.baseColorUV == k2.baseColorUV) &&
            (k1.hasClearCoatTexture == k2.hasClearCoatTexture) &&
            (k1.clearCoatUV == k2.clearCoatUV) &&
            (k1.hasClearCoatRoughnessTexture == k2.hasClearCoatRoughnessTexture) &&
            (k1.clearCoatRoughnessUV == k2.clearCoatRoughnessUV) &&
            (k1.hasClearCoatNormalTexture == k2.hasClearCoatNormalTexture) &&
            (k1.clearCoatNormalUV == k2.clearCoatNormalUV) &&
            (k1.hasClearCoat == k2.hasClearCoat) &&
            (k1.hasTransmission == k2.hasTransmission) &&
            (k1.hasTextureTransforms == k2.hasTextureTransforms) &&
            (k1.emissiveUV == k2.emissiveUV) &&
            (k1.aoUV == k2.aoUV) &&
            (k1.normalUV == k2.normalUV) &&
            (k1.hasTransmissionTexture == k2.hasTransmissionTexture) &&
            (k1.transmissionUV == k2.transmissionUV) &&
            (k1.hasSheenColorTexture == k2.hasSheenColorTexture) &&
            (k1.sheenColorUV == k2.sheenColorUV) &&
            (k1.hasSheenRoughnessTexture == k2.hasSheenRoughnessTexture) &&
            (k1.sheenRoughnessUV == k2.sheenRoughnessUV) &&
            (k1.hasSheen == k2.hasSheen);
    }

    struct VariantStagePair{
        using Stage = Demo::ShaderType;
        VariantStagePair(uint8 variant, Stage stage) : variant(variant), stage(stage) {}
        uint8 variant;
        Stage stage;
    };

    static Soul::Array<VariantStagePair> _GetSurfaceVariants(uint8 variantFilter, bool isLit, bool shadowMultiplier) {
        Soul::Array<VariantStagePair> variants;
        uint8_t variantMask = ~variantFilter;
        for (uint8_t k = 0; k < VARIANT_COUNT; k++) {
            if (GPUProgramVariant::isReserved(k)) {
                continue;
            }

            // Remove variants for unlit materials
            uint8_t v = GPUProgramVariant::filterVariant(
                k & variantMask, isLit || shadowMultiplier);

            if (GPUProgramVariant::filterVariantVertex(v) == k) {
                variants.add(VariantStagePair(k, Demo::ShaderType::VERTEX));
            }

            if (GPUProgramVariant::filterVariantFragment(v) == k) {
                variants.add(VariantStagePair(k, Demo::ShaderType::FRAGMENT));
            }
        }
        return variants;
    }

    static Soul::GPU::ShaderID _GenerateVertexShader(const Demo::ShaderGenerator& generator, GPUProgramVariant variant, const ProgramSetInfo& info) {
        
        bool litVariants = info.isLit || info.hasShadowMultiplier;

        Demo::ShaderDesc desc;
        desc.type = Demo::ShaderType::VERTEX;

        AttributeBitSet attributes = info.requiredAttributes;
        if (variant.hasSkinningOrMorphing()) {
            attributes |= (1 << VertexAttribute::BONE_INDICES);
            attributes |= (1 << VertexAttribute::BONE_WEIGHTS);
            attributes |= (1 << VertexAttribute::MORPH_POSITION_0);
            attributes |= (1 << VertexAttribute::MORPH_POSITION_1);
            attributes |= (1 << VertexAttribute::MORPH_POSITION_2);
            attributes |= (1 << VertexAttribute::MORPH_POSITION_3);
            attributes |= (1 << VertexAttribute::MORPH_TANGENTS_0);
            attributes |= (1 << VertexAttribute::MORPH_TANGENTS_1);
            attributes |= (1 << VertexAttribute::MORPH_TANGENTS_2);
            attributes |= (1 << VertexAttribute::MORPH_TANGENTS_3);
        }
        static auto _testAttribute = [](AttributeBitSet attributes, VertexAttribute vertexAttribute) -> bool {
            return attributes & (1 << vertexAttribute);
        };

        // generate inputs
        static const Soul::EnumArray<VertexAttribute, Demo::ShaderInput> inputMap(
            {
                Demo::ShaderInput("mesh_position", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_tangents", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_color", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_uv0", Demo::ShaderVarType::FLOAT2),
                Demo::ShaderInput("mesh_uv1", Demo::ShaderVarType::FLOAT2),
                Demo::ShaderInput("mesh_bone_indices", Demo::ShaderVarType::UINT4),
                Demo::ShaderInput("mesh_bone_weights", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput(nullptr, Demo::ShaderVarType::COUNT),
                Demo::ShaderInput("mesh_custom0", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom1", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom2", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom3", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom4", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom5", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom6", Demo::ShaderVarType::FLOAT4),
                Demo::ShaderInput("mesh_custom7", Demo::ShaderVarType::FLOAT4)
            }
        );
        Util::ForEachBit(attributes, [&desc](uint32 bitIndex) { desc.inputs[bitIndex] = inputMap[VertexAttribute(bitIndex)]; });
        
        // generate outputs
        desc.outputs[4] = Demo::ShaderOutput("vertex_worldPosition", Demo::ShaderVarType::FLOAT3);
        if (_testAttribute(attributes, VertexAttribute::TANGENTS)) {
            desc.outputs[5] = Demo::ShaderOutput("vertex_worldNormal", Demo::ShaderVarType::FLOAT3, Demo::ShaderPrecision::MEDIUM);
            if (_IsPropertyNeedTBN(info.properties))
                desc.outputs[6] = Demo::ShaderOutput("vertex_worldTangent", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM);
        }
        desc.outputs[7] = Demo::ShaderOutput("vertex_position", Demo::ShaderVarType::FLOAT4);
        if (_testAttribute(attributes, VertexAttribute::COLOR)) 
            desc.outputs[9] = Demo::ShaderOutput("vertex_color", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM);
        if (_testAttribute(attributes, VertexAttribute::UV0) && !_testAttribute(attributes, VertexAttribute::UV1))
            desc.outputs[10] = Demo::ShaderOutput("vertex_uv01", Demo::ShaderVarType::FLOAT2, Demo::ShaderPrecision::HIGH);
        else if (_testAttribute(attributes, VertexAttribute::UV1))
            desc.outputs[10] = Demo::ShaderOutput("vertex_uv01", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH);
        if (variant.hasShadowReceiver() && variant.hasDirectionalLighting())
            desc.outputs[11] = Demo::ShaderOutput("vertex_lightSpacePosition", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH);
        if (variant.hasShadowReceiver() && variant.hasDynamicLighting())
            desc.outputs[12] = Demo::ShaderOutput("vertex_spotLightSpacePosition", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS);

        // generate uniforms
        Demo::ShaderUniform uniforms[BindingPoints::COUNT];
        uint64 uniformCount = 0;
        uniforms[uniformCount++] = FRAME_UNIFORM;
        uniforms[uniformCount++] = OBJECT_UNIFORM;
        if (litVariants && variant.hasShadowReceiver()) uniforms[uniformCount++] = SHADOW_UNIFORM;
        if (variant.hasSkinningOrMorphing()) uniforms[uniformCount++] = BONES_UNIFORM;
        SOUL_ASSERT(0, info.uib.size() < SOUL_UTYPE_MAX(uint8), "");
        uniforms[uniformCount++] = { "MaterialParams", "materialParams", info.uib.data(), uint8(info.uib.size()), MATERIAL_UNIFORM_SET, BindingPoints::PER_MATERIAL_INSTANCE };
        desc.uniforms = uniforms;
        desc.uniformCount = uniformCount;

        // generate samplers
        desc.samplers = info.sib.data();
        desc.samplerCount = info.sib.size();

        // generate common defines
        Soul::Array<Demo::ShaderDefine> defines;
        defines.reserve(20);
        defines.add(Demo::ShaderDefine("FILAMENT_QUALITY", "FILAMENT_QUALITY_HIGH"));
        defines.add(Demo::ShaderDefine("MAX_SHADOW_CASTING_SPOTS", CONFIG_MAX_SHADOW_CASTING_SPOTS));
        if (info.flipUV) defines.add(Demo::ShaderDefine("FLIP_UV_ATTRIBUTE"));
        if (litVariants && variant.hasDirectionalLighting()) defines.add(Demo::ShaderDefine("HAS_DIRECTIONAL_LIGHTING"));
        if (litVariants && variant.hasDynamicLighting()) defines.add(Demo::ShaderDefine("HAS_DYNAMIC_LIGHTING"));
        if (litVariants && variant.hasShadowReceiver()) defines.add(Demo::ShaderDefine("HAS_SHADOWING"));
        if (info.hasShadowMultiplier) defines.add(Demo::ShaderDefine("HAS_SHADOW_MULTIPLIER"));
        if (variant.hasSkinningOrMorphing()) defines.add(Demo::ShaderDefine("HAS_SKINNING_OR_MORPHING"));
        if (variant.hasVsm()) defines.add(Demo::ShaderDefine("HAS_VSM"));
        if (_IsPropertyNeedTBN(info.properties)) defines.add(Demo::ShaderDefine("MATERIAL_NEEDS_TBN"));
        desc.defines = defines.data();
        desc.defineCount = defines.size();
        
        // generate material property defines
        Util::ForEachBit(info.properties, [&defines](uint32 index) { defines.add(PROPERTY_DEFINES[Property(index)]); });
        
        // generate attribute defines
        Util::ForEachBit(info.requiredAttributes, 
            [&defines](uint32 index) { 
                VertexAttribute attr = VertexAttribute(index);
                if (ATTRIBUTE_DEFINES[attr] != nullptr) defines.add(Demo::ShaderDefine(ATTRIBUTE_DEFINES[attr])); 
                if (ATTRIBUTE_LOCATION_DEFINES[attr] != nullptr) defines.add(Demo::ShaderDefine(ATTRIBUTE_LOCATION_DEFINES[attr], index));
            });
        desc.defines = defines.data();
        desc.defineCount = defines.size();

        // generate templateCodes
        Soul::Array<const char*> templateCodes;
        templateCodes.reserve(20);
        templateCodes.add("filament::common_math.glsl");
        templateCodes.add("filament::common_shadowing.glsl");
        templateCodes.add("filament::common_getters.glsl");
        templateCodes.add("filament::getters.vert.glsl");
        templateCodes.add("filament::material_inputs.vert.glsl");

        if (variant.isDepthPass() && (info.blendingMode != BlendingMode::MASKED))
            templateCodes.add("filament::depth_main.vert.glsl");
        else {
            templateCodes.add("filament::main.vert.glsl");
        }
        desc.templateCodes = templateCodes.data();
        desc.templateCodeCount = templateCodes.size();

        //generate customCodes
        desc.customCode = "void materialVertex(inout MaterialVertexInputs m) {\n}\n";

        desc.type = Demo::ShaderType::VERTEX;
        desc.name = "";

        return generator.createShader(desc);
    }

    static Soul::GPU::ShaderID _GenerateFragmentShader(const Demo::ShaderGenerator& generator, GPUProgramVariant variant, const ProgramSetInfo& info) {
        return Soul::GPU::SHADER_ID_NULL;
    }

    static bool _GenerateProgramSet(GPUProgramSet& programSet, const Demo::ShaderGenerator& generator, const Soul::Array<VariantStagePair>& variants, const ProgramSetInfo& info) {
        for (const auto& variantPair : variants) {
            if (variantPair.stage == Demo::ShaderType::VERTEX) {
                Soul::GPU::ShaderID shaderID = _GenerateVertexShader(generator, GPUProgramVariant(variantPair.variant), info);
                programSet.vertShaderIDs[variantPair.variant] = shaderID;
            }
            else {
                Soul::GPU::ShaderID shaderID = _GenerateFragmentShader(generator, GPUProgramVariant(variantPair.variant), info);
                programSet.fragShaderIDs[variantPair.variant] = shaderID;
            }
        }
        return true;
    }

	static std::string _MaterialCodeFromKey(const GPUProgramKey& config, PropertyBitSet& properties) {
        std::string shader = "void material(inout MaterialInputs material) {\n";

        auto _setProperty = [&properties](Property property) {
            properties |= (1 << uint64(property));
        };

        _setProperty(Property::BASE_COLOR);
        if (config.hasNormalTexture && !config.unlit) {
            _setProperty(Property::NORMAL);
            shader += "highp float2 normalUV = ${normal};\n";
            if (config.hasTextureTransforms) {
                shader += "normalUV = (vec3(normalUV, 1.0) * materialParams.normalUvMatrix).xy;\n";
            }
            shader += R"SHADER(
                material.normal = texture(materialParams_normalMap, normalUV).xyz * 2.0 - 1.0;
                material.normal.xy *= materialParams.normalScale;
            )SHADER";
        }

        if (config.enableDiagnostics && !config.unlit) {
            _setProperty(Property::NORMAL);
            shader += R"SHADER(
                if (materialParams.enableDiagnostics) {
                    material.normal = vec3(0, 0, 1);
                }
            )SHADER";
        }

        shader += R"SHADER(
            prepareMaterial(material);
            material.baseColor = materialParams.baseColorFactor;
        )SHADER";

        if (config.hasBaseColorTexture) {
            shader += "highp float2 baseColorUV = ${color};\n";
            if (config.hasTextureTransforms) {
                shader += "baseColorUV = (vec3(baseColorUV, 1.0) * "
                    "materialParams.baseColorUvMatrix).xy;\n";
            }
            shader += R"SHADER(
                material.baseColor *= texture(materialParams_baseColorMap, baseColorUV);
            )SHADER";
        }

        if (config.enableDiagnostics) {
            shader += R"SHADER(
               #if defined(HAS_ATTRIBUTE_TANGENTS)
                if (materialParams.enableDiagnostics) {
                    material.baseColor.rgb = vertex_worldNormal * 0.5 + 0.5;
                }
              #endif
            )SHADER";
        }

        if (config.alphaMode == AlphaMode::BLEND) {
            shader += R"SHADER(
                material.baseColor.rgb *= material.baseColor.a;
            )SHADER";
        }

        if (config.hasVertexColors) {
            shader += "material.baseColor *= getColor();\n";
        }

        if (!config.unlit) {
            _setProperty(Property::EMISSIVE);
            if (config.useSpecularGlossiness) {
                _setProperty(Property::GLOSSINESS);
                _setProperty(Property::SPECULAR_COLOR);
                shader += R"SHADER(
                    material.glossiness = materialParams.glossinessFactor;
                    material.specularColor = materialParams.specularFactor;
                    material.emissive = vec4(materialParams.emissiveFactor.rgb, 0.0);
                )SHADER";
            }
            else {
                _setProperty(Property::ROUGHNESS);
                _setProperty(Property::METALLIC);
                shader += R"SHADER(
                    material.roughness = materialParams.roughnessFactor;
                    material.metallic = materialParams.metallicFactor;
                    material.emissive = vec4(materialParams.emissiveFactor.rgb, 0.0);
                )SHADER";
            }
            if (config.hasMetallicRoughnessTexture) {
                shader += "highp float2 metallicRoughnessUV = ${metallic};\n";
                if (config.hasTextureTransforms) {
                    shader += "metallicRoughnessUV = (vec3(metallicRoughnessUV, 1.0) * "
                        "materialParams.metallicRoughnessUvMatrix).xy;\n";
                }
                if (config.useSpecularGlossiness) {
                    shader += R"SHADER(
                        vec4 sg = texture(materialParams_metallicRoughnessMap, metallicRoughnessUV);
                        material.specularColor *= sg.rgb;
                        material.glossiness *= sg.a;
                    )SHADER";
                }
                else {
                    _setProperty(Property::ROUGHNESS);
                    _setProperty(Property::METALLIC);
                    shader += R"SHADER(
                        vec4 mr = texture(materialParams_metallicRoughnessMap, metallicRoughnessUV);
                        material.roughness *= mr.g;
                        material.metallic *= mr.b;
                    )SHADER";
                }
            }
            if (config.hasOcclusionTexture) {
                shader += "highp float2 aoUV = ${ao};\n";
                if (config.hasTextureTransforms) {
                    shader += "aoUV = (vec3(aoUV, 1.0) * materialParams.occlusionUvMatrix).xy;\n";
                }
                _setProperty(Property::AMBIENT_OCCLUSION);
                shader += R"SHADER(
                    material.ambientOcclusion = texture(materialParams_occlusionMap, aoUV).r *
                            materialParams.aoStrength;
                )SHADER";
            }
            if (config.hasEmissiveTexture) {
                shader += "highp float2 emissiveUV = ${emissive};\n";
                if (config.hasTextureTransforms) {
                    shader += "emissiveUV = (vec3(emissiveUV, 1.0) * "
                        "materialParams.emissiveUvMatrix).xy;\n";
                }
                shader += R"SHADER(
                    material.emissive.rgb *= texture(materialParams_emissiveMap, emissiveUV).rgb;
                )SHADER";
            }
            if (config.hasTransmission) {
                _setProperty(Property::ABSORPTION);
                _setProperty(Property::TRANSMISSION);
                shader += R"SHADER(
                    material.transmission = materialParams.transmissionFactor;

                    // KHR_materials_transmission stipulates that baseColor be used for absorption, and
                    // it says "the transmitted light will be modulated by this color as it passes",
                    // which is inverted from Filament's notion of absorption.  Note that Filament
                    // clamps this value to [0,1].
                    material.absorption = 1.0 - material.baseColor.rgb;

                )SHADER";
                if (config.hasTransmissionTexture) {
                    shader += "highp float2 transmissionUV = ${transmission};\n";
                    if (config.hasTextureTransforms) {
                        shader += "transmissionUV = (vec3(transmissionUV, 1.0) * "
                            "materialParams.transmissionUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.transmission *= texture(materialParams_transmissionMap, transmissionUV).r;
                    )SHADER";
                }
            }
            if (config.hasClearCoat) {

                _setProperty(Property::CLEAR_COAT);
                _setProperty(Property::CLEAR_COAT_ROUGHNESS);
                shader += R"SHADER(
                    material.clearCoat = materialParams.clearCoatFactor;
                    material.clearCoatRoughness = materialParams.clearCoatRoughnessFactor;
                )SHADER";

                if (config.hasClearCoatNormalTexture) {
                    _setProperty(Property::CLEAR_COAT_NORMAL);
                    shader += "highp float2 clearCoatNormalUV = ${clearCoatNormal};\n";
                    if (config.hasTextureTransforms) {
                        shader += "clearCoatNormalUV = (vec3(clearCoatNormalUV, 1.0) * "
                            "materialParams.clearCoatNormalUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.clearCoatNormal = texture(materialParams_clearCoatNormalMap, clearCoatNormalUV).xyz * 2.0 - 1.0;
                        material.clearCoatNormal.xy *= materialParams.clearCoatNormalScale;
                    )SHADER";
                }

                if (config.hasClearCoatTexture) {
                    shader += "highp float2 clearCoatUV = ${clearCoat};\n";
                    if (config.hasTextureTransforms) {
                        shader += "clearCoatUV = (vec3(clearCoatUV, 1.0) * "
                            "materialParams.clearCoatUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.clearCoat *= texture(materialParams_clearCoatMap, clearCoatUV).r;
                    )SHADER";
                }

                if (config.hasClearCoatRoughnessTexture) {
                    shader += "highp float2 clearCoatRoughnessUV = ${clearCoatRoughness};\n";
                    if (config.hasTextureTransforms) {
                        shader += "clearCoatRoughnessUV = (vec3(clearCoatRoughnessUV, 1.0) * "
                            "materialParams.clearCoatRoughnessUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.clearCoatRoughness *= texture(materialParams_clearCoatRoughnessMap, clearCoatRoughnessUV).g;
                    )SHADER";
                }
            }

            if (config.hasSheen) {
                _setProperty(Property::SHEEN_COLOR);
                _setProperty(Property::SHEEN_ROUGHNESS);
                shader += R"SHADER(
                    material.sheenColor = materialParams.sheenColorFactor;
                    material.sheenRoughness = materialParams.sheenRoughnessFactor;
                )SHADER";

                if (config.hasSheenColorTexture) {
                    shader += "highp float2 sheenColorUV = ${sheenColor};\n";
                    if (config.hasTextureTransforms) {
                        shader += "sheenColorUV = (vec3(sheenColorUV, 1.0) * "
                            "materialParams.sheenColorUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.sheenColor *= texture(materialParams_sheenColorMap, sheenColorUV).rgb;
                    )SHADER";
                }

                if (config.hasSheenRoughnessTexture) {
                    shader += "highp float2 sheenRoughnessUV = ${sheenRoughness};\n";
                    if (config.hasTextureTransforms) {
                        shader += "sheenRoughnessUV = (vec3(sheenRoughnessUV, 1.0) * "
                            "materialParams.sheenRoughnessUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.sheenRoughness *= texture(materialParams_sheenRoughnessMap, sheenRoughnessUV).a;
                    )SHADER";
                }
            }
        }

        shader += "}\n";

        std::string* shaderPtr = &shader;
        auto replaceAll = [shaderPtr](const std::string& from, const std::string& to) {
            size_t pos = shaderPtr->find(from);
            for (; pos != std::string::npos; pos = shaderPtr->find(from, pos)) {
                shaderPtr->replace(pos, from.length(), to);
            }
        };

        static const std::string uvstrings[] = { "vec2(0)", "getUV0()", "getUV1()" };
        const auto& normalUV = uvstrings[config.normalUV];
        const auto& baseColorUV = uvstrings[config.baseColorUV];
        const auto& metallicRoughnessUV = uvstrings[config.metallicRoughnessUV];
        const auto& emissiveUV = uvstrings[config.emissiveUV];
        const auto& transmissionUV = uvstrings[config.transmissionUV];
        const auto& aoUV = uvstrings[config.aoUV];
        const auto& clearCoatUV = uvstrings[config.clearCoatUV];
        const auto& clearCoatRoughnessUV = uvstrings[config.clearCoatRoughnessUV];
        const auto& clearCoatNormalUV = uvstrings[config.clearCoatNormalUV];
        const auto& sheenColorUV = uvstrings[config.sheenColorUV];
        const auto& sheenRoughnessUV = uvstrings[config.sheenRoughnessUV];

        replaceAll("${normal}", normalUV);
        replaceAll("${color}", baseColorUV);
        replaceAll("${metallic}", metallicRoughnessUV);
        replaceAll("${ao}", aoUV);
        replaceAll("${emissive}", emissiveUV);
        replaceAll("${transmission}", transmissionUV);
        replaceAll("${clearCoat}", clearCoatUV);
        replaceAll("${clearCoatRoughness}", clearCoatRoughnessUV);
        replaceAll("${clearCoatNormal}", clearCoatNormalUV);
        replaceAll("${sheenColor}", sheenColorUV);
        replaceAll("${sheenRoughness}", sheenRoughnessUV);

        return shader;
	}

	GPUProgramRegistry::GPUProgramRegistry(Soul::Memory::Allocator* allocator, Soul::GPU::System* gpuSystem) :
		_allocator(allocator),
		_allocatorInitializer(allocator),
		_gpuSystem(gpuSystem),
		_shaderGenerator(allocator, gpuSystem)
	{
		_allocatorInitializer.end();
        _shaderGenerator.addShaderTemplates("filament", "shaders/filament");
	}

	GPUProgramSetID GPUProgramRegistry::createProgramSet(const GPUProgramKey& config) {
        ProgramSetInfo info;
        info.flipUV = false;
        info.specularAO = SpecularAmbientOcclusion::SIMPLE;
        info.specularAntiAliasing = true;
        info.clearCoatIorChange = false;
        info.materialCode = _MaterialCodeFromKey(config, info.properties);
        info.hasDoubleSidedCapability = true;
        Soul::Array<Demo::ShaderUniformMember>& matBufferMembers = info.uib;
        Soul::Array<Demo::ShaderSampler>& matSamplers = info.sib;
        AttributeBitSet& requiredAttributes = info.requiredAttributes;

        // Compute shading type
        if (config.unlit) info.shading = Shading::UNLIT;
        else if (config.useSpecularGlossiness) info.shading = Shading::SPECULAR_GLOSSINESS;
        else info.shading = Shading::LIT;

        // Compute required attributes
        requiredAttributes |= (1 << VertexAttribute::POSITION);
        auto _getNumUV = [](const GPUProgramKey& key) -> int {
            return std::max({ key.baseColorUV, key.metallicRoughnessUV, key.normalUV, key.aoUV,
                key.emissiveUV, key.transmissionUV, key.clearCoatUV, key.clearCoatRoughnessUV,
                key.clearCoatNormalUV, key.sheenColorUV, key.sheenRoughnessUV });
        };
        int numUv = _getNumUV(config);
        if (numUv > 0) requiredAttributes |= (1 << VertexAttribute::UV0);
        if (numUv > 1) requiredAttributes |= (1 << VertexAttribute::UV1);
        if (config.hasVertexColors) requiredAttributes |= (1 << VertexAttribute::COLOR);
        if (info.shading != Shading::UNLIT) requiredAttributes |= (1 << VertexAttribute::TANGENTS);

        auto _addMatBufferMember = [&matBufferMembers](const char* name, Demo::ShaderVarType varType) {
            matBufferMembers.add(Demo::ShaderUniformMember(name, varType));
        };

        uint32 numSampler = 0;
        auto _addMatSampler = [&matSamplers, &numSampler](const char* name, Demo::SamplerType samplerType) {
            matSamplers.add(Demo::ShaderSampler(name, MATERIAL_SAMPLER_SET, numSampler, samplerType, Demo::SamplerFormat::FLOAT));
            numSampler++;
        };


        if (config.enableDiagnostics) {
            _addMatBufferMember("enableDiagnostics", Demo::ShaderVarType::BOOL);
        }

        // BASE COLOR
        _addMatBufferMember("baseColorFactor", Demo::ShaderVarType::FLOAT4);
        if (config.hasBaseColorTexture) {
            _addMatSampler("materialParams_baseColorMap", Demo::SamplerType::SAMPLER_2D);
            if (config.hasTextureTransforms) {
                _addMatBufferMember("baseColorUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }
        
        // METALLIC-ROUGHNESS
        _addMatBufferMember("metallicFactor", Demo::ShaderVarType::FLOAT);
        _addMatBufferMember("roughnessFactor", Demo::ShaderVarType::FLOAT);
        if (config.hasMetallicRoughnessTexture) {
            _addMatSampler("materialParams_metallicRoughnessMap", Demo::SamplerType::SAMPLER_2D);
            if (config.hasTextureTransforms) {
                _addMatBufferMember("metallicRoughnessUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // SPECULAR-GLOSSINESS
        if (config.useSpecularGlossiness) {
            _addMatBufferMember("glossinessFactor", Demo::ShaderVarType::FLOAT);
            _addMatBufferMember("specularFactor", Demo::ShaderVarType::FLOAT3);
        }

        // NORMAL MAP
        // In the glTF spec normalScale is in normalTextureInfo; in cgltf it is part of texture_view.
        _addMatBufferMember("normalScale", Demo::ShaderVarType::FLOAT);
        if (config.hasNormalTexture) {
            _addMatSampler("materialParams_normalMap", Demo::SamplerType::SAMPLER_2D);
            if (config.hasTextureTransforms) {
                _addMatBufferMember("normalUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // AMBIENT OCCLUSION
        // In the glTF spec aoStrength is in occlusionTextureInfo; in cgltf it is part of texture_view.
        _addMatBufferMember("aoStrength", Demo::ShaderVarType::FLOAT);
        if (config.hasOcclusionTexture) {
            _addMatSampler("materialPrams_occlusionMap", Demo::SamplerType::SAMPLER_2D);
            if (config.hasTextureTransforms) {
                _addMatBufferMember("occlusionMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // EMISSIVE
        _addMatBufferMember("emissiveFactor", Demo::ShaderVarType::FLOAT3);
        if (config.hasEmissiveTexture) {
            _addMatSampler("materialParams_emissiveMap", Demo::SamplerType::SAMPLER_2D);
            if (config.hasTextureTransforms) {
                _addMatBufferMember("emissiveUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // CLEAR COAT
        if (config.hasClearCoat) {
            _addMatBufferMember("clearCoatFactor", Demo::ShaderVarType::FLOAT);
            _addMatBufferMember("clearCoatRoughnessFactor", Demo::ShaderVarType::FLOAT);
            if (config.hasClearCoatTexture) {
                _addMatSampler("materialParams_clearCoatMap", Demo::SamplerType::SAMPLER_2D);
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("clearCoatUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }

            if (config.hasClearCoatRoughnessTexture) {
                _addMatSampler("materialParams_clearCoatRoughnessMap", Demo::SamplerType::SAMPLER_2D);
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("clearCoatRoughnessUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
            if (config.hasClearCoatNormalTexture) {
                _addMatBufferMember("clearCoatNormalScale", Demo::ShaderVarType::FLOAT);
                _addMatSampler("materialParams_clearCoatNormalMap", Demo::SamplerType::SAMPLER_2D);
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("clearCoatNormalUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
        }

        // SHEEN
        if (config.hasSheen) {
            _addMatBufferMember("sheenColorFactor", Demo::ShaderVarType::FLOAT3);
            _addMatBufferMember("sheenRoughnessFactor", Demo::ShaderVarType::FLOAT);
            if (config.hasSheenColorTexture) {
                _addMatSampler("materialParams_sheenColorMap", Demo::SamplerType::SAMPLER_2D);
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("sheenColorUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
            if (config.hasSheenRoughnessTexture) {
                _addMatSampler("materialParams_sheenRoughnessMap", Demo::SamplerType::SAMPLER_2D);
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("sheenRoughnessUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
        }

        // TRANSMISSION
        if (config.hasTransmission) {

            // According to KHR_materials_transmission, the minimum expectation for a compliant renderer
            // is to at least render any opaque objects that lie behind transmitting objects.
            info.refractionMode = RefractionMode::CUBEMAP;

            // Thin refraction probably makes the most sense, given the language of the transmission
            // spec and its lack of an IOR parameter. This means that we would do a good job rendering a
            // window pane, but a poor job of rendering a glass full of liquid.
            info.refractionType = RefractionType::THIN;


            _addMatBufferMember("transmissionFactor", Demo::ShaderVarType::FLOAT);
            if (config.hasTransmissionTexture) {
                _addMatSampler("materialParams_transmissionMap", Demo::SamplerType::SAMPLER_2D);
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("transmissionUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }

            info.blendingMode = BlendingMode::FADE;

        }
        else {

            // BLENDING
            switch (config.alphaMode) {
            case AlphaMode::OPAQUE:
                info.blendingMode = BlendingMode::OPAQUE;
                break;
            case AlphaMode::MASK:
                info.blendingMode = BlendingMode::MASKED;
                _addMatBufferMember("_maskThreshold", Demo::ShaderVarType::FLOAT);
                break;
            case AlphaMode::BLEND:
                info.blendingMode = BlendingMode::FADE;
                break;
            default:
                // Ignore
                break;
            }
        }

        if (info.hasDoubleSidedCapability) _addMatBufferMember("_doubleSided", Demo::ShaderVarType::FLOAT);
        if (info.specularAntiAliasing) {
            _addMatBufferMember("_specularAntiAliasingVariance", Demo::ShaderVarType::FLOAT);
            _addMatBufferMember("_specularAntiAliasingThreshold", Demo::ShaderVarType::FLOAT);
        }
        
        GPUProgramSetID programSetID = GPUProgramSetID(_programSets.add(GPUProgramSet()));
        _programSetMap.add(config, programSetID);

        _GenerateProgramSet(_programSets[programSetID.id], _shaderGenerator, _GetSurfaceVariants(0, info.shading != Shading::UNLIT, info.hasShadowMultiplier), info);

        return programSetID;
	}

}