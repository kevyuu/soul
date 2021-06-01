#include "data.h"
#include "core/math.h"
#include "core/geometry.h"
#include "core/enum_array.h"
#include "memory/allocators/scope_allocator.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "stb_image.h"

const uint32 GLTF_URI_MAX_LENGTH = 1000;

static Soul::Mat3 _MatrixFromUVTransform(Vec2f offset, float rotation, Vec2f scale) {
    float tx = offset.x;
    float ty = offset.y;
    float sx = scale.x;
    float sy = scale.y;
    float c = cos(rotation);
    float s = sin(rotation);
    Soul::Mat3 matTransform;
    matTransform.elem[0][0] = sx * c;
    matTransform.elem[0][1] = -sy * s;
    matTransform.elem[0][2] = 0.0f;
    matTransform.elem[1][0] = sx * s;
    matTransform.elem[1][1] = sy * c;
    matTransform.elem[1][2] = 0.0f;
    matTransform.elem[2][0] = tx;
    matTransform.elem[2][1] = ty;
    matTransform.elem[2][2] = 1.0f;
    return matTransform;
}

static Soul::Vec4f _GetVec4f(const cgltf_float val[4]) {
    return Soul::Vec4f(val[0], val[1], val[2], val[3]);
}

static Soul::Vec3f _GetVec3f(const cgltf_float val[3]) {
    return Soul::Vec3f(val[0], val[1], val[2]);
}

static Soul::Vec2f _GetVec2f(const cgltf_float val[2]) {
    return Soul::Vec2f(val[0], val[1]);
}

static inline bool _GetVertexAttrType (cgltf_attribute_type atype, SoulFila::VertexAttribute* attrType, uint32 index) {
    using namespace SoulFila;
    switch (atype) {
    case cgltf_attribute_type_position:
        *attrType = VertexAttribute::POSITION;
        return true;
    case cgltf_attribute_type_texcoord:
        if (index == 0) {
            *attrType = VertexAttribute::UV0;
        }
        else if (index == 1) {
            *attrType = VertexAttribute::UV1;
        }
        else {
            SOUL_NOT_IMPLEMENTED();
        }

        return true;
    case cgltf_attribute_type_color:
        *attrType = VertexAttribute::COLOR;
        return true;
    case cgltf_attribute_type_joints:
        *attrType = VertexAttribute::BONE_INDICES;
        return true;
    case cgltf_attribute_type_weights:
        *attrType = VertexAttribute::BONE_WEIGHTS;
        return true;
    case cgltf_attribute_type_normal:
    case cgltf_attribute_type_tangent:
    default:
        return false;
    }
};

static inline bool _GetTopology(cgltf_primitive_type in,
    Soul::GPU::Topology* out) {
    using namespace Soul::GPU;
    switch (in) {
    case cgltf_primitive_type_points:
        *out = Topology::POINT_LIST;
        return true;
    case cgltf_primitive_type_lines:
        *out = Topology::LINE_LIST;
        return true;
    case cgltf_primitive_type_triangles:
        *out = Topology::TRIANGLE_LIST;
        return true;
    case cgltf_primitive_type_line_loop:
    case cgltf_primitive_type_line_strip:
    case cgltf_primitive_type_triangle_strip:
    case cgltf_primitive_type_triangle_fan:
        return false;
    }
    return false;
}

static SoulFila::LightRadiationType _GetLightType(const cgltf_light_type light) {
    using namespace SoulFila;
    switch (light) {
    case cgltf_light_type_invalid:
    case cgltf_light_type_directional:
        return LightRadiationType::DIRECTIONAL;
    case cgltf_light_type_point:
        return LightRadiationType::POINT;
    case cgltf_light_type_spot:
        return LightRadiationType::FOCUSED_SPOT;
    }
}

static SoulFila::AlphaMode _GetAlphaMode(const cgltf_alpha_mode alphaMode) {
    switch (alphaMode) {
    case cgltf_alpha_mode_opaque:
        return SoulFila::AlphaMode::OPAQUE;
    case cgltf_alpha_mode_mask:
        return SoulFila::AlphaMode::MASK;
    case cgltf_alpha_mode_blend:
        return SoulFila::AlphaMode::BLEND;
    }

    SOUL_NOT_IMPLEMENTED();
}

/*
// The internal workhorse function of the Transcoder, which takes arbitrary input but always
// produced packed floats. We expose a more readable interface than this to users, who often have
// untyped blobs of interleaved data. Note that this variant takes an arbitrary number of
// components, we also have a fixed-size variant for better compiler output.
template<typename SOURCE_TYPE, int NORMALIZATION_FACTOR>
void Convert(float* target, void const*source, uint64 count,
    int componentCount, int srcStride) noexcept {
    constexpr float scale = 1.0f / float(NORMALIZATION_FACTOR);
    uint8_t const* srcBytes = (uint8_t const*)source;
    for (uint64 i = 0; i < count; ++i, target += componentCount, srcBytes += srcStride) {
        SOURCE_TYPE const* src = (SOURCE_TYPE const*)srcBytes;
        for (int n = 0; n < componentCount; ++n) {
            target[n] = float(src[n]) * scale;
        }
    }
}

template<typename SOURCE_TYPE, int NORMALIZATION_FACTOR, int NUM_COMPONENTS>
void Convert(float* target, void const* source, uint64 count,
    int srcStride) noexcept {
    constexpr float scale = 1.0f / float(NORMALIZATION_FACTOR);
    uint8_t const* srcBytes = (uint8_t const*)source;
    for (uint64 i = 0; i < count; ++i, target += NUM_COMPONENTS, srcBytes += srcStride) {
        SOURCE_TYPE const* src = (SOURCE_TYPE const*)srcBytes;
        for (int n = 0; n < NUM_COMPONENTS; ++n) {
            target[n] = float(src[n]) * scale;
        }
    }
}

// Similar to "convert" but clamps the result to -1, which is required for normalized signed types.
// For example, -128 can be represented in SBYTE but is outside the permitted range and should
// therefore be clamped. For more information, see the Vulkan spec under the section "Conversion
// from Normalized Fixed-Point to Floating-Point".
template<typename SOURCE_TYPE, int NORMALIZATION_FACTOR>
void ConvertClamped(float*target, void const*source, uint64 count,
    int componentCount, int srcStride) noexcept {
    constexpr float scale = 1.0f / float(NORMALIZATION_FACTOR);
    uint8_t const* srcBytes = (uint8_t const*)source;
    for (uint64 i = 0; i < count; ++i, target += componentCount, srcBytes += srcStride) {
        SOURCE_TYPE const* src = (SOURCE_TYPE const*)srcBytes;
        for (int n = 0; n < componentCount; ++n) {
            const float value = float(src[n]) * scale;
            target[n] = value < -1.0f ? -1.0f : value;
        }
    }
}

template<typename SOURCE_TYPE, int NORMALIZATION_FACTOR, int NUM_COMPONENTS>
static void ConvertClamped(float* target, void const* source, uint64 count,
    int srcStride) noexcept {
    constexpr float scale = 1.0f / float(NORMALIZATION_FACTOR);
    uint8 const* srcBytes = (uint8 const*)source;
    for (uint64 i = 0; i < count; ++i, target += NUM_COMPONENTS, srcBytes += srcStride) {
        SOURCE_TYPE const* src = (SOURCE_TYPE const*)srcBytes;
        for (int n = 0; n < NUM_COMPONENTS; ++n) {
            const float value = float(src[n]) * scale;
            target[n] = value < -1.0f ? -1.0f : value;
        }
    }
}*/

enum class ComponentType {
    BYTE,   //!< If normalization is enabled, this maps from [-127,127] to [-1,+1]
    UBYTE,  //!< If normalization is enabled, this maps from [0,255] to [0, +1]
    SHORT,  //!< If normalization is enabled, this maps from [-32767,32767] to [-1,+1]
    USHORT, //!< If normalization is enabled, this maps from [0,65535] to [0, +1]
    HALF,   //!< 1 sign bit, 5 exponent bits, and 5 mantissa bits.
};

/*static uint64 Transcode(float* target, void const* source, uint64 count, 
    ComponentType componentType, bool normalized, uint32 componentCount, uint32 inputStrideBytes) {
    const uint64 required = count * componentCount * sizeof(float);
    if (target == nullptr) {
        return required;
    }
    const uint32_t comp = componentCount;
    switch (componentType) {
    case ComponentType::BYTE: {
        const uint32_t stride = inputStrideBytes ? inputStrideBytes : comp;
        if (normalized) {
            if (comp == 2) {
                ConvertClamped<int8_t, 127, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                ConvertClamped<int8_t, 127, 3>(target, source, count, stride);
            }
            else {
                ConvertClamped<int8_t, 127>(target, source, count, comp, stride);
            }
        }
        else {
            if (comp == 2) {
                Convert<int8_t, 1, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                Convert<int8_t, 1, 3>(target, source, count, stride);
            }
            else {
                Convert<int8_t, 1>(target, source, count, comp, stride);
            }
        }
        return required;
    }
    case ComponentType::UBYTE: {
        const uint32_t stride = inputStrideBytes ? inputStrideBytes : comp;
        if (normalized) {
            if (comp == 2) {
                Convert<uint8_t, 255, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                Convert<uint8_t, 255, 3>(target, source, count, stride);
            }
            else {
                Convert<uint8_t, 255>(target, source, count, comp, stride);
            }
        }
        else {
            if (comp == 2) {
                Convert<uint8_t, 1, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                Convert<uint8_t, 1, 3>(target, source, count, stride);
            }
            else {
                Convert<uint8_t, 1>(target, source, count, comp, stride);
            }
        }
        return required;
    }
    case ComponentType::SHORT: {
        const uint32_t stride = inputStrideBytes ? inputStrideBytes : (2 * comp);
        if (normalized) {
            if (comp == 2) {
                ConvertClamped<int16_t, 32767, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                ConvertClamped<int16_t, 32767, 3>(target, source, count, stride);
            }
            else {
                ConvertClamped<int16_t, 32767>(target, source, count, comp, stride);
            }
        }
        else {
            if (comp == 2) {
                Convert<int16_t, 1, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                Convert<int16_t, 1, 3>(target, source, count, stride);
            }
            else {
                Convert<int16_t, 1>(target, source, count, comp, stride);
            }
        }
        return required;
    }
    case ComponentType::USHORT: {
        const uint32_t stride = inputStrideBytes ? inputStrideBytes : (2 * comp);
        if (normalized) {
            if (comp == 2) {
                Convert<uint16_t, 65535, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                Convert<uint16_t, 65535, 3>(target, source, count, stride);
            }
            else {
                Convert<uint16_t, 65535>(target, source, count, comp, stride);
            }
        }
        else {
            if (comp == 2) {
                Convert<uint16_t, 1, 2>(target, source, count, stride);
            }
            else if (comp == 3) {
                Convert<uint16_t, 1, 3>(target, source, count, stride);
            }
            else {
                Convert<uint16_t, 1>(target, source, count, comp, stride);
            }
        }
        return required;
    }
    case ComponentType::HALF: {
        SOUL_NOT_IMPLEMENTED();
        return required;
    }
    }
    return 0;
} */

// Sometimes a glTF bufferview includes unused data at the end (e.g. in skinning.gltf) so we need to
// compute the correct size of the vertex buffer. Filament automatically infers the size of
// driver-level vertex buffers from the attribute data (stride, count, offset) and clients are
// expected to avoid uploading data blobs that exceed this size. Since this information doesn't
// exist in the glTF we need to compute it manually. This is a bit of a cheat, cgltf_calc_size is
// private but its implementation file is available in this cpp file.
uint32_t _ComputeBindingSize(const cgltf_accessor& accessor) {
    cgltf_size element_size = cgltf_calc_size(accessor.type, accessor.component_type);
    return uint32_t(accessor.stride * (accessor.count - 1) + element_size);
}

uint32 _ComputeBindingOffset(const cgltf_accessor& accessor) {
    return uint32_t(accessor.offset + accessor.buffer_view->offset);
}

static ComponentType _GetComponentType(const cgltf_accessor& accessor) {
    switch (accessor.component_type) {
    case cgltf_component_type_r_8: return ComponentType::BYTE;
    case cgltf_component_type_r_8u: return ComponentType::UBYTE;
    case cgltf_component_type_r_16: return ComponentType::SHORT;
    case cgltf_component_type_r_16u: return ComponentType::USHORT;
    default:
        // This should be unreachable because other types do not require conversion.
        SOUL_NOT_IMPLEMENTED();
        return {};
    }
}

/*static void ConvertToFloats(float* dest, const cgltf_accessor& accessor) {
    const uint32_t dim = cgltf_num_components(accessor.type);
    const uint64 floatsSize = accessor.count * sizeof(float) * dim;
    auto bufferData = (const uint8*)accessor.buffer_view->buffer->data;
    const uint8* source = bufferData + ComputeBindingOffset(accessor);
    Transcode(dest, source, accessor.count, GetComponentType(accessor), bool(accessor.normalized), dim, uint32(accessor.stride));
} */

// This converts a cgltf component type into a Filament Attribute type.
//
// This function has two out parameters. One result is a safe "permitted type" which we know is
// universally accepted across GPU's and backends, but may require conversion (see Transcoder). The
// other result is the "actual type" which requires no conversion.
//
// Returns false if the given component type is invalid.
static bool _GetElementType(cgltf_type type, cgltf_component_type ctype,
    Soul::GPU::ElementType* permitType,
    Soul::GPU::ElementType* actualType) {
    using namespace Soul::GPU;
    switch (type) {
    case cgltf_type_scalar:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = ElementType::BYTE;
            *actualType = ElementType::BYTE;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = ElementType::UBYTE;
            *actualType = ElementType::UBYTE;
            return true;
        case cgltf_component_type_r_16:
            *permitType = ElementType::SHORT;
            *actualType = ElementType::SHORT;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = ElementType::USHORT;
            *actualType = ElementType::USHORT;
            return true;
        case cgltf_component_type_r_32u:
            *permitType = ElementType::UINT;
            *actualType = ElementType::UINT;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = ElementType::FLOAT;
            *actualType = ElementType::FLOAT;
            return true;
        default:
            return false;
        }
        break;
    case cgltf_type_vec2:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = ElementType::BYTE2;
            *actualType = ElementType::BYTE2;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = ElementType::UBYTE2;
            *actualType = ElementType::UBYTE2;
            return true;
        case cgltf_component_type_r_16:
            *permitType = ElementType::SHORT2;
            *actualType = ElementType::SHORT2;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = ElementType::USHORT2;
            *actualType = ElementType::USHORT2;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = ElementType::FLOAT2;
            *actualType = ElementType::FLOAT2;
            return true;
        default:
            return false;
        }
        break;
    case cgltf_type_vec3:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = ElementType::FLOAT3;
            *actualType = ElementType::BYTE3;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = ElementType::FLOAT3;
            *actualType = ElementType::UBYTE3;
            return true;
        case cgltf_component_type_r_16:
            *permitType = ElementType::FLOAT3;
            *actualType = ElementType::SHORT3;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = ElementType::FLOAT3;
            *actualType = ElementType::USHORT3;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = ElementType::FLOAT3;
            *actualType = ElementType::FLOAT3;
            return true;
        default:
            return false;
        }
        break;
    case cgltf_type_vec4:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = ElementType::BYTE4;
            *actualType = ElementType::BYTE4;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = ElementType::UBYTE4;
            *actualType = ElementType::UBYTE4;
            return true;
        case cgltf_component_type_r_16:
            *permitType = ElementType::SHORT4;
            *actualType = ElementType::SHORT4;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = ElementType::USHORT4;
            *actualType = ElementType::USHORT4;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = ElementType::FLOAT4;
            *actualType = ElementType::FLOAT4;
            return true;
        default:
            return false;
        }
        break;
    default:
        return false;
    }
    return false;
}

inline static bool _RequiresConversion(cgltf_type type, cgltf_component_type ctype) {
    Soul::GPU::ElementType permitted;
    Soul::GPU::ElementType actual;
    bool supported = _GetElementType(type, ctype, &permitted, &actual);
    return supported && permitted != actual;
}

static const char* _GetNodeName(const cgltf_node* node, const char* defaultNodeName) {
	if (node->name) return node->name;
	if (node->mesh && node->mesh->name) return node->mesh->name;
	if (node->light && node->light->name) return node->light->name;
	if (node->camera && node->camera->name) return node->camera->name;
	return defaultNodeName;
}

static void _ComputeUriPath(char* uriPath, const char* gltfPath, const char* uri) {
	cgltf_combine_paths(uriPath, gltfPath, uri);

	// after combining, the tail of the resulting path is a uri; decode_uri converts it into path
	cgltf_decode_uri(uriPath + strlen(uriPath) - strlen(uri));
}

namespace SoulFila {
    static void _AddAttributeToPrimitive(Primitive* primitive, VertexAttribute attrType, Soul::GPU::BufferID gpuBuffer,
        Soul::GPU::ElementType type, uint8 attributeStride) {
        primitive->vertexBuffers[primitive->vertexBindingCount] = gpuBuffer;
        primitive->attributes[attrType] = {
            0,
            attributeStride,
            primitive->vertexBindingCount,
            type
        };
        primitive->vertexBindingCount++;
    }
}


void SoulFila::Scene::importFromGLTF(const char* path) {

	char* uriPath = (char*) Soul::Runtime::GetTempAllocator()->allocate(strlen(path) + GLTF_URI_MAX_LENGTH + 1, alignof(char), "").addr;

	cgltf_options options = {};
	cgltf_data* asset = nullptr;
	
	cgltf_result result = cgltf_parse_file(&options, path, &asset);
	SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf json");

	result = cgltf_load_buffers(&options, asset, path);
	SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf buffers");

	_textures.clear();
	_textures.resize(asset->textures_count);
	for (int i = 0; i < asset->textures_count; i++) {
		const cgltf_texture& srcTexture = asset->textures[i];
		const cgltf_buffer_view& bv = *srcTexture.image->buffer_view;
		void** data = &bv ? &bv.buffer->data : nullptr;
		const uint32_t totalSize = uint32_t(&bv ? bv.size : 0);

		Soul::GPU::TextureDesc texDesc;
		texDesc.type = Soul::GPU::TextureType::D2;
		texDesc.format = Soul::GPU::TextureFormat::RGBA8;
		texDesc.depth = 1;
		texDesc.usageFlags = Soul::GPU::TEXTURE_USAGE_SAMPLED_BIT;
		texDesc.queueFlags = Soul::GPU::QUEUE_GRAPHIC_BIT;
		byte* texels = nullptr;
		
		if (data) {
			const uint64 offset = &bv ? bv.offset : 0;
			const uint8_t* sourceData = offset + (const uint8_t*)*data;
			int width, height, comp;
			texels = stbi_load_from_memory(sourceData, totalSize,
				&width, &height, &comp, 4);
			SOUL_ASSERT(0, texels != nullptr, "Fail to load texels");
			texDesc.width = width;
			texDesc.height = height;
		}
		else {
			_ComputeUriPath(uriPath, path, srcTexture.image->uri);
			int width, height, comp;
			texels = stbi_load(uriPath, &width, &height, &comp, 4);
			SOUL_ASSERT(0, texels != nullptr, "Fail to load texels");
			texDesc.width = width;
			texDesc.height = height;
		}
		texDesc.mipLevels = floorLog2(max(texDesc.width, texDesc.height));

		_textures[i] = _gpuSystem->textureCreate(texDesc, texels, totalSize);
	}

    _materials.clear();
    _materials.resize(asset->materials_count);

    auto _getTextureView = [asset](const cgltf_texture_view& srcTextureView) -> TextureView {
        TextureView dstTextureView;
        dstTextureView.textureID = TextureID(srcTextureView.texture - asset->textures);
        dstTextureView.texCoord = srcTextureView.texcoord;
        const cgltf_texture_transform& transform = srcTextureView.transform;
        dstTextureView.transform = _MatrixFromUVTransform(_GetVec2f(transform.offset), transform.rotation, _GetVec2f(transform.scale));
        return dstTextureView;
    };
    

    for (cgltf_size matIndex = 0; matIndex < asset->materials_count; matIndex++) {
        const cgltf_material& srcMaterial = asset->materials[matIndex];
        Material& dstMaterial = _materials[matIndex];

        if (srcMaterial.has_pbr_metallic_roughness) dstMaterial.flags |= MATERIAL_HAS_PBR_METALLIC_ROUGHNESS_BIT;
        if (srcMaterial.has_pbr_specular_glossiness) dstMaterial.flags |= MATERIAL_HAS_PBR_SPECULAR_GLOSSINESS_BIT;
        if (srcMaterial.has_clearcoat) dstMaterial.flags |= MATERIAL_HAS_CLEARCOAT;
        if (srcMaterial.has_transmission) dstMaterial.flags |= MATERIAL_HAS_TRANSMISSION;
        if (srcMaterial.has_volume) dstMaterial.flags |= MATERIAL_HAS_VOLUME;
        if (srcMaterial.has_ior) dstMaterial.flags |= MATERIAL_HAS_IOR;
        if (srcMaterial.has_specular) dstMaterial.flags |= MATERIAL_HAS_SPECULAR;
        if (srcMaterial.has_sheen) dstMaterial.flags |= MATERIAL_HAS_SHEEN;
        if (srcMaterial.double_sided) dstMaterial.flags |= MATERIAL_DOUBLE_SIDED;
        if (srcMaterial.unlit) dstMaterial.flags |= MATERIAL_UNLIT;

        dstMaterial.metallicRoughness = {
            _getTextureView(srcMaterial.pbr_metallic_roughness.base_color_texture),
            _getTextureView(srcMaterial.pbr_metallic_roughness.metallic_roughness_texture),
            _GetVec4f(srcMaterial.pbr_metallic_roughness.base_color_factor),
            srcMaterial.pbr_metallic_roughness.metallic_factor,
            srcMaterial.pbr_metallic_roughness.roughness_factor
        };

        const cgltf_pbr_specular_glossiness& sgConfig = srcMaterial.pbr_specular_glossiness;
        dstMaterial.specularGlossiness = {
            _getTextureView(sgConfig.diffuse_texture),
            _getTextureView(sgConfig.specular_glossiness_texture),
            _GetVec4f(sgConfig.diffuse_factor),
            _GetVec3f(sgConfig.specular_factor),
            sgConfig.glossiness_factor
        };

        const cgltf_clearcoat& clearcoatConfig = srcMaterial.clearcoat;
        dstMaterial.clearcoat = {
            _getTextureView(clearcoatConfig.clearcoat_texture),
            _getTextureView(clearcoatConfig.clearcoat_roughness_texture),
            _getTextureView(clearcoatConfig.clearcoat_normal_texture),
            clearcoatConfig.clearcoat_factor,
            clearcoatConfig.clearcoat_roughness_factor
        };

        dstMaterial.ior = srcMaterial.ior.ior;

        const cgltf_specular& specularConfig = srcMaterial.specular;
        dstMaterial.specular = {
            _getTextureView(specularConfig.specular_texture),
            _getTextureView(specularConfig.specular_color_texture),
            _GetVec3f(specularConfig.specular_color_factor),
            specularConfig.specular_factor
        };

        const cgltf_sheen& sheenConfig = srcMaterial.sheen;
        dstMaterial.sheen = {
            _getTextureView(sheenConfig.sheen_color_texture),
            _getTextureView(sheenConfig.sheen_roughness_texture),
            _GetVec3f(sheenConfig.sheen_color_factor),
            sheenConfig.sheen_roughness_factor
        };

        const cgltf_transmission& transmissionConfig = srcMaterial.transmission;
        dstMaterial.transmission = {
            _getTextureView(transmissionConfig.transmission_texture),
            transmissionConfig.transmission_factor
        };

        const cgltf_volume& volumeConfig = srcMaterial.volume;
        dstMaterial.volume = {
            _getTextureView(volumeConfig.thickness_texture),
            _GetVec3f(volumeConfig.attenuation_color),
            volumeConfig.attenuation_distance,
            volumeConfig.thickness_factor
        };

        dstMaterial.normalTexture = _getTextureView(srcMaterial.normal_texture);
        dstMaterial.occlusionTexture = _getTextureView(srcMaterial.occlusion_texture);
        dstMaterial.emmisiveTexture = _getTextureView(srcMaterial.emissive_texture);
        dstMaterial.emmisiveFactor = _GetVec3f(srcMaterial.emissive_factor);
        dstMaterial.alphaMode = _GetAlphaMode(srcMaterial.alpha_mode);
        dstMaterial.alphaCutoff = srcMaterial.alpha_cutoff;
    }

	_meshes.clear();
	_meshes.resize(asset->meshes_count);
	for (cgltf_size meshIndex = 0; meshIndex < asset->meshes_count; meshIndex++) {
		const cgltf_mesh& srcMesh = asset->meshes[meshIndex];
        Mesh& dstMesh = _meshes[meshIndex];
        dstMesh.primitives.resize(srcMesh.primitives_count);

        GPU::BufferDesc indexBufferDesc;
        indexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
        indexBufferDesc.usageFlags = GPU::BUFFER_USAGE_INDEX_BIT;

        for (cgltf_size primitiveIndex = 0; primitiveIndex < srcMesh.primitives_count; primitiveIndex++) {
            Soul::Memory::ScopeAllocator<> primitiveScopeAllocator("Loading Attribute Allocation");

            TangentFrameComputeInput qtangentComputeInput;

            const cgltf_primitive& srcPrimitive = srcMesh.primitives[primitiveIndex];

            if (srcPrimitive.has_draco_mesh_compression) {
                SOUL_NOT_IMPLEMENTED();
            }

            Primitive& dstPrimitive = dstMesh.primitives[primitiveIndex];
            dstPrimitive.materialID = srcPrimitive.material == nullptr ? MATERIAL_ID_NULL : MaterialID(srcPrimitive.material - asset->materials);
            
            bool getTopologySuccess = _GetTopology(srcPrimitive.type, &dstPrimitive.topology);
            SOUL_ASSERT(0, getTopologySuccess, "");

            // build index buffer
            if (srcPrimitive.indices) {

                const cgltf_accessor& indices = *srcPrimitive.indices;

                const byte* bufferData = (const uint8*)indices.buffer_view->buffer->data + _ComputeBindingOffset(indices);
                
                switch (srcPrimitive.indices->component_type) {
                case cgltf_component_type_r_8u: 
                case cgltf_component_type_r_16u: {
                    using IndexType = uint16;
                    indexBufferDesc.typeSize = sizeof(IndexType);
                    indexBufferDesc.typeAlignment = alignof(IndexType);
                    indexBufferDesc.count = indices.count;
                    uint64 indexStride = indices.stride;

                    dstPrimitive.indexBuffer = _gpuSystem->bufferCreate(indexBufferDesc,
                        [bufferData, indexStride](int i, byte* data) {
                            const auto index = (IndexType*)data;
                            (*index) = IndexType(*(bufferData + indexStride * i));
                        }
                    );
                    break;
                }
                case cgltf_component_type_r_32u: {
                    using IndexType = uint32;
                    indexBufferDesc.typeSize = sizeof(IndexType);
                    indexBufferDesc.typeAlignment = alignof(IndexType);
                    indexBufferDesc.count = indices.count;
                    uint64 indexStride = indices.stride;

                    dstPrimitive.indexBuffer = _gpuSystem->bufferCreate(indexBufferDesc,
                        [bufferData, indexStride](int i, byte* data) {
                            const auto index = (IndexType*)data;
                            (*index) = IndexType(*(bufferData + indexStride *i));
                        }
                    );
                    break;
                }
                default:
                    SOUL_NOT_IMPLEMENTED();
                }

                qtangentComputeInput.triangles32 = (Vec3ui32*)primitiveScopeAllocator.allocate(indices.count * sizeof(uint32), alignof(Vec3ui32), "").addr;
                for (size_t tri = 0, indexIdx = 0; tri < indices.count / 3; ++tri) {
                    auto& triangle = qtangentComputeInput.triangles32[tri];
                    triangle.x = cgltf_accessor_read_index(&indices, indexIdx++);
                    triangle.y = cgltf_accessor_read_index(&indices, indexIdx++);
                    triangle.z = cgltf_accessor_read_index(&indices, indexIdx++);
                }

            }
            else if (srcPrimitive.attributes_count > 0) {

                using IndexType = uint32;
                indexBufferDesc.count = srcPrimitive.attributes[0].data->count;

                const uint64 indexDataSize = indexBufferDesc.count * sizeof(IndexType);
                IndexType* indexData = (IndexType*)primitiveScopeAllocator.allocate(indexDataSize, alignof(IndexType), "").addr;
                for (uint64 i = 0; i < indexBufferDesc.count; ++i) {
                    indexData[i] = i;
                }
                indexBufferDesc.typeSize = sizeof(IndexType);
                indexBufferDesc.typeAlignment = alignof(IndexType);

                dstPrimitive.indexBuffer = _gpuSystem->bufferCreate(indexBufferDesc,
                    [](int i, byte* data) {
                        const auto index = (IndexType*)data;
                        (*index) = IndexType(i);
                    }
                );

                qtangentComputeInput.triangles32 = (Vec3ui32*)indexData;

            }
            
            qtangentComputeInput.vertexCount = srcPrimitive.attributes[0].data->count;
            qtangentComputeInput.triangleCount = indexBufferDesc.count / 3;

            for (cgltf_size aindex = 0; aindex < srcPrimitive.attributes_count; aindex++) {

                const cgltf_attribute& srcAttribute = srcPrimitive.attributes[aindex];
                const cgltf_accessor& accessor = *srcAttribute.data;

                if (srcAttribute.type == cgltf_attribute_type_weights) {
                    auto normalize = [](cgltf_accessor* data) {
                        if (data->type != cgltf_type_vec4 || data->component_type != cgltf_component_type_r_32f) {
                            SOUL_LOG_ERROR("Attribute type is not supported");
                            SOUL_NOT_IMPLEMENTED();
                        }
                        uint8_t* bytes = (uint8_t*)data->buffer_view->buffer->data;
                        bytes += data->offset + data->buffer_view->offset;
                        for (cgltf_size i = 0, n = data->count; i < n; ++i, bytes += data->stride) {
                            Soul::Vec4f* weights = (Vec4f*)bytes;
                            const float sum = weights->x + weights->y + weights->z + weights->w;
                            *weights /= sum;
                        }
                    };

                    normalize(srcAttribute.data);
                }       

                const byte* attributeData = nullptr;
                uint64 attributeStride = 0;
                uint64 attributeDataCount = 0;
                uint64 attributeTypeSize = 0;
                uint64 attributeTypeAlignment = 0;

                if (accessor.is_sparse || srcAttribute.type == cgltf_attribute_type_tangent || 
                    srcAttribute.type == cgltf_attribute_type_normal || 
                    srcAttribute.type == cgltf_attribute_type_position) {
                    cgltf_size numFloats = accessor.count * cgltf_num_components(accessor.type);

                    cgltf_size numBytes = sizeof(float) * numFloats;
                    byte* generated = (byte*) primitiveScopeAllocator.allocate(numBytes, alignof(float), "").addr;
                    cgltf_accessor_unpack_floats(&accessor, (float*)generated, numFloats);

                    attributeData = generated;
                    attributeDataCount = accessor.count;
                    attributeTypeSize = cgltf_num_components(accessor.type) * sizeof(float);
                    attributeTypeAlignment = sizeof(float);
                    attributeStride = attributeTypeSize;
                }
                else {
                    const byte* bufferData = (const byte*)accessor.buffer_view->buffer->data;
                    attributeData = bufferData + _ComputeBindingOffset(accessor);
                    attributeDataCount = accessor.count;
                    attributeTypeSize = cgltf_calc_size(accessor.type, accessor.component_type);
                    attributeTypeAlignment = cgltf_component_size(accessor.component_type);
                    attributeStride = accessor.stride;
                }

                if (srcAttribute.type == cgltf_attribute_type_tangent) {
                    SOUL_ASSERT(0, sizeof(Vec4f) == attributeStride, "");
                    qtangentComputeInput.tangents = (Vec4f*)attributeData;
                    continue;
                }
                if (srcAttribute.type == cgltf_attribute_type_normal) {
                    SOUL_ASSERT(0, sizeof(Vec3f) == attributeStride, "");
                    qtangentComputeInput.normals = (Vec3f*)attributeData;
                    continue;
                }

                if (accessor.component_type == cgltf_attribute_type_texcoord && srcAttribute.index == 0) {
                    cgltf_size numFloats = accessor.count * cgltf_num_components(accessor.type);
                    cgltf_size numBytes = sizeof(float) * numFloats;
                    Vec2f* generated = (Vec2f*)primitiveScopeAllocator.allocate(numBytes, alignof(float), "").addr;
                    cgltf_accessor_unpack_floats(&accessor, (float*)generated, numFloats);
                    qtangentComputeInput.uvs = (Vec2f*)generated;
                }

                if (accessor.component_type == cgltf_attribute_type_position) {
                    SOUL_ASSERT(0, sizeof(Vec3f) == attributeStride, "");
                    qtangentComputeInput.positions = (Vec3f*)attributeData;
                }

                GPU::BufferDesc gpuDesc;
                gpuDesc.count = attributeDataCount;
                gpuDesc.typeSize = attributeTypeSize;
                gpuDesc.typeAlignment = attributeTypeAlignment;
                gpuDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
                gpuDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
                GPU::BufferID attributeGPUBuffer = _gpuSystem->bufferCreate(gpuDesc,
                    [attributeData, attributeTypeSize, attributeStride](int index, byte* data)
                    {
                        uint64 offset = index * attributeStride;
                        memcpy(data, attributeData + offset, attributeTypeSize);
                    });

                VertexAttribute attrType;
                _GetVertexAttrType(srcAttribute.type, &attrType, srcAttribute.index);

                Soul::GPU::ElementType permitted;
                Soul::GPU::ElementType actual;
                _GetElementType(accessor.type, accessor.component_type, &permitted, &actual);

                _AddAttributeToPrimitive(&dstPrimitive, attrType, attributeGPUBuffer, actual, attributeStride);

            }

            uint64 qtangentBufferSize = qtangentComputeInput.vertexCount * sizeof(Quaternion);
            Quaternion* qtangents = (Quaternion*) primitiveScopeAllocator.allocate(qtangentBufferSize, alignof(Quaternion), "").addr;
            if (ComputeTangentFrame(qtangentComputeInput, qtangents)) {
                GPU::BufferDesc qtangentsBufferDesc = {
                     qtangentComputeInput.vertexCount,
                     sizeof(Quaternion),
                     alignof(Quaternion),
                     GPU::BUFFER_USAGE_VERTEX_BIT,
                     GPU::QUEUE_GRAPHIC_BIT
                };
                Soul::GPU::BufferID qtangentsGPUBUffer = _gpuSystem->bufferCreate(qtangentsBufferDesc, qtangents);
                _AddAttributeToPrimitive(&dstPrimitive, VertexAttribute::TANGENTS, qtangentsGPUBUffer, GPU::ElementType::FLOAT, sizeof(Quaternion));
            }

            cgltf_size targetsCount = srcPrimitive.targets_count;
            targetsCount = targetsCount > MAX_MORPH_TARGETS ? MAX_MORPH_TARGETS: targetsCount;

            constexpr int baseTangentsAttr = (int)VertexAttribute::MORPH_TANGENTS_0;
            constexpr int basePositionAttr = (int)VertexAttribute::MORPH_POSITION_0;
            for (cgltf_size targetIndex = 0; targetIndex < targetsCount; targetIndex++) {
                const cgltf_morph_target& morphTarget = srcPrimitive.targets[targetIndex];
                
                enum class MorphTargetType : uint8 {
                    POSITION,
                    NORMAL,
                    TANGENT,
                    COUNT
                };

                auto getMorphTargetType = [](cgltf_attribute_type atype, MorphTargetType* targetType) -> bool {
                    switch (atype) {
                    case cgltf_attribute_type_position:
                        *targetType = MorphTargetType::POSITION;
                        return true;
                    case cgltf_attribute_type_tangent:
                        *targetType = MorphTargetType::TANGENT;
                        return true;
                    case cgltf_attribute_type_normal:
                        *targetType = MorphTargetType::NORMAL;
                        return true;
                    default:
                        return false;
                    }
                };

                Soul::EnumArray<MorphTargetType, const byte*> morphTargetAttributes(nullptr);
                
                for (cgltf_size aindex = 0; aindex < morphTarget.attributes_count; aindex++) {
                    const cgltf_attribute& srcAttribute = morphTarget.attributes[aindex];
                    const cgltf_accessor& accessor = *srcAttribute.data;
                    const cgltf_attribute_type atype = srcAttribute.type;
                    const int morphId = targetIndex + 1;

                    const byte* attributeData = nullptr;
                    uint64 attributeStride = 0;
                    uint64 attributeDataCount = 0;
                    uint64 attributeTypeSize = 0;
                    uint64 attributeTypeAlignment = 0;

                    MorphTargetType morphTargetType;
                    bool success = getMorphTargetType(srcAttribute.type, &morphTargetType);
                    SOUL_ASSERT(0, success, "");

                    cgltf_size numFloats = accessor.count * cgltf_num_components(accessor.type);
                    cgltf_size numBytes = sizeof(float) * numFloats;
                    byte* generated = (byte*)primitiveScopeAllocator.allocate(numBytes, alignof(float), "").addr;
                    cgltf_accessor_unpack_floats(&accessor, (float*)generated, numFloats);

                    attributeData = generated;
                    attributeDataCount = accessor.count;
                    attributeTypeSize = cgltf_num_components(accessor.type) * sizeof(float);
                    attributeTypeAlignment = sizeof(float);
                    attributeStride = attributeTypeSize;
                    
                    morphTargetAttributes[morphTargetType] = attributeData;

                    if (srcAttribute.type == cgltf_attribute_type_position) {
                        auto attrType = (VertexAttribute)(basePositionAttr + targetIndex);
                        GPU::BufferDesc gpuDesc;
                        gpuDesc.count = attributeDataCount;
                        gpuDesc.typeSize = attributeTypeSize;
                        gpuDesc.typeAlignment = attributeTypeAlignment;
                        gpuDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
                        gpuDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
                        GPU::BufferID attributeGPUBuffer = _gpuSystem->bufferCreate(gpuDesc,
                            [attributeData, attributeTypeSize, attributeStride](int index, byte* data)
                            {
                                uint64 offset = index * attributeStride;
                                memcpy(data, attributeData + offset, attributeTypeSize);
                            });

                        Soul::GPU::ElementType permitted;
                        Soul::GPU::ElementType actual;
                        _GetElementType(accessor.type, accessor.component_type, &permitted, &actual);

                        _AddAttributeToPrimitive(&dstPrimitive, attrType, attributeGPUBuffer, actual, attributeStride);
                    }
                }

                if (morphTargetAttributes[MorphTargetType::NORMAL]) {
                    if (qtangentComputeInput.normals) {

                        Soul::Vec3f* normalTarget = (Soul::Vec3f*)morphTargetAttributes[MorphTargetType::NORMAL];
                        if (normalTarget != nullptr) {
                            for (cgltf_size vertIndex = 0; vertIndex < qtangentComputeInput.vertexCount; vertIndex++) {
                                qtangentComputeInput.normals[vertIndex] += normalTarget[vertIndex];
                            }
                        }

                        Soul::Vec3f* tangentTarget = (Soul::Vec3f*)morphTargetAttributes[MorphTargetType::TANGENT];
                        if (tangentTarget != nullptr) {
                            for (cgltf_size vertIndex = 0; vertIndex < qtangentComputeInput.vertexCount; vertIndex++) {
                                qtangentComputeInput.tangents[vertIndex].xyz += tangentTarget[vertIndex];
                            }
                        }

                        Soul::Vec3f* positionTarget = (Soul::Vec3f*)morphTargetAttributes[MorphTargetType::POSITION];
                        if (positionTarget != nullptr) {
                            for (cgltf_size vertIndex = 0; vertIndex < qtangentComputeInput.vertexCount; vertIndex++) {
                                qtangentComputeInput.positions[vertIndex] += positionTarget[vertIndex];
                            }
                        }
                    }
                }

                uint64 qtangentBufferSize = qtangentComputeInput.vertexCount * sizeof(Quaternion);
                Quaternion* qtangents = (Quaternion*)primitiveScopeAllocator.allocate(qtangentBufferSize, alignof(Quaternion), "").addr;
                if (ComputeTangentFrame(qtangentComputeInput, qtangents)) {
                    GPU::BufferDesc qtangentsBufferDesc = {
                         qtangentComputeInput.vertexCount,
                         sizeof(Quaternion),
                         alignof(Quaternion),
                         GPU::BUFFER_USAGE_VERTEX_BIT,
                         GPU::QUEUE_GRAPHIC_BIT
                    };
                    Soul::GPU::BufferID qtangentsGPUBUffer = _gpuSystem->bufferCreate(qtangentsBufferDesc, qtangents);
                    _AddAttributeToPrimitive(&dstPrimitive, (VertexAttribute)(baseTangentsAttr + targetIndex), qtangentsGPUBUffer, GPU::ElementType::FLOAT, sizeof(Quaternion));
                }

            }

        }
	}

	_rootEntity = _registry.create();

	const cgltf_scene* scene = asset->scene ? asset->scene : asset->scenes;
	if (!scene) {
		return;
	}

	// create root entity
	_rootEntity = _registry.create();
	_registry.emplace<TransformComponent>(_rootEntity, mat4Identity(), mat4Identity(), _rootEntity, ENTITY_ID_NULL, ENTITY_ID_NULL, ENTITY_ID_NULL);
	
	for (cgltf_size i = 0, len = scene->nodes_count; i < len; ++i) {
		cgltf_node** nodes = scene->nodes;
		_createEntity(asset, nodes[i], _rootEntity);
	}

	cgltf_free(asset);
}

void SoulFila::Scene::_createEntity(const cgltf_data* asset, const cgltf_node* node, EntityID parent) {
	EntityID entity = _registry.create();

	Soul::Mat4 localTransform;
	if (node->has_matrix) {
		localTransform = Soul::mat4((float*)node->matrix);
	}
	else {
		Soul::Vec3f translation((float*)node->translation);
		Soul::Vec3f scale((float*)node->scale);
		Soul::Quaternion rotation((float*)node->rotation);
		localTransform = Soul::mat4Transform(Soul::Transform{ translation, scale, rotation });
	}

	TransformComponent& parentTransform = _registry.get<TransformComponent>(parent);
	const Mat4 worldTransform = parentTransform.world * localTransform;
    EntityID nextEntity = parentTransform.firstChild;
    parentTransform.firstChild = entity;
    if (nextEntity == ENTITY_ID_NULL) {
        TransformComponent& nextTransform = _registry.get<TransformComponent>(nextEntity);
        nextTransform.prev = entity;
    }

	_registry.emplace<TransformComponent>(entity, localTransform, worldTransform, parent, ENTITY_ID_NULL, nextEntity, ENTITY_ID_NULL);

	if (node->mesh) {
		_createRenderable(asset, node, entity);
	}
    if (node->light) {
        _createLight(asset, node, entity);
    }
    if (node->camera) {
        _createCamera(asset, node, entity);
    }

    for (cgltf_size i = 0; i < node->children_count; i++) {
        _createEntity(asset, node->children[i], entity);
    }
}

void SoulFila::Scene::_createRenderable(const cgltf_data* asset, const cgltf_node* node, EntityID entity) {

    Visibility visibility;
    visibility.priority = 0x4;
    visibility.castShadows = true;
    visibility.receiveShadows = true;
    visibility.culling = true;

    SOUL_ASSERT(0, node->mesh != nullptr, "");

    cgltf_mesh& srcMesh = *node->mesh;
    MeshID meshID = MeshID(node->mesh - asset->meshes);

    SOUL_ASSERT(0, srcMesh.primitives_count > 0, "");

    cgltf_size numMorphTargets = srcMesh.primitives[0].targets_count;
    visibility.morphing = numMorphTargets > 0;
    visibility.screenSpaceContactShadows = false;

    Soul::Vec4f morphWeights;
    if (numMorphTargets > 0) {
        for (cgltf_size i = 0; i < std::min(MAX_MORPH_TARGETS, srcMesh.weights_count); ++i) {
            morphWeights.mem[i] = srcMesh.weights[i];
        }
        for (cgltf_size i = 0; i < std::min(MAX_MORPH_TARGETS, node->weights_count); ++i) {
            morphWeights.mem[i] = node->weights[i];
        }
    }

    SkinID skinID = node->skin != nullptr ? SkinID(node->skin - asset->skins) : SKIN_ID_NULL;

    _registry.emplace<RenderComponent>(entity, visibility, meshID, skinID, morphWeights);
}

void SoulFila::Scene::_createLight(const cgltf_data* asset, const cgltf_node* node, EntityID entity) {
    
    SOUL_ASSERT(0, node->light != nullptr, "");
    const cgltf_light& light = *node->light;
    
    LightType lightType;
    lightType.type = _GetLightType(light.type);
    lightType.shadowCaster = true;
    lightType.lightCaster = true;
    Soul::Vec3f direction(0.0f, 0.0f, -1.0f);
    Soul::Vec3f color(light.color[0], light.color[1], light.color[2]);
    float falloff = light.range == 0.0f ? 10.0f : light.range;
    float luminousPower = light.intensity;
    float luminousIntensity;

    ShadowParams shadowParams;
    SpotParams spotParams;

    if (lightType.type == LightRadiationType::SPOT || lightType.type == LightRadiationType::FOCUSED_SPOT) {
        float innerClamped = std::min(std::abs(light.spot_inner_cone_angle), FCONST::PI_2);
        float outerClamped = std::min(std::abs(light.spot_outer_cone_angle), FCONST::PI_2);

        // outer must always be bigger than inner
        outerClamped = std::max(innerClamped, outerClamped);

        float cosOuter = std::cos(outerClamped);
        float cosInner = std::cos(innerClamped);
        float cosOuterSquared = cosOuter * cosOuter;
        float scale = 1 / std::max(1.0f / 1024.0f, cosInner - cosOuter);
        float offset = -cosOuter * scale;

        spotParams.outerClamped = outerClamped;
        spotParams.cosOuterSquared = cosOuterSquared;
        spotParams.sinInverse = 1 / std::sqrt(1 - cosOuterSquared);
        spotParams.scaleOffset = { scale, offset };
    }

    switch (lightType.type) {
        case LightRadiationType::SUN:
        case LightRadiationType::DIRECTIONAL:
            // luminousPower is in lux, nothing to do.
            luminousIntensity = luminousPower;
            break;

        case LightRadiationType::POINT:
            luminousIntensity = luminousPower * FCONST::ONE_OVER_PI * 0.25f;
            break;

        case LightRadiationType::FOCUSED_SPOT: {

            float cosOuter = std::sqrt(spotParams.cosOuterSquared);
            // intensity specified directly in candela, no conversion needed
            luminousIntensity = luminousPower;
            // lp = li * (2 * pi * (1 - cos(cone_outer / 2)))
            luminousPower = luminousIntensity * (FCONST::TAU * (1.0f - cosOuter));
            spotParams.luminousPower = luminousPower;
            break;
        }
        case LightRadiationType::SPOT:
            luminousIntensity = luminousPower;
            break;
    }
    _registry.emplace<LightComponent>(entity, lightType, Soul::Vec3f(0, 0, 0), direction, color, ShadowParams(), spotParams, 0.0f, 0.0f, 0.0f, luminousIntensity, falloff);
}

void SoulFila::Scene::_createCamera(const cgltf_data* data, const cgltf_node* node, EntityID entity) {
    CameraComponent& cameraComponent = _registry.emplace<CameraComponent>(entity);
    
    SOUL_ASSERT(0, node->camera != nullptr, "");
    const cgltf_camera& srcCamera = *node->camera;

    if (srcCamera.type == cgltf_camera_type_perspective) {
        const cgltf_camera_perspective& srcPerspective = srcCamera.data.perspective;
        const double far = srcPerspective.zfar > 0.0 ? srcPerspective.zfar : 100000000;
        cameraComponent.setPerspectiveProjection(srcPerspective.yfov, srcPerspective.aspect_ratio, srcPerspective.znear, far);
    }
    else if (srcCamera.type == cgltf_camera_type_orthographic) {
        const cgltf_camera_orthographic& srcOrthographic = srcCamera.data.orthographic;
        const double left = -srcOrthographic.xmag * 0.5;
        const double right = srcOrthographic.xmag * 0.5;
        const double bottom = -srcOrthographic.ymag * 0.5;
        const double top = srcOrthographic.ymag * 0.5;
        cameraComponent.setOrthoProjection(left, right, bottom, top, srcOrthographic.znear, srcOrthographic.zfar);
    }
    else {
        SOUL_NOT_IMPLEMENTED();
    }
}

void SoulFila::CameraComponent::setOrthoProjection(double left, double right, double bottom, double top, double near, double far) {
    this->projection = mat4Ortho(left, right, bottom, top, near, far);
    this->projectionForCulling = this->projection;

    this->near = near;
    this->far = far;
}

void SoulFila::CameraComponent::setPerspectiveProjection(double fovRadian, double aspectRatio, double near, double far) {
    this->projectionForCulling = mat4Perspective(fovRadian, aspectRatio, near, far);
    this->projection = this->projectionForCulling;
    
    // Make far infinity, lim (zFar -> infinity), then (zNear + zFar) * -1 / (zFar - zNear) = -1
    this->projection.elem[2][2] = -1;
    // lim (zFar -> infinity), then (-2 * zFar *zNear) / (zFar - zNear) = -2 * zNear
    this->projection.elem[2][3] = -2 * near;

    this->near = near;
    this->far = far;
}

void SoulFila::CameraComponent::setScaling(Soul::Vec2f scaling) {
    this->scaling = scaling;
}

Soul::Vec2f SoulFila::CameraComponent::getScaling() {
    return this->scaling;
}

Soul::Mat4 SoulFila::CameraComponent::getProjectionMatrix() {
    return this->projection;
}

Soul::Mat4 SoulFila::CameraComponent::getCullingProjectionMatrix() {
    return this->projectionForCulling;
}


