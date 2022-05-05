#include "memory/allocator.h"
#include "gpu/system.h"
#include "core/util.h"

#include "renderer.h"
#include "gpu_program_registry.h"

#include <string>
#include <algorithm>

namespace soul_fila {
    
    static constexpr const char* SHADER_BRDF_FRAG = "filament::brdf.frag.glsl";
    static constexpr const char* SHADER_COMMON_GETTERS = "filament::common_getters.glsl";
    static constexpr const char* SHADER_COMMON_GRAPHICS = "filament::common_graphics.glsl";
    static constexpr const char* SHADER_COMMON_LIGHTING = "filament::common_lighting.glsl";
    static constexpr const char* SHADER_COMMON_MATERIAL = "filament::common_material.glsl";
    static constexpr const char* SHADER_COMMON_MATH = "filament::common_math.glsl";
    static constexpr const char* SHADER_COMMON_SHADING = "filament::common_shading.glsl";
    static constexpr const char* SHADER_COMMON_SHADOWING = "filament::common_shadowing.glsl";
    static constexpr const char* SHADER_COMMON_TYPE = "filament::common_type.glsl";
    static constexpr const char* SHADER_DEPTH_MAIN_FRAG = "filament::depth_main.frag.glsl";
    static constexpr const char* SHADER_DEPTH_MAIN_VERT = "filament::depth_main.vert.glsl";
    static constexpr const char* SHADER_FOG_FRAG = "filament::fog.frag.glsl";
    static constexpr const char* SHADER_GETTERS_FRAG = "filament::getters.frag.glsl";
    static constexpr const char* SHADER_GETTERS_VERT = "filament::getters.vert.glsl";
    static constexpr const char* SHADER_LIGHT_DIRECTIONAL_FRAG = "filament::light_directional.frag.glsl";
    static constexpr const char* SHADER_LIGHT_INDIRECT_FRAG = "filament::light_indirect.frag.glsl";
    static constexpr const char* SHADER_LIGHT_PUNCTUAL_FRAG = "filament::light_punctual.frag.glsl";
    static constexpr const char* SHADER_MAIN_FRAG = "filament::main.frag.glsl";
    static constexpr const char* SHADER_MAIN_VERT = "filament::main.vert.glsl";
    static constexpr const char* SHADER_MATERIAL_INPUTS_FRAG = "filament::material_inputs.frag.glsl";
    static constexpr const char* SHADER_MATERIAL_INPUTS_VERT = "filament::material_inputs.vert.glsl";
    static constexpr const char* SHADER_SHADING_LIT_FRAG = "filament::shading_lit.frag.glsl";
    static constexpr const char* SHADER_SHADING_MODEL_CLOTH_FRAG = "filament::shading_model_cloth.frag.glsl";
    static constexpr const char* SHADER_SHADING_MODEL_STANDARD_FRAG = "filament::shading_model_standard.frag.glsl";
    static constexpr const char* SHADER_SHADING_MODEL_SUBSURFACE_FRAG = "filament::shading_model_subsurface.frag.glsl";
    static constexpr const char* SHADER_SHADING_PARAMETERS_FRAG = "filament::shading_parameters.frag.glsl";
    static constexpr const char* SHADER_SHADING_UNLIT_FRAG = "filament::shading_unlit.frag.glsl";
    static constexpr const char* SHADER_SHADOWING_FRAG = "filament::shadowing.frag.glsl";

    static constexpr const char* EMPTY_VERTEX_CODE = "void materialVertex(inout MaterialVertexInputs m) {\n}\n";

    static constexpr Demo::ShaderDefine COMMON_DEFINES_[] = {
        Demo::ShaderDefine("TARGET_VULKAN_ENVIRONMENT"),
        Demo::ShaderDefine("FILAMENT_VULKAN_SEMANTICS"),
        Demo::ShaderDefine("FILAMENT_HAS_FEATURE_TEXTURE_GATHER"),
        Demo::ShaderDefine("FILAMENT_QUALITY_LOW", uint64(0)),
        Demo::ShaderDefine("FILAMENT_QUALITY_NORMAL", 1),
        Demo::ShaderDefine("FILAMENT_QUALITY_HIGH",2),
        Demo::ShaderDefine("FILAMENT_QUALITY", "FILAMENT_QUALITY_HIGH"),
        Demo::ShaderDefine("MAX_SHADOW_CASTING_SPOTS", CONFIG_MAX_SHADOW_CASTING_SPOTS),
        Demo::ShaderDefine("VERTEX_DOMAIN_OBJECT")
    };

    [[maybe_unused]] static auto PROPERTY_NAMES = EnumArray<Property, const char*>::build_from_list({
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

    static auto PROPERTY_DEFINES = EnumArray<Property, const char*>::build_from_list({
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


    static bool TestProperty_(PropertyBitSet properties, Property property) {
        return (properties & (1 << uint64(property)));
    }

    static bool IsPropertyNeedTBN_(PropertyBitSet properties) {
        return TestProperty_(properties, Property::ANISOTROPY) ||
            TestProperty_(properties, Property::NORMAL) ||
            TestProperty_(properties, Property::BENT_NORMAL) ||
            TestProperty_(properties, Property::CLEAR_COAT_NORMAL);
    }

    static constexpr Demo::ShaderUniformMember FRAME_UNIFORM_MEMBER[] = {
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
        Demo::ShaderUniformMember("padding0",Demo::ShaderVarType::FLOAT4),
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
        Demo::ShaderUniformMember("iblSH", Demo::ShaderVarType::FLOAT3, Demo::ShaderPrecision::DEFAULT, 9),
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

        Demo::ShaderUniformMember("vsmExponent", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("vsmDepthScale", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("vsmLightBleedReduction", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("vsmReserved0", Demo::ShaderVarType::FLOAT),

        // bring PerViewUib to 2 KiB
        Demo::ShaderUniformMember("padding2", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::DEFAULT, 59)

    };
    static constexpr Demo::ShaderUniform FRAME_UNIFORM = {
        "FrameUniforms",
        "frameUniforms",
        FRAME_UNIFORM_MEMBER,
        sizeof(FRAME_UNIFORM_MEMBER) / sizeof(FRAME_UNIFORM_MEMBER[0]),
        FRAME_UNIFORM_BINDING_POINT.set,
        FRAME_UNIFORM_BINDING_POINT.binding
    };

    static constexpr Demo::ShaderUniformMember OBJECT_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("worldFromModelMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("worldFromModelNormalMatrix", Demo::ShaderVarType::MAT3, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("morphWeights", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH),
        Demo::ShaderUniformMember("skinningEnabled", Demo::ShaderVarType::INT),
        Demo::ShaderUniformMember("morphingEnabled", Demo::ShaderVarType::INT),
        Demo::ShaderUniformMember("screenSpaceContactShadows", Demo::ShaderVarType::UINT),
        Demo::ShaderUniformMember("userData", Demo::ShaderVarType::FLOAT)
    };
    static constexpr Demo::ShaderUniform OBJECT_UNIFORM = {
        "ObjectUniforms",
        "objectUniforms",
        OBJECT_UNIFORM_MEMBER,
        sizeof(OBJECT_UNIFORM_MEMBER) / sizeof(OBJECT_UNIFORM_MEMBER[0]),
        RENDERABLE_UNIFORM_BINDING_POINT.set,
        RENDERABLE_UNIFORM_BINDING_POINT.binding
    };

    static constexpr Demo::ShaderUniformMember LIGHTS_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("lights", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_LIGHT_COUNT)
    };
    static constexpr Demo::ShaderUniform LIGHT_UNIFORM = {
        "LightsUniforms",
        "lightsUniforms",
        LIGHTS_UNIFORM_MEMBER,
        sizeof(LIGHTS_UNIFORM_MEMBER) / sizeof(LIGHTS_UNIFORM_MEMBER[0]),
        LIGHT_UNIFORM_BINDING_POINT.set,
        LIGHT_UNIFORM_BINDING_POINT.binding
    };

    static constexpr Demo::ShaderUniformMember SHADOW_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("spotLightFromWorldMatrix", Demo::ShaderVarType::MAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS),
        Demo::ShaderUniformMember("directionShadowBias", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS)
    };
    static constexpr Demo::ShaderUniform SHADOW_UNIFORM = {
        "ShadowUniforms",
        "shadowUniforms",
        SHADOW_UNIFORM_MEMBER,
        sizeof(SHADOW_UNIFORM_MEMBER) / sizeof(SHADOW_UNIFORM_MEMBER[0]),
        SHADOW_UNIFORM_BINDING_POINT.set,
        SHADOW_UNIFORM_BINDING_POINT.binding
    };

    static constexpr Demo::ShaderUniformMember BONES_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("bones", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM, CONFIG_MAX_BONE_COUNT * 4)
    };
    static constexpr Demo::ShaderUniform BONES_UNIFORM = {
        "BonesUniforms",
        "bonesUniforms",
        BONES_UNIFORM_MEMBER,
        sizeof(BONES_UNIFORM_MEMBER) / sizeof(BONES_UNIFORM_MEMBER[0]),
        RENDERABLE_BONE_UNIFORM_BINDING_POINT.set,
        RENDERABLE_BONE_UNIFORM_BINDING_POINT.binding
    };

    static constexpr Demo::ShaderUniformMember FROXEL_RECORD_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("records", Demo::ShaderVarType::UINT4, Demo::ShaderPrecision::HIGH, 1024)
    };

    static constexpr Demo::ShaderUniform FROXEL_RECORD_UNIFORM = {
        "FroxelRecordUniforms",
        "froxelRecordUniforms",
        FROXEL_RECORD_UNIFORM_MEMBER,
        std::size(FROXEL_RECORD_UNIFORM_MEMBER),
        FROXEL_RECORD_UNIFORM_BINDING_POINT.set,
        FROXEL_RECORD_UNIFORM_BINDING_POINT.binding
    };

    static constexpr Demo::ShaderUniformMember MATERIAL_UNIFORM_MEMBER[] = {
        Demo::ShaderUniformMember("baseColorUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("metallicRoughnessUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("normalUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("occlusionUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("emissiveUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("clearCoatUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("clearCoatRoughnessMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("clearCoatNormalUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("sheenColorUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("sheenRoughnessUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("transmissionUvMatrix", Demo::ShaderVarType::MAT3),
        Demo::ShaderUniformMember("volumeThicknessUvMatrix", Demo::ShaderVarType::MAT3),

        Demo::ShaderUniformMember("baseColorFactor", Demo::ShaderVarType::FLOAT4),
        Demo::ShaderUniformMember("emissiveFactor", Demo::ShaderVarType::FLOAT3),
    	Demo::ShaderUniformMember("pad1", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("specularFactor", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("pad2", Demo::ShaderVarType::FLOAT),
    	Demo::ShaderUniformMember("sheenColorFactor", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("pad3", Demo::ShaderVarType::FLOAT),

        Demo::ShaderUniformMember("volumeAbsorption", Demo::ShaderVarType::FLOAT3),
        Demo::ShaderUniformMember("volumeThicknessFactor", Demo::ShaderVarType::FLOAT),
    	Demo::ShaderUniformMember("pad4", Demo::ShaderVarType::FLOAT4),
        Demo::ShaderUniformMember("pad5", Demo::ShaderVarType::FLOAT4),
        Demo::ShaderUniformMember("pad6", Demo::ShaderVarType::FLOAT4),

        Demo::ShaderUniformMember("roughnessFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("metallicFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("glossinessFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("normalScale", Demo::ShaderVarType::FLOAT),

        Demo::ShaderUniformMember("transmissionFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("sheenRoughnessFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("enableDiagnostics", Demo::ShaderVarType::BOOL),
        Demo::ShaderUniformMember("ior", Demo::ShaderVarType::FLOAT),

        Demo::ShaderUniformMember("aoStrength", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("clearCoatFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("clearCoatRoughnessFactor", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("clearCoatNormalScale", Demo::ShaderVarType::FLOAT),

        Demo::ShaderUniformMember("_maskThreshold", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("_doubleSided", Demo::ShaderVarType::BOOL),
        Demo::ShaderUniformMember("_specularAntiAliasingVariance", Demo::ShaderVarType::FLOAT),
        Demo::ShaderUniformMember("_specularAntiAliasingThreshold", Demo::ShaderVarType::FLOAT)


    };
    static constexpr Demo::ShaderUniform MATERIAL_UNIFORM = {
        "MaterialParams",
        "materialParams",
        MATERIAL_UNIFORM_MEMBER,
        sizeof(MATERIAL_UNIFORM_MEMBER) / sizeof(MATERIAL_UNIFORM_MEMBER[0]),
        MATERIAL_UNIFORM_BINDING_POINT.set,
        MATERIAL_UNIFORM_BINDING_POINT.binding
    };

    static constexpr char* NULL_STRING = nullptr;

    static auto ATTRIBUTE_DEFINES = EnumArray<VertexAttribute, const char*>::build_from_list({
        NULL_STRING,
        "HAS_ATTRIBUTE_TANGENTS",
        "HAS_ATTRIBUTE_COLOR",
        "HAS_ATTRIBUTE_UV0",
        "HAS_ATTRIBUTE_UV1",
        "HAS_ATTRIBUTE_BONE_INDICES",
        "HAS_ATTRIBUTE_BONE_WEIGHTS",
        NULL_STRING,
        "HAS_ATTRIBUTE_CUSTOM0",
        "HAS_ATTRIBUTE_CUSTOM1",
        "HAS_ATTRIBUTE_CUSTOM2",
        "HAS_ATTRIBUTE_CUSTOM3",
        "HAS_ATTRIBUTE_CUSTOM4",
        "HAS_ATTRIBUTE_CUSTOM5",
        "HAS_ATTRIBUTE_CUSTOM6",
        "HAS_ATTRIBUTE_CUSTOM7"
    });

    static auto ATTRIBUTE_LOCATION_DEFINES = EnumArray<VertexAttribute, const char*>::build_from_list({
        "LOCATION_POSITION",
        "LOCATION_TANGENTS",
        "LOCATION_COLOR",
        "LOCATION_UV0",
        "LOCATION_UV1",
        "LOCATION_BONE_INDICES",
        "LOCATION_BONE_WEIGHTS",
        NULL_STRING,
        "LOCATION_CUSTOM0",
        "LOCATION_CUSTOM1",
        "LOCATION_CUSTOM2",
        "LOCATION_CUSTOM3",
        "LOCATION_CUSTOM4",
        "LOCATION_CUSTOM5",
        "LOCATION_CUSTOM6",
        "LOCATION_CUSTOM7"
        });

    static auto SHADING_DEFINES = EnumArray<Shading, const char*>::build_from_list({
        "SHADING_MODEL_UNLIT",
        "SHADING_MODEL_LIT",
        "SHADING_MODEL_SUBSURFACE",
        "SHADING_MODEL_CLOTH",
        "SHADING_MODEL_SPECULAR_GLOSSINESS",
    });


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
            (k1.brdf.metallicRoughness.hasTexture == k2.brdf.metallicRoughness.hasTexture) &&
            (k1.brdf.metallicRoughness.hasTexture == k2.brdf.metallicRoughness.UV) &&
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

    bool operator!=(const GPUProgramKey& k1, const GPUProgramKey& k2){
        return !(k1 == k2);
    }

    struct VariantStagePair{
        using Stage = Demo::ShaderType;
        VariantStagePair(uint8 variant, Stage stage) : variant(variant), stage(stage) {}
        uint8 variant;
        Stage stage;
    };

    static bool TestAttribute_(AttributeBitSet attributes, VertexAttribute vertexAttribute) {
        return attributes & (1 << vertexAttribute);
    };

    static soul::Array<VariantStagePair> GetSurfaceVariants_(uint8 variantFilter, bool isLit, bool shadowMultiplier) {
        soul::Array<VariantStagePair> variants;
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

    static void GenerateCommonDefines_(soul::Array<Demo::ShaderDefine>& defines, GPUProgramVariant variant, const ProgramSetInfo& info) {
        for (auto define : COMMON_DEFINES_) {
            defines.add(define);
        }
        bool litVariants = info.isLit || info.hasShadowMultiplier;
        if (litVariants && variant.hasDirectionalLighting()) defines.add(Demo::ShaderDefine("HAS_DIRECTIONAL_LIGHTING"));
        if (litVariants && variant.hasDynamicLighting()) defines.add(Demo::ShaderDefine("HAS_DYNAMIC_LIGHTING"));
        if (litVariants && variant.hasShadowReceiver()) defines.add(Demo::ShaderDefine("HAS_SHADOWING"));
        if (info.hasShadowMultiplier) defines.add(Demo::ShaderDefine("HAS_SHADOW_MULTIPLIER"));
        if (variant.hasVsm()) defines.add(Demo::ShaderDefine("HAS_VSM"));
        Util::ForEachBit(info.properties, [&defines](uint32 index) { defines.add(Demo::ShaderDefine(PROPERTY_DEFINES[Property(index)])); });
        if (IsPropertyNeedTBN_(info.properties)) defines.add(Demo::ShaderDefine("MATERIAL_NEEDS_TBN"));
        // generate attribute defines
        Util::ForEachBit(info.requiredAttributes,
            [&defines](uint32 index) {
                VertexAttribute attr = VertexAttribute(index);
                if (ATTRIBUTE_DEFINES[attr] != NULL_STRING) defines.add(Demo::ShaderDefine(ATTRIBUTE_DEFINES[attr]));
            });
    }

    static soul::gpu::ShaderID GenerateVertexShader_(const Demo::ShaderGenerator& generator, GPUProgramVariant variant, const ProgramSetInfo& info) {
        SOUL_PROFILE_ZONE();
        bool litVariants = info.isLit || info.hasShadowMultiplier;

        Demo::ShaderDesc desc;
        desc.type = Demo::ShaderType::VERTEX;
        desc.name = "";

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

        // generate common defines
        soul::Array<Demo::ShaderDefine> defines;
        GenerateCommonDefines_(defines, variant, info);
        if (info.flipUV) defines.add(Demo::ShaderDefine("FLIP_UV_ATTRIBUTE"));
        if (variant.hasSkinningOrMorphing()) defines.add(Demo::ShaderDefine("HAS_SKINNING_OR_MORPHING"));
        Util::ForEachBit(info.requiredAttributes,
            [&defines](uint32 index) {
                VertexAttribute attr = VertexAttribute(index);
                if (ATTRIBUTE_LOCATION_DEFINES[attr] != nullptr) defines.add(Demo::ShaderDefine(ATTRIBUTE_LOCATION_DEFINES[attr], index));
            });
        desc.defines = defines.data();
        desc.defineCount = soul::cast<uint8>(defines.size());

        // generate inputs
        static auto inputMap = EnumArray<VertexAttribute, Demo::ShaderInput>::build_from_list({
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
        });
        Util::ForEachBit(attributes, [&desc](uint32 bitIndex) { desc.inputs[bitIndex] = inputMap[VertexAttribute(bitIndex)]; });
        
        // generate outputs
        desc.outputs[4] = Demo::ShaderOutput("vertex_worldPosition", Demo::ShaderVarType::FLOAT3);
        if (TestAttribute_(attributes, VertexAttribute::QTANGENTS)) {
            desc.outputs[5] = Demo::ShaderOutput("vertex_worldNormal", Demo::ShaderVarType::FLOAT3, Demo::ShaderPrecision::MEDIUM);
            if (IsPropertyNeedTBN_(info.properties))
                desc.outputs[6] = Demo::ShaderOutput("vertex_worldTangent", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM);
        }
        desc.outputs[7] = Demo::ShaderOutput("vertex_position", Demo::ShaderVarType::FLOAT4);
        if (TestAttribute_(attributes, VertexAttribute::COLOR)) 
            desc.outputs[9] = Demo::ShaderOutput("vertex_color", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM);
        if (TestAttribute_(attributes, VertexAttribute::UV0) && !TestAttribute_(attributes, VertexAttribute::UV1))
            desc.outputs[10] = Demo::ShaderOutput("vertex_uv01", Demo::ShaderVarType::FLOAT2, Demo::ShaderPrecision::HIGH);
        else if (TestAttribute_(attributes, VertexAttribute::UV1))
            desc.outputs[10] = Demo::ShaderOutput("vertex_uv01", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH);
        if (variant.hasShadowReceiver() && variant.hasDirectionalLighting())
            desc.outputs[11] = Demo::ShaderOutput("vertex_lightSpacePosition", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH);
        if (variant.hasShadowReceiver() && variant.hasDynamicLighting())
            desc.outputs[12] = Demo::ShaderOutput("vertex_spotLightSpacePosition", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS);

        // generate uniforms
        soul::Array<Demo::ShaderUniform> uniforms;
        uniforms.reserve(8);
        uniforms.add(FRAME_UNIFORM);
        uniforms.add(OBJECT_UNIFORM);
        if (variant.hasSkinningOrMorphing()) uniforms.add(BONES_UNIFORM);
        SOUL_ASSERT(0, info.uib.size() < SOUL_UTYPE_MAX(uint8), "");
        uniforms.add(MATERIAL_UNIFORM);
        desc.uniforms = uniforms.data();
        desc.uniformCount = soul::cast<uint8>(uniforms.size());

        // generate samplers
        desc.samplers = info.sib.data();
        desc.samplerCount = soul::cast<uint8>(info.sib.size());

        // generate codes
        soul::Array<const char*> templateCodes;
        templateCodes.reserve(20);
        templateCodes.add(SHADER_COMMON_MATH);
        templateCodes.add(SHADER_COMMON_SHADOWING);
        templateCodes.add(SHADER_COMMON_GETTERS);
        templateCodes.add(SHADER_GETTERS_VERT);
        templateCodes.add(SHADER_MATERIAL_INPUTS_VERT);
        if (variant.isDepthPass() && (info.blendingMode != BlendingMode::MASKED))
            templateCodes.add(SHADER_DEPTH_MAIN_VERT);
        else {
            templateCodes.add(SHADER_MAIN_VERT);
        }
        desc.templateCodes = templateCodes.data();
        desc.templateCodeCount = soul::cast<uint8>(templateCodes.size());

        if (!variant.isDepthPass() || (info.blendingMode == BlendingMode::MASKED)) {
            if (info.materialVertexCode.empty())
                desc.customCode = EMPTY_VERTEX_CODE;
            else
                desc.customCode = info.materialVertexCode.c_str();
        }
        
        return generator.createShader(desc);
    }

    static soul::gpu::ShaderID GenerateFragmentShader_(const Demo::ShaderGenerator& generator, GPUProgramVariant variant, const ProgramSetInfo& info) {
        SOUL_PROFILE_ZONE();
        Demo::ShaderDesc desc;
        desc.type = Demo::ShaderType::FRAGMENT;
        desc.name = "";

        // generate defines
        soul::Array<Demo::ShaderDefine> defines;
        GenerateCommonDefines_(defines, variant, info);
        if (info.specularAntiAliasing && info.isLit) defines.add(Demo::ShaderDefine("GEOMETRIC_SPECULAR_AA"));
        if (info.clearCoatIorChange) defines.add(Demo::ShaderDefine("CLEAR_COAT_IOR_CHANGE"));
        auto specularAO = info.specularAOSet ? info.specularAO : SpecularAmbientOcclusion::SIMPLE;
        defines.add(Demo::ShaderDefine("SPECULAR_AMBIENT_OCCLUSION", uint64(specularAO)));
        if (info.refractionMode != RefractionMode::NONE) {
            defines.add(Demo::ShaderDefine("HAS_REFRACTION"));
            defines.add(Demo::ShaderDefine("REFRACTION_MODE_CUBEMAP", uint64(RefractionMode::CUBEMAP)));
            defines.add(Demo::ShaderDefine("REFRACTION_MODE_SCREEN_SPACE", uint64(RefractionMode::SCREEN_SPACE)));
            switch (info.refractionMode) {
            case RefractionMode::CUBEMAP:
                defines.add(Demo::ShaderDefine("REFRACTION_MODE", "REFRACTION_MODE_CUBEMAP"));
                break;
            case RefractionMode::SCREEN_SPACE:
                defines.add(Demo::ShaderDefine("REFRACTION_MODE", "REFRACTION_MODE_SCREEN_SPACE"));
                break;
            case RefractionMode::NONE:
            case RefractionMode::COUNT:
            default:
                SOUL_NOT_IMPLEMENTED();
                break;
            }
            defines.add(Demo::ShaderDefine("REFRACTION_TYPE_SOLID", uint64(RefractionType::SOLID)));
            defines.add(Demo::ShaderDefine("REFRACTION_TYPE_THIN", uint64(RefractionType::THIN)));
            switch (info.refractionType) {
            case RefractionType::SOLID:
                defines.add(Demo::ShaderDefine("REFRACTION_TYPE", "REFRACTION_TYPE_SOLID"));
                break;
            case RefractionType::THIN:
                defines.add(Demo::ShaderDefine("REFRACTION_TYPE", "REFRACTION_TYPE_THIN"));
                break;
            case RefractionType::COUNT:
            default:
                SOUL_NOT_IMPLEMENTED();
                break;
            }
        }
        bool multiBounceAO = info.multiBounceAOSet ? info.multiBounceAO : true;
        defines.add(Demo::ShaderDefine("MULTI_BOUNCE_AMBIENT_OCCLUSION", multiBounceAO ? 1u : 0u));
        if (variant.hasFog()) defines.add(Demo::ShaderDefine("HAS_FOG"));
        if (info.hasTransparentShadow) defines.add(Demo::ShaderDefine("HAS_TRANSPARENT_SHADOW"));
        if (info.hasDoubleSidedCapability) defines.add(Demo::ShaderDefine("MATERIAL_HAS_DOUBLE_SIDED_CAPABILITY"));
        switch (info.blendingMode) {
        case BlendingMode::OPAQUE:
            defines.add(Demo::ShaderDefine("BLEND_MODE_OPAQUE"));
            break;
        case BlendingMode::TRANSPARENT:
            defines.add(Demo::ShaderDefine("BLEND_MODE_TRANSPARENT"));
            break;
        case BlendingMode::ADD:
            defines.add(Demo::ShaderDefine("BLEND_MODE_ADD"));
            break;
        case BlendingMode::MASKED:
            defines.add(Demo::ShaderDefine("BLEND_MODE_MASKED"));
            break;
        case BlendingMode::FADE:
            defines.add(Demo::ShaderDefine("BLEND_MODE_TRANSPARENT"));
            defines.add(Demo::ShaderDefine("BLEND_MODE_FADE"));
            break;
        case BlendingMode::MULTIPLY:
            defines.add(Demo::ShaderDefine("BLEND_MODE_MULTIPLY"));
            break;
        case BlendingMode::SCREEN:
            defines.add(Demo::ShaderDefine("BLEND_MODE_SCREEN"));
            break;
        case BlendingMode::COUNT:
        default:
            SOUL_NOT_IMPLEMENTED();
            break;
        }
        switch (info.postLightingBlendingMode) {
        case BlendingMode::OPAQUE:
            defines.add(Demo::ShaderDefine("POST_LIGHTING_BLEND_MODE_OPAQUE"));
            break;
        case BlendingMode::TRANSPARENT:
            defines.add(Demo::ShaderDefine("POST_LIGHTING_BLEND_MODE_TRANSPARENT"));
            break;
        case BlendingMode::ADD:
            defines.add(Demo::ShaderDefine("POST_LIGHTNG_BLEND_MODE_ADD"));
            break;
        case BlendingMode::MULTIPLY:
            defines.add(Demo::ShaderDefine("POST_LIGHTING_BLEND_MODE_MULTIPLY"));
            break;
        case BlendingMode::SCREEN:
            defines.add(Demo::ShaderDefine("POST_LIGHTING_BLEND_MODE_SCREEN"));
            break;
        case BlendingMode::MASKED:
        case BlendingMode::FADE:
        case BlendingMode::COUNT:
        default:
            SOUL_NOT_IMPLEMENTED();
            break;
        }
        defines.add(Demo::ShaderDefine(SHADING_DEFINES[info.shading]));
        if (info.hasCustomSurfaceShading) defines.add(Demo::ShaderDefine("MATERIAL_HAS_CUSTOM_SURFACE_SHADING"));
        desc.defines = defines.data();
        desc.defineCount = soul::cast<uint8>(defines.size());

        AttributeBitSet attributes = info.requiredAttributes;

        // defines shader inputs
        desc.inputs[4] = Demo::ShaderInput("vertex_worldPosition", Demo::ShaderVarType::FLOAT3);
        if (TestAttribute_(info.requiredAttributes, VertexAttribute::QTANGENTS)) {
            desc.inputs[5] = Demo::ShaderInput("vertex_worldNormal", Demo::ShaderVarType::FLOAT3, Demo::ShaderPrecision::MEDIUM);
            if (IsPropertyNeedTBN_(info.properties)) {
                desc.inputs[6] = Demo::ShaderInput("vertex_worldTangent", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM);
            }
        }
        desc.inputs[7] = Demo::ShaderInput("vertex_position", Demo::ShaderVarType::FLOAT4);
        if (TestAttribute_(attributes, VertexAttribute::COLOR)) desc.inputs[9] = Demo::ShaderInput("vertex_color", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::MEDIUM);
        if (TestAttribute_(attributes, VertexAttribute::UV0) && !TestAttribute_(attributes, VertexAttribute::UV1))
            desc.inputs[10] = Demo::ShaderInput("vertex_uv01", Demo::ShaderVarType::FLOAT2, Demo::ShaderPrecision::HIGH);
        else if (TestAttribute_(attributes, VertexAttribute::UV1))
            desc.inputs[10] = Demo::ShaderInput("vertex_uv01", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH);
        if (variant.hasShadowReceiver() && variant.hasDirectionalLighting())
            desc.inputs[11] = Demo::ShaderInput("vertex_lightSpacePosition", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH);
        if (variant.hasShadowReceiver() && variant.hasDynamicLighting())
            desc.inputs[12] = Demo::ShaderInput("vertex_spotLightSpacePosition", Demo::ShaderVarType::FLOAT4, Demo::ShaderPrecision::HIGH, CONFIG_MAX_SHADOW_CASTING_SPOTS);

        // generate uniforms
        soul::Array<Demo::ShaderUniform> uniforms;
        uniforms.add(FRAME_UNIFORM);
        uniforms.add(OBJECT_UNIFORM);
        uniforms.add(MATERIAL_UNIFORM);
        if (!variant.isDepthPass())
        {
            uniforms.add(LIGHT_UNIFORM);
            uniforms.add(SHADOW_UNIFORM);
            uniforms.add(FROXEL_RECORD_UNIFORM);
        }
        desc.uniforms = uniforms.data();
        desc.uniformCount = soul::cast<decltype(desc.uniformCount)>(uniforms.size());

        // generate samplers
        Array<Demo::ShaderSampler> samplers = info.sib;
        if (!variant.isDepthPass()) {
            uint8 frameSamplerBinding = FRAME_SAMPLER_START_BINDING;
            if (variant.hasVsm())
                samplers.add(Demo::ShaderSampler("light_shadowMap", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D_ARRAY, Demo::SamplerFormat::FLOAT, Demo::ShaderPrecision::HIGH));
            else
                samplers.add(Demo::ShaderSampler("light_shadowMap", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D_ARRAY, Demo::SamplerFormat::SHADOW, Demo::ShaderPrecision::MEDIUM));
            samplers.add(Demo::ShaderSampler("light_froxels", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D, Demo::SamplerFormat::UINT, Demo::ShaderPrecision::MEDIUM));
            samplers.add(Demo::ShaderSampler("light_iblDFG", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D, Demo::SamplerFormat::FLOAT, Demo::ShaderPrecision::MEDIUM));
            samplers.add(Demo::ShaderSampler("light_iblSpecular", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_CUBEMAP, Demo::SamplerFormat::FLOAT, Demo::ShaderPrecision::MEDIUM));
            samplers.add(Demo::ShaderSampler("light_ssao", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D, Demo::SamplerFormat::FLOAT, Demo::ShaderPrecision::MEDIUM));
            samplers.add(Demo::ShaderSampler("light_ssr", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D, Demo::SamplerFormat::FLOAT, Demo::ShaderPrecision::MEDIUM));
            // ReSharper disable once CppAssignedValueIsNeverUsed
            samplers.add(Demo::ShaderSampler("light_structure", FRAME_SAMPLER_SET, frameSamplerBinding++, Demo::SamplerType::SAMPLER_2D, Demo::SamplerFormat::FLOAT, Demo::ShaderPrecision::MEDIUM));
        }
        desc.samplers = samplers.data();
        desc.samplerCount = soul::cast<decltype(desc.samplerCount)>(samplers.size());
       
        // generate code
        Array<const char*> templateCodes;
        templateCodes.add(SHADER_COMMON_TYPE);
        templateCodes.add(SHADER_COMMON_MATH);
        templateCodes.add(SHADER_COMMON_SHADOWING);
        templateCodes.add(SHADER_COMMON_SHADING);
        templateCodes.add(SHADER_COMMON_GRAPHICS);
        templateCodes.add(SHADER_COMMON_MATERIAL);
        templateCodes.add(SHADER_COMMON_GETTERS);
        templateCodes.add(SHADER_GETTERS_FRAG);
        templateCodes.add(SHADER_MATERIAL_INPUTS_FRAG);
        templateCodes.add(SHADER_SHADING_PARAMETERS_FRAG);
        if (variant.isDepthPass()) {
            if (info.blendingMode == BlendingMode::MASKED) {
                desc.customCode = info.materialCode.c_str();
            }
            templateCodes.add(SHADER_DEPTH_MAIN_FRAG);
        }
        else {
            templateCodes.add(SHADER_FOG_FRAG);
            desc.customCode = info.materialCode.c_str();
            if (info.isLit) {
                templateCodes.add(SHADER_COMMON_LIGHTING);
                if (variant.hasShadowReceiver()) {
                    templateCodes.add(SHADER_SHADOWING_FRAG);
                }
                templateCodes.add(SHADER_BRDF_FRAG);

                switch (info.shading) {
                case Shading::UNLIT:
                    SOUL_NOT_IMPLEMENTED();
                    break;
                case Shading::SPECULAR_GLOSSINESS:
                case Shading::LIT:
                    templateCodes.add(SHADER_SHADING_MODEL_STANDARD_FRAG);
                    break;
                case Shading::SUBSURFACE:
                    templateCodes.add(SHADER_SHADING_MODEL_SUBSURFACE_FRAG);
                    break;
                case Shading::CLOTH:
                    templateCodes.add(SHADER_SHADING_MODEL_CLOTH_FRAG);
                    break;
                case Shading::COUNT:
                default:
                    SOUL_NOT_IMPLEMENTED();
                    break;
                }

                if (info.shading != Shading::UNLIT) {
                    templateCodes.add("filament::ambient_occlusion.frag.glsl");
                    templateCodes.add(SHADER_LIGHT_INDIRECT_FRAG);
                }
                if (variant.hasDirectionalLighting()) {
                    templateCodes.add(SHADER_LIGHT_DIRECTIONAL_FRAG);
                }
                if (variant.hasDynamicLighting()) {
                    templateCodes.add(SHADER_LIGHT_PUNCTUAL_FRAG);
                }

                templateCodes.add(SHADER_SHADING_LIT_FRAG);
            }
            else {
                if (info.hasShadowMultiplier && variant.hasShadowReceiver()) {
                    templateCodes.add(SHADER_SHADOWING_FRAG);
                }
                templateCodes.add(SHADER_SHADING_UNLIT_FRAG);
            }
            templateCodes.add(SHADER_MAIN_FRAG);
        }
        desc.templateCodes = templateCodes.data();
        desc.templateCodeCount = soul::cast<decltype(desc.templateCodeCount)>(templateCodes.size());

        return generator.createShader(desc);
    }

    static bool GenerateProgramSet_(GPUProgramSet& program_set, const Demo::ShaderGenerator& generator, const soul::Array<VariantStagePair>& variants, const ProgramSetInfo& info) {
        program_set.info = info;
    	runtime::TaskID parent = runtime::create_task(runtime::TaskID::ROOT(), [](runtime::TaskID) {});
    	for (const auto& variant_pair : variants) {
            if (variant_pair.stage == Demo::ShaderType::VERTEX) {
                runtime::create_and_run_task(parent, [&](runtime::TaskID) {
                    soul::gpu::ShaderID shader_id = GenerateVertexShader_(generator, GPUProgramVariant(variant_pair.variant), info);
                    program_set.vertShaderIDs[variant_pair.variant] = shader_id;
                });
            }
            else {
                runtime::create_and_run_task(parent, [&](runtime::TaskID) {
                    soul::gpu::ShaderID shader_id = GenerateFragmentShader_(generator, GPUProgramVariant(variant_pair.variant), info);
                    program_set.fragShaderIDs[variant_pair.variant] = shader_id;
                });
            }
        }
        runtime::run_task(parent);
        runtime::wait_task(parent);
        return true;
    }

	static std::string MaterialCodeFromKey_(const GPUProgramKey& programKey, PropertyBitSet& properties) {
        std::string shader = "void material(inout MaterialInputs material) {\n";

        auto _setProperty = [&properties](Property property) {
            properties |= (1 << uint64(property));
        };

        _setProperty(Property::BASE_COLOR);
        if (programKey.hasNormalTexture && !programKey.unlit) {
            _setProperty(Property::NORMAL);
            shader += "highp float2 normalUV = ${normal};\n";
            if (programKey.hasTextureTransforms) {
                shader += "normalUV = (vec3(normalUV, 1.0) * materialParams.normalUvMatrix).xy;\n";
            }
            shader += R"SHADER(
                material.normal = texture(materialParams_normalMap, normalUV).xyz * 2.0 - 1.0;
                material.normal.xy *= materialParams.normalScale;
            )SHADER";
        }

        if (programKey.enableDiagnostics && !programKey.unlit) {
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

        if (programKey.hasBaseColorTexture) {
            shader += "highp float2 baseColorUV = ${color};\n";
            if (programKey.hasTextureTransforms) {
                shader += "baseColorUV = (vec3(baseColorUV, 1.0) * "
                    "materialParams.baseColorUvMatrix).xy;\n";
            }
            shader += R"SHADER(
                material.baseColor *= texture(materialParams_baseColorMap, baseColorUV);
            )SHADER";
        }

        if (programKey.enableDiagnostics) {
            shader += R"SHADER(
               #if defined(HAS_ATTRIBUTE_TANGENTS)
                if (materialParams.enableDiagnostics) {
                    material.baseColor.rgb = vertex_worldNormal * 0.5 + 0.5;
                }
              #endif
            )SHADER";
        }

        if (programKey.alphaMode == AlphaMode::BLEND) {
            shader += R"SHADER(
                material.baseColor.rgb *= material.baseColor.a;
            )SHADER";
        }

        if (programKey.hasVertexColors) {
            shader += "material.baseColor *= getColor();\n";
        }

        if (!programKey.unlit) {
            _setProperty(Property::EMISSIVE);
            if (programKey.useSpecularGlossiness) {
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
            if (programKey.brdf.metallicRoughness.hasTexture) {
                shader += "highp float2 metallicRoughnessUV = ${metallic};\n";
                if (programKey.hasTextureTransforms) {
                    shader += "metallicRoughnessUV = (vec3(metallicRoughnessUV, 1.0) * "
                        "materialParams.metallicRoughnessUvMatrix).xy;\n";
                }
                if (programKey.useSpecularGlossiness) {
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
            if (programKey.hasOcclusionTexture) {
                shader += "highp float2 aoUV = ${ao};\n";
                if (programKey.hasTextureTransforms) {
                    shader += "aoUV = (vec3(aoUV, 1.0) * materialParams.occlusionUvMatrix).xy;\n";
                }
                _setProperty(Property::AMBIENT_OCCLUSION);
                shader += R"SHADER(
                    material.ambientOcclusion = texture(materialParams_occlusionMap, aoUV).r *
                            materialParams.aoStrength;
                )SHADER";
            }
            if (programKey.hasEmissiveTexture) {
                shader += "highp float2 emissiveUV = ${emissive};\n";
                if (programKey.hasTextureTransforms) {
                    shader += "emissiveUV = (vec3(emissiveUV, 1.0) * "
                        "materialParams.emissiveUvMatrix).xy;\n";
                }
                shader += R"SHADER(
                    material.emissive.rgb *= texture(materialParams_emissiveMap, emissiveUV).rgb;
                )SHADER";
            }
            if (programKey.hasTransmission) {
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
                if (programKey.hasTransmissionTexture) {
                    shader += "highp float2 transmissionUV = ${transmission};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "transmissionUV = (vec3(transmissionUV, 1.0) * "
                            "materialParams.transmissionUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.transmission *= texture(materialParams_transmissionMap, transmissionUV).r;
                    )SHADER";
                }
            }
            if (programKey.hasClearCoat) {

                _setProperty(Property::CLEAR_COAT);
                _setProperty(Property::CLEAR_COAT_ROUGHNESS);
                shader += R"SHADER(
                    material.clearCoat = materialParams.clearCoatFactor;
                    material.clearCoatRoughness = materialParams.clearCoatRoughnessFactor;
                )SHADER";

                if (programKey.hasClearCoatNormalTexture) {
                    _setProperty(Property::CLEAR_COAT_NORMAL);
                    shader += "highp float2 clearCoatNormalUV = ${clearCoatNormal};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "clearCoatNormalUV = (vec3(clearCoatNormalUV, 1.0) * "
                            "materialParams.clearCoatNormalUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.clearCoatNormal = texture(materialParams_clearCoatNormalMap, clearCoatNormalUV).xyz * 2.0 - 1.0;
                        material.clearCoatNormal.xy *= materialParams.clearCoatNormalScale;
                    )SHADER";
                }

                if (programKey.hasClearCoatTexture) {
                    shader += "highp float2 clearCoatUV = ${clearCoat};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "clearCoatUV = (vec3(clearCoatUV, 1.0) * "
                            "materialParams.clearCoatUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.clearCoat *= texture(materialParams_clearCoatMap, clearCoatUV).r;
                    )SHADER";
                }

                if (programKey.hasClearCoatRoughnessTexture) {
                    shader += "highp float2 clearCoatRoughnessUV = ${clearCoatRoughness};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "clearCoatRoughnessUV = (vec3(clearCoatRoughnessUV, 1.0) * "
                            "materialParams.clearCoatRoughnessUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.clearCoatRoughness *= texture(materialParams_clearCoatRoughnessMap, clearCoatRoughnessUV).g;
                    )SHADER";
                }
            }

            if (programKey.hasSheen) {
                _setProperty(Property::SHEEN_COLOR);
                _setProperty(Property::SHEEN_ROUGHNESS);
                shader += R"SHADER(
                    material.sheenColor = materialParams.sheenColorFactor;
                    material.sheenRoughness = materialParams.sheenRoughnessFactor;
                )SHADER";

                if (programKey.hasSheenColorTexture) {
                    shader += "highp float2 sheenColorUV = ${sheenColor};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "sheenColorUV = (vec3(sheenColorUV, 1.0) * "
                            "materialParams.sheenColorUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.sheenColor *= texture(materialParams_sheenColorMap, sheenColorUV).rgb;
                    )SHADER";
                }

                if (programKey.hasSheenRoughnessTexture) {
                    shader += "highp float2 sheenRoughnessUV = ${sheenRoughness};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "sheenRoughnessUV = (vec3(sheenRoughnessUV, 1.0) * "
                            "materialParams.sheenRoughnessUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
                        material.sheenRoughness *= texture(materialParams_sheenRoughnessMap, sheenRoughnessUV).a;
                    )SHADER";
                }
            }

            if (programKey.hasVolume)
            {
                _setProperty(Property::ABSORPTION);
                _setProperty(Property::THICKNESS);
                shader += R"SHADER(
	                material.absorption = materialParams.volumeAbsorption;

	                // TODO: Provided by Filament, but this should really be provided/computed by gltfio
	                // TODO: This scale is per renderable and should include the scale of the mesh node
	                float scale = objectUniforms.userData;
	                material.thickness = materialParams.volumeThicknessFactor * scale;
	            )SHADER";

                if (programKey.hasVolumeThicknessTexture) {
                    shader += "highp float2 volumeThicknessUV = ${volumeThickness};\n";
                    if (programKey.hasTextureTransforms) {
                        shader += "volumeThicknessUV = (vec3(volumeThicknessUV, 1.0) * "
                            "materialParams.volumeThicknessUvMatrix).xy;\n";
                    }
                    shader += R"SHADER(
	                    material.thickness *= texture(materialParams_volumeThicknessMap, volumeThicknessUV).g;
	                )SHADER";
                }
            }

            if (programKey.hasIOR)
            {
                _setProperty(Property::IOR);
                shader += R"SHADER(
	                material.ior = materialParams.ior;
	            )SHADER";
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

        static constexpr char const* uvstrings[] = { "vec2(0)", "getUV0()", "getUV1()" };
        const auto& normalUV = uvstrings[programKey.normalUV];
        const auto& baseColorUV = uvstrings[programKey.baseColorUV];
        const auto& metallicRoughnessUV = uvstrings[programKey.brdf.metallicRoughness.UV];
        const auto& emissiveUV = uvstrings[programKey.emissiveUV];
        const auto& transmissionUV = uvstrings[programKey.transmissionUV];
        const auto& aoUV = uvstrings[programKey.aoUV];
        const auto& clearCoatUV = uvstrings[programKey.clearCoatUV];
        const auto& clearCoatRoughnessUV = uvstrings[programKey.clearCoatRoughnessUV];
        const auto& clearCoatNormalUV = uvstrings[programKey.clearCoatNormalUV];
        const auto& sheenColorUV = uvstrings[programKey.sheenColorUV];
        const auto& sheenRoughnessUV = uvstrings[programKey.sheenRoughnessUV];

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

	GPUProgramRegistry::GPUProgramRegistry(soul::memory::Allocator* allocator, soul::gpu::System* gpuSystem) :
		_allocatorInitializer(allocator),
		_gpuSystem(gpuSystem),
		_shaderGenerator(allocator, gpuSystem)
	{
		_allocatorInitializer.end();
        _shaderGenerator.addShaderTemplates("filament", "shaders/filament");
	}

	GPUProgramSetID GPUProgramRegistry::createProgramSet(const GPUProgramKey& config) {
        SOUL_ASSERT_MAIN_THREAD();
        if (_programSetMap.isExist(config)) {
            return _programSetMap[config];
        }

        ProgramSetInfo info;
        info.flipUV = false;
        info.specularAO = SpecularAmbientOcclusion::SIMPLE;
        info.specularAntiAliasing = true;
        info.clearCoatIorChange = false;
        info.materialCode = MaterialCodeFromKey_(config, info.properties);
        info.hasDoubleSidedCapability = true;
        // shading configuration
        if (config.unlit) info.shading = Shading::UNLIT;
        else if (config.useSpecularGlossiness) info.shading = Shading::SPECULAR_GLOSSINESS;
        else info.shading = Shading::LIT;
        info.isLit = (info.shading != Shading::UNLIT);

        soul::Array<Demo::ShaderUniformMember>& matBufferMembers = info.uib;
        soul::Array<Demo::ShaderSampler>& matSamplers = info.sib;
        AttributeBitSet& requiredAttributes = info.requiredAttributes;

        // Compute required attributes
        requiredAttributes |= (1 << VertexAttribute::POSITION);
        auto _getNumUV = [](const GPUProgramKey& key) -> int {
            return std::max({ key.baseColorUV, key.brdf.metallicRoughness.UV, key.normalUV, key.aoUV,
                key.emissiveUV, key.transmissionUV, key.clearCoatUV, key.clearCoatRoughnessUV,
                key.clearCoatNormalUV, key.sheenColorUV, key.sheenRoughnessUV });
        };
        int numUv = _getNumUV(config);
        if (numUv > 0) requiredAttributes |= (1 << VertexAttribute::UV0);
        if (numUv > 1) requiredAttributes |= (1 << VertexAttribute::UV1);
        if (config.hasVertexColors) requiredAttributes |= (1 << VertexAttribute::COLOR);
        if (info.shading != Shading::UNLIT) requiredAttributes |= (1 << VertexAttribute::QTANGENTS);

        auto _addMatBufferMember = [&matBufferMembers](const char* name, Demo::ShaderVarType varType) {
            matBufferMembers.add(Demo::ShaderUniformMember(name, varType));
        };

        uint32 numSampler = 0;
        auto _addMatSampler = [&matSamplers, &numSampler](const char* name, Demo::SamplerType samplerType) {
            matSamplers.add(Demo::ShaderSampler(name, MATERIAL_SAMPLER_SET, soul::cast<uint8>(numSampler), samplerType, Demo::SamplerFormat::FLOAT));
            numSampler++;
        };

        _addMatSampler("materialParams_baseColorMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_metallicRoughnessMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_normalMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_occlusionMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_emissiveMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_clearCoatMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_clearCoatRoughnessMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_clearCoatNormalMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_sheenColorMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_sheenRoughnessMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_transmissionMap", Demo::SamplerType::SAMPLER_2D);
        _addMatSampler("materialParams_volumeThicknessMap", Demo::SamplerType::SAMPLER_2D);

        if (config.enableDiagnostics) {
            _addMatBufferMember("enableDiagnostics", Demo::ShaderVarType::BOOL);
        }

        // BASE COLOR
        _addMatBufferMember("baseColorFactor", Demo::ShaderVarType::FLOAT4);
        if (config.hasBaseColorTexture) {
            if (config.hasTextureTransforms) {
                _addMatBufferMember("baseColorUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }
        
        // METALLIC-ROUGHNESS
        _addMatBufferMember("metallicFactor", Demo::ShaderVarType::FLOAT);
        _addMatBufferMember("roughnessFactor", Demo::ShaderVarType::FLOAT);
        if (config.brdf.metallicRoughness.hasTexture) {
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
            if (config.hasTextureTransforms) {
                _addMatBufferMember("normalUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // AMBIENT OCCLUSION
        // In the glTF spec aoStrength is in occlusionTextureInfo; in cgltf it is part of texture_view.
        _addMatBufferMember("aoStrength", Demo::ShaderVarType::FLOAT);
        if (config.hasOcclusionTexture) {
            if (config.hasTextureTransforms) {
                _addMatBufferMember("occlusionMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // EMISSIVE
        _addMatBufferMember("emissiveFactor", Demo::ShaderVarType::FLOAT3);
        if (config.hasEmissiveTexture) {
            if (config.hasTextureTransforms) {
                _addMatBufferMember("emissiveUvMatrix", Demo::ShaderVarType::MAT3);
            }
        }

        // CLEAR COAT
        if (config.hasClearCoat) {
            _addMatBufferMember("clearCoatFactor", Demo::ShaderVarType::FLOAT);
            _addMatBufferMember("clearCoatRoughnessFactor", Demo::ShaderVarType::FLOAT);
            if (config.hasClearCoatTexture) {
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("clearCoatUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }

            if (config.hasClearCoatRoughnessTexture) {
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("clearCoatRoughnessUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
            if (config.hasClearCoatNormalTexture) {
                _addMatBufferMember("clearCoatNormalScale", Demo::ShaderVarType::FLOAT);
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
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("sheenColorUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
            if (config.hasSheenRoughnessTexture) {
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("sheenRoughnessUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }
        }

        // TRANSMISSION
        if (config.hasTransmission) {

            // According to KHR_materials_transmission, the minimum expectation for a compliant renderer
            // is to at least render any opaque objects that lie behind transmitting objects.
            info.refractionMode = RefractionMode::SCREEN_SPACE;

            // Thin refraction probably makes the most sense, given the language of the transmission
            // spec and its lack of an IOR parameter. This means that we would do a good job rendering a
            // window pane, but a poor job of rendering a glass full of liquid.
            info.refractionType = RefractionType::THIN;

            _addMatBufferMember("transmissionFactor", Demo::ShaderVarType::FLOAT);
            if (config.hasTransmissionTexture) {
                if (config.hasTextureTransforms) {
                    _addMatBufferMember("transmissionUvMatrix", Demo::ShaderVarType::MAT3);
                }
            }

            info.blendingMode = BlendingMode::MASKED;

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
            case AlphaMode::COUNT:
            default:
                SOUL_NOT_IMPLEMENTED();
                break;
            }
        }

        if (config.hasVolume)
        {
            info.refractionMode = RefractionMode::SCREEN_SPACE;
        	info.refractionType = RefractionType::SOLID;
            info.blendingMode = BlendingMode::MASKED;
        }

        if (info.hasDoubleSidedCapability) _addMatBufferMember("_doubleSided", Demo::ShaderVarType::BOOL);
        if (info.specularAntiAliasing) {
            _addMatBufferMember("_specularAntiAliasingVariance", Demo::ShaderVarType::FLOAT);
            _addMatBufferMember("_specularAntiAliasingThreshold", Demo::ShaderVarType::FLOAT);
        }
        
        GPUProgramSetID program_set_id = GPUProgramSetID(soul::cast<GPUProgramSetID::store_type>(_programSets.add(GPUProgramSet())));
        _programSetMap.add(config, program_set_id);

        GenerateProgramSet_(_programSets[program_set_id.id], _shaderGenerator, GetSurfaceVariants_(0, info.shading != Shading::UNLIT, info.hasShadowMultiplier), info);

        return program_set_id;
	}

    soul::gpu::ProgramID GPUProgramRegistry::getProgram(GPUProgramSetID programSetID, GPUProgramVariant variant) {

        GPUProgramSet& program_set = _programSets[programSetID.id];
        gpu::ProgramID programID = program_set.programIDs[variant.key];
        if (programID != soul::gpu::PROGRAM_ID_NULL) {
            return programID;
        }

        uint8 vertexVariant = GPUProgramVariant::filterVariantVertex(variant.key);
        uint8 fragmentVariant = GPUProgramVariant::filterVariantFragment(variant.key);

        soul::gpu::ProgramDesc programDesc;
        programDesc.shaderIDs[soul::gpu::ShaderStage::VERTEX] = program_set.vertShaderIDs[vertexVariant];
        programDesc.shaderIDs[soul::gpu::ShaderStage::FRAGMENT] = program_set.fragShaderIDs[fragmentVariant];
        programID = _gpuSystem->request_program(programDesc);
        program_set.programIDs[variant.key] = programID;
        return programID;
    }

    const ProgramSetInfo& GPUProgramRegistry::getProgramSetInfo(GPUProgramSetID programSetID) const
    {
        return _programSets[programSetID.id].info;
    }


}