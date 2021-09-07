#pragma once

#include "core/type.h"
#include "core/math.h"
#include "core/hash_map.h"

#include "memory/allocator.h"

#include "../../shader_generator.h"

namespace SoulFila {

    static constexpr size_t VARIANT_COUNT = 128;

    struct GPUProgramSet;
    using GPUProgramSetID = Soul::ID<GPUProgramSet, uint32>;

    enum class AlphaMode : uint8 {
        OPAQUE,
        MASK,
        BLEND,
        COUNT
    };

    // from filament MaterialKey
	struct GPUProgramKey {
        bool doubleSided : 1;
        bool unlit : 1;
        bool hasVertexColors : 1;
        bool hasBaseColorTexture : 1;
        bool hasNormalTexture : 1;
        bool hasOcclusionTexture : 1;
        bool hasEmissiveTexture : 1;
        bool useSpecularGlossiness : 1;
        AlphaMode alphaMode : 4;
        bool enableDiagnostics : 4;
        union BRDF{
            struct MetallicRoughness {
                bool hasTexture : 1;
                uint8 UV : 7;
                MetallicRoughness() : hasTexture(false), UV(0){}
            } metallicRoughness;
            struct {
                bool hasTexture : 1;
                uint8 UV : 7;
            } specularGlossiness;
            BRDF(): metallicRoughness(){}
        } brdf;
        uint8 baseColorUV;
        // -- 32 bit boundary --
        bool hasClearCoatTexture : 1;
        uint8 clearCoatUV : 7;
        bool hasClearCoatRoughnessTexture : 1;
        uint8 clearCoatRoughnessUV : 7;
        bool hasClearCoatNormalTexture : 1;
        uint8 clearCoatNormalUV : 7;
        bool hasClearCoat : 1;
        bool hasTransmission : 1;
        bool hasTextureTransforms : 6;
        // -- 32 bit boundary --
        uint8 emissiveUV;
        uint8 aoUV;
        uint8 normalUV;
        bool hasTransmissionTexture : 1;
        uint8 transmissionUV : 7;

		// -- 32 bit boundary --
        bool hasSheenColorTexture : 1;
        uint8 sheenColorUV : 7;
        bool hasSheenRoughnessTexture : 1;
        uint8 sheenRoughnessUV : 7;
        bool hasSheen;
        
        SOUL_NODISCARD uint64 hash() const {
            return Soul::hashFNV1((const uint8*) this, sizeof(GPUProgramKey));
        }

        GPUProgramKey() :
            doubleSided(false), unlit(false), hasVertexColors(false),
            hasBaseColorTexture(false), hasNormalTexture(false), hasOcclusionTexture(false), hasEmissiveTexture(false),
            useSpecularGlossiness(false),
			alphaMode(AlphaMode::COUNT), enableDiagnostics(false),
			baseColorUV(0),
			hasClearCoatTexture(false), clearCoatUV(0), hasClearCoatRoughnessTexture(false), clearCoatRoughnessUV(0),hasClearCoatNormalTexture(false),clearCoatNormalUV(0),hasClearCoat(false),
			hasTransmission(false), hasTextureTransforms(false), emissiveUV(0), aoUV(0) ,normalUV(0), hasTransmissionTexture(false), transmissionUV(0),
			hasSheenColorTexture(false), sheenColorUV(0), hasSheenRoughnessTexture(false), sheenRoughnessUV(0), hasSheen(false)
        {}

        GPUProgramKey(const GPUProgramKey&) = default;
	};
    bool operator==(const GPUProgramKey& k1, const GPUProgramKey& k2);
    bool operator!=(const GPUProgramKey& k1, const GPUProgramKey& k2);

    // from filament::Variant
    struct GPUProgramVariant {
        GPUProgramVariant() noexcept = default;
        constexpr explicit GPUProgramVariant(uint8 key) noexcept : key(key) { }


        // DIR: Directional Lighting
        // DYN: Dynamic Lighting
        // SRE: Shadow Receiver
        // SKN: Skinning
        // DEP: Depth only
        // FOG: Fog
        // VSM: Variance shadow maps
        //
        //   X: either 1 or 0
        //
        //                    ...-----+-----+-----+-----+-----+-----+-----+-----+
        // Variant                 0  | VSM | FOG | DEP | SKN | SRE | DYN | DIR |
        //                    ...-----+-----+-----+-----+-----+-----+-----+-----+
        // Reserved variants:
        //       Vertex depth            X     0     1     X     0     0     0
        //     Fragment depth            X     0     1     0     0     0     0
        //           Reserved            X     X     1     X     X     X     X
        //           Reserved            X     X     0     X     1     0     0
        //           Reserved            1     X     0     X     0     X     X
        //
        // Standard variants:
        //      Vertex shader            0     0     0     X     X     X     X
        //    Fragment shader            X     X     0     0     X     X     X

        uint8 key = 0;

        // when adding more bits, update FRenderer::CommandKey::draw::materialVariant as needed
        // when adding more bits, update VARIANT_COUNT
        static constexpr uint8 DIRECTIONAL_LIGHTING = 0x01; // directional light present, per frame/world position
        static constexpr uint8 DYNAMIC_LIGHTING = 0x02; // point, spot or area present, per frame/world position
        static constexpr uint8 SHADOW_RECEIVER = 0x04; // receives shadows, per renderable
        static constexpr uint8 SKINNING_OR_MORPHING = 0x08; // GPU skinning and/or morphing
        static constexpr uint8 DEPTH = 0x10; // depth only variants
        static constexpr uint8 FOG = 0x20; // fog
        static constexpr uint8 VSM = 0x40; // variance shadow maps

        static constexpr uint8 VERTEX_MASK = DIRECTIONAL_LIGHTING |
            DYNAMIC_LIGHTING |
            SHADOW_RECEIVER |
            SKINNING_OR_MORPHING |
            DEPTH;

        static constexpr uint8 FRAGMENT_MASK = DIRECTIONAL_LIGHTING |
            DYNAMIC_LIGHTING |
            SHADOW_RECEIVER |
            FOG |
            DEPTH |
            VSM;

        static constexpr uint8 DEPTH_MASK = DIRECTIONAL_LIGHTING |
            DYNAMIC_LIGHTING |
            SHADOW_RECEIVER |
            DEPTH |
            FOG;

        // the depth variant deactivates all variants that make no sense when writing the depth
        // only -- essentially, all fragment-only variants.
        static constexpr uint8 DEPTH_VARIANT = DEPTH;

        // this mask filters out the lighting variants
        static constexpr uint8 UNLIT_MASK = SKINNING_OR_MORPHING | FOG;

        inline bool hasSkinningOrMorphing() const noexcept { return key & SKINNING_OR_MORPHING; }
        inline bool hasDirectionalLighting() const noexcept { return key & DIRECTIONAL_LIGHTING; }
        inline bool hasDynamicLighting() const noexcept { return key & DYNAMIC_LIGHTING; }
        inline bool hasShadowReceiver() const noexcept { return key & SHADOW_RECEIVER; }
        inline bool hasFog() const noexcept { return key & FOG; }
        inline bool hasVsm() const noexcept { return key & VSM; }

        inline void setSkinning(bool v) noexcept { set(v, SKINNING_OR_MORPHING); }
        inline void setDirectionalLighting(bool v) noexcept { set(v, DIRECTIONAL_LIGHTING); }
        inline void setDynamicLighting(bool v) noexcept { set(v, DYNAMIC_LIGHTING); }
        inline void setShadowReceiver(bool v) noexcept { set(v, SHADOW_RECEIVER); }
        inline void setFog(bool v) noexcept { set(v, FOG); }
        inline void setVsm(bool v) noexcept { set(v, VSM); }

        inline constexpr bool isDepthPass() const noexcept {
            return isValidDepthVariant(key);
        }

        inline static constexpr bool isValidDepthVariant(uint8 variantKey) noexcept {
            // For a variant to be a valid depth variant, all of the bits in DEPTH_MASK must be 0,
            // except for DEPTH.
            return (variantKey & DEPTH_MASK) == DEPTH_VARIANT;
        }

        static constexpr bool isReserved(uint8 variantKey) noexcept {
            // reserved variants that should just be skipped
            // 1. If the DEPTH bit is set, then it must be a valid depth variant. Otherwise, the
            // variant is reserved.
            // 2. If SRE is set, either DYN or DIR must also be set (it makes no sense to have
            // shadows without lights).
            // 3. If VSM is set, then SRE must be set.
            return
                ((variantKey & DEPTH) && !isValidDepthVariant(variantKey)) ||
                (variantKey & 0b0010111u) == 0b0000100u ||
                (variantKey & 0b1010100u) == 0b1000000u;
        }

        static constexpr uint8 filterVariantVertex(uint8 variantKey) noexcept {
            // filter out vertex variants that are not needed. For e.g. fog doesn't affect the
            // vertex shader.
            if (variantKey & DEPTH) {
                // VSM affects the vertex shader, but only for DEPTH variants.
                return variantKey & (VERTEX_MASK | VSM);
            }
            return variantKey & VERTEX_MASK;
        }

        static constexpr uint8 filterVariantFragment(uint8 variantKey) noexcept {
            // filter out fragment variants that are not needed. For e.g. skinning doesn't
            // affect the fragment shader.
            return variantKey & FRAGMENT_MASK;
        }

        static constexpr uint8 filterVariant(uint8 variantKey, bool isLit) noexcept {
            // special case for depth variant
            if (isValidDepthVariant(variantKey)) {
                return variantKey;
            }
            // when the shading mode is unlit, remove all the lighting variants
            return isLit ? variantKey : (variantKey & UNLIT_MASK);
        }

    private:
        inline void set(bool v, uint8 mask) noexcept {
            key = (key & ~mask) | (v ? mask : uint8(0));
        }
    };

    struct GPUProgramSet {
        Soul::GPU::ProgramID programIDs[VARIANT_COUNT];
        Soul::GPU::ShaderID vertShaderIDs[VARIANT_COUNT];
        Soul::GPU::ShaderID fragShaderIDs[VARIANT_COUNT];

        GPUProgramSet() {
            for (uint64 varIdx = 0; varIdx < VARIANT_COUNT; varIdx++) {
                programIDs[varIdx] = Soul::GPU::PROGRAM_ID_NULL;
                vertShaderIDs[varIdx] = Soul::GPU::SHADER_ID_NULL;
                fragShaderIDs[varIdx] = Soul::GPU::SHADER_ID_NULL;
            }
        }
    };

	class GPUProgramRegistry {

	public:

        GPUProgramRegistry(Soul::Memory::Allocator* allocator, Soul::GPU::System* gpuSystem);
        GPUProgramSetID createProgramSet(const GPUProgramKey& key);
        Soul::GPU::ProgramID getProgram(GPUProgramSetID programSetID, GPUProgramVariant variant);

        GPUProgramRegistry(const GPUProgramRegistry&) = delete;
        GPUProgramRegistry(GPUProgramRegistry&&) = delete;
        GPUProgramRegistry& operator=(const GPUProgramRegistry&) = delete;
        GPUProgramRegistry& operator=(GPUProgramRegistry&&) = delete;
        ~GPUProgramRegistry() = default;
	
    private:
        Soul::Runtime::AllocatorInitializer _allocatorInitializer;
		Soul::GPU::System* _gpuSystem;
		Demo::ShaderGenerator _shaderGenerator;
        Soul::HashMap<GPUProgramKey, GPUProgramSetID> _programSetMap;
        Soul::Array<GPUProgramSet> _programSets;
	};
}