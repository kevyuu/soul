#include <algorithm>

#include "imgui/imgui.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core/math.h"
#include "core/geometry.h"
#include "core/enum_array.h"

#include "runtime/scope_allocator.h"
#include "gpu_program_registry.h"

#include "data.h"
#include "../../camera_manipulator.h"
#include "../../ui/widget.h"

// Define a mapping from a uv set index in the source asset to one of Filament's uv sets.
enum UvSet : uint8 { UNUSED, UV0, UV1 };
constexpr int UV_MAP_SIZE = 8;
using UvMap = std::array<UvSet, UV_MAP_SIZE>;
constexpr ImGuiTreeNodeFlags SCENE_TREE_FLAGS = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

void ItemRowsBackground(float lineHeight = -1.0f, const ImColor& color = ImColor(20, 20, 20, 64))
{
    auto* drawList = ImGui::GetWindowDrawList();
    const auto& style = ImGui::GetStyle();

    if (lineHeight < 0)
    {
        lineHeight = ImGui::GetTextLineHeight();
    }
    lineHeight += style.ItemSpacing.y;

    const float scrollOffsetH = ImGui::GetScrollX();
    float scrollOffsetV = ImGui::GetScrollY();
    const float scrolledOutLines = floorf(scrollOffsetV / lineHeight);
    scrollOffsetV -= lineHeight * scrolledOutLines;

    ImVec2 clipRectMin(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
    ImVec2 clipRectMax(clipRectMin.x + ImGui::GetWindowWidth(), clipRectMin.y + ImGui::GetWindowHeight());

    if (ImGui::GetScrollMaxX() > 0)
    {
        clipRectMax.y -= style.ScrollbarSize;
    }

    drawList->PushClipRect(clipRectMin, clipRectMax);

    bool isOdd = (static_cast<int>(scrolledOutLines) % 2) == 0;

    float yMin = clipRectMin.y - scrollOffsetV + ImGui::GetCursorPosY();
    float yMax = clipRectMax.y - scrollOffsetV + lineHeight;
    float xMin = clipRectMin.x + scrollOffsetH + ImGui::GetWindowContentRegionMin().x;
    float xMax = clipRectMin.x + scrollOffsetH + ImGui::GetWindowContentRegionMax().x;

    for (float y = yMin; y < yMax; y += lineHeight, isOdd = !isOdd)
    {
        if (isOdd)
        {
            drawList->AddRectFilled({ xMin, y - style.ItemSpacing.y }, { xMax, y + lineHeight }, color);
        }
    }

    drawList->PopClipRect();
}

static void MakeBone(SoulFila::BoneUBO* out, const Soul::Mat4f& t) {
    Soul::Mat4f m = mat4Transpose(t);

    // figure out the scales
    Vec4f s = { length(m.rows[0]), length(m.rows[1]), length(m.rows[2]), 0.0f };
    if (dot(cross(m.rows[0].xyz, m.rows[1].xyz), m.rows[2].xyz) < 0) {
        s.mem[2] = -s.mem[2];
    }

    // compute the inverse scales
    Vec4f is = { 1.0f / s.x, 1.0f / s.y, 1.0f / s.z, 0.0f };

    // normalize the matrix
    m.rows[0] *= is.mem[0];
    m.rows[1] *= is.mem[1];
    m.rows[2] *= is.mem[2];

    out->s = s;
    out->q = quaternionFromMat4(mat4Transpose(m));
    out->t = m.rows[3];
    out->ns = is / std::max({ abs(is.x), abs(is.y), abs(is.z), abs(is.w) });
}


static uint8 GetNumUvSets(const UvMap& uvmap) {
    return std::max({
        uvmap[0], uvmap[1], uvmap[2], uvmap[3],
        uvmap[4], uvmap[5], uvmap[6], uvmap[7],
    });
};

static constexpr cgltf_material GetDefaultCGLTFMaterial() {
    cgltf_material kDefaultMat{};
    kDefaultMat.name = nullptr;
    kDefaultMat.has_pbr_metallic_roughness = true;
    kDefaultMat.has_pbr_specular_glossiness = false;
    kDefaultMat.has_clearcoat = false;
    kDefaultMat.has_transmission = false;
    kDefaultMat.has_ior = false;
    kDefaultMat.has_specular = false;
    kDefaultMat.has_sheen = false;
    kDefaultMat.pbr_metallic_roughness = {
        {},
        {},
        {1.0, 1.0, 1.0, 1.0},
        1.0,
        1.0,
        {}
    };
    return kDefaultMat;
}

static void ConstrainGPUProgramKey(SoulFila::GPUProgramKey* key, UvMap* uvmap) {
    constexpr int MAX_INDEX = 2;
    UvMap retval {};
    int index = 1;


    if (key->hasBaseColorTexture) {
        retval[key->baseColorUV] = (UvSet) index++;
    }
    key->baseColorUV = retval[key->baseColorUV];

    if (key->hasMetallicRoughnessTexture && retval[key->metallicRoughnessUV] == UNUSED) {
        retval[key->metallicRoughnessUV] = (UvSet) index++;
    }
    key->metallicRoughnessUV = retval[key->metallicRoughnessUV];

    if (key->hasNormalTexture && retval[key->normalUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasNormalTexture = false;
        } else {
            retval[key->normalUV] = (UvSet) index++;
        }
    }
    key->normalUV = retval[key->normalUV];
    
    if (key->hasOcclusionTexture && retval[key->aoUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasOcclusionTexture = false;
        } else {
            retval[key->aoUV] = (UvSet) index++;       
        }
    }
    key->aoUV = retval[key->aoUV];

    if (key->hasEmissiveTexture && retval[key->emissiveUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasEmissiveTexture = false;
        } else {
            retval[key->emissiveUV] = (UvSet) index++;        
        }
    }
    key->emissiveUV = retval[key->emissiveUV];

    if (key->hasTransmissionTexture && retval[key->transmissionUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasTransmissionTexture = false;
        } else {
            retval[key->transmissionUV] = (UvSet) index++;        
        }
    }
    key->transmissionUV = retval[key->transmissionUV];

    if (key->hasClearCoatTexture && retval[key->clearCoatUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasClearCoatTexture = false;
        } else {
            retval[key->clearCoatUV] = (UvSet) index++;        
        }
    }
    key->clearCoatUV = retval[key->clearCoatUV];

    if (key->hasClearCoatRoughnessTexture && retval[key->clearCoatRoughnessUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasClearCoatRoughnessTexture = false;
        } else {
            retval[key->clearCoatRoughnessUV] = (UvSet) index++;
        }
    }
    key->clearCoatRoughnessUV = retval[key->clearCoatRoughnessUV];

    if (key->hasClearCoatNormalTexture && retval[key->clearCoatNormalUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasClearCoatNormalTexture = false;
        } else {
            retval[key->clearCoatNormalUV] = (UvSet) index++;
        }
    }
    key->clearCoatNormalUV = retval[key->clearCoatNormalUV];

    if (key->hasSheenColorTexture && retval[key->sheenColorUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasSheenColorTexture = false;
        } else {
            retval[key->sheenColorUV] = (UvSet) index++;
        }
    }
    key->sheenColorUV = retval[key->sheenColorUV];

    if (key->hasSheenRoughnessTexture && retval[key->sheenRoughnessUV] == UNUSED) {
        if (index > MAX_INDEX) {
            key->hasSheenRoughnessTexture = false;
        } else {
            retval[key->sheenRoughnessUV] = (UvSet) index++;        
        }
    }
    key->sheenRoughnessUV = retval[key->sheenRoughnessUV];

    // NOTE: KHR_materials_clearcoat does not provide separate UVs, we'll assume UV0
    *uvmap = retval;
}

bool PrimitiveHasVertexColor(const cgltf_primitive* inPrim) {
    for (int slot = 0; slot < inPrim->attributes_count; slot++) {
        const cgltf_attribute& inputAttribute = inPrim->attributes[slot];
        if (inputAttribute.type == cgltf_attribute_type_color) {
            return true;
        }
    }
    return false;
}


const uint32 GLTF_URI_MAX_LENGTH = 1000;

static Soul::GLSLMat3f MatrixFromUVTransform(const cgltf_texture_transform& uvt) {
    float tx = uvt.offset[0];
    float ty = uvt.offset[1];
    float sx = uvt.scale[0];
    float sy = uvt.scale[1];
    float c = cos(uvt.rotation);
    float s = sin(uvt.rotation);
    Soul::Mat3f matTransform;
    matTransform.elem[0][0] = sx * c;
    matTransform.elem[0][1] = -sy * s;
    matTransform.elem[0][2] = 0.0f;
    matTransform.elem[1][0] = sx * s;
    matTransform.elem[1][1] = sy * c;
    matTransform.elem[1][2] = 0.0f;
    matTransform.elem[2][0] = tx;
    matTransform.elem[2][1] = ty;
    matTransform.elem[2][2] = 1.0f;
    return Soul::GLSLMat3f(matTransform);
}

static bool GetVertexAttrType (cgltf_attribute_type atype,uint32 index, const UvMap& uvmap, SoulFila::VertexAttribute* attrType, bool* hasUv0) {
    using namespace SoulFila;
    switch (atype) {
    case cgltf_attribute_type_position:
        *attrType = VertexAttribute::POSITION;
        return true;
    case cgltf_attribute_type_texcoord: {
        UvSet uvset = uvmap[index];
        switch (uvset) {
        case UvSet::UV0:
            *hasUv0 = true;
            *attrType = VertexAttribute::UV0;
            return true;
        case UvSet::UV1:
            *attrType = VertexAttribute::UV1;
            return true;
        case UvSet::UNUSED:
            if (GetNumUvSets(uvmap) == 0) {
                *hasUv0 = true;
                *attrType = VertexAttribute::UV0;
                return true;
            }
            return false;
        }
    }
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

#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_REPEAT                         0x2901
#define GL_MIRRORED_REPEAT                0x8370
#define GL_CLAMP_TO_EDGE                  0x812F

inline Soul::GPU::TextureWrap GetWrapMode(cgltf_int wrap) {
    switch (wrap) {
    case GL_REPEAT:
        return Soul::GPU::TextureWrap::REPEAT;
    case GL_MIRRORED_REPEAT:
        return Soul::GPU::TextureWrap::MIRRORED_REPEAT;
    case GL_CLAMP_TO_EDGE:
        return Soul::GPU::TextureWrap::CLAMP_TO_EDGE;
    }
    return Soul::GPU::TextureWrap::REPEAT;
}

inline Soul::GPU::SamplerDesc GetSamplerDesc(const cgltf_sampler& srcSampler) {
    Soul::GPU::SamplerDesc res;
    res.wrapU = GetWrapMode(srcSampler.wrap_s);
    res.wrapV = GetWrapMode(srcSampler.wrap_t);
    switch (srcSampler.min_filter) {
    case GL_NEAREST:
        res.minFilter = Soul::GPU::TextureFilter::NEAREST;
        break;
    case GL_LINEAR:
        res.minFilter = Soul::GPU::TextureFilter::LINEAR;
        break;
    case GL_NEAREST_MIPMAP_NEAREST:
        res.minFilter = Soul::GPU::TextureFilter::NEAREST;
        res.mipmapFilter = Soul::GPU::TextureFilter::NEAREST;
        break;
    case GL_LINEAR_MIPMAP_NEAREST:
        res.minFilter = Soul::GPU::TextureFilter::LINEAR;
        res.mipmapFilter = Soul::GPU::TextureFilter::NEAREST;
        break;
    case GL_NEAREST_MIPMAP_LINEAR:
        res.minFilter = Soul::GPU::TextureFilter::NEAREST;
        res.mipmapFilter = Soul::GPU::TextureFilter::LINEAR;
        break;
    case GL_LINEAR_MIPMAP_LINEAR:
    default:
        res.minFilter = Soul::GPU::TextureFilter::LINEAR;
        res.mipmapFilter = Soul::GPU::TextureFilter::LINEAR;
        break;
    }

    switch (srcSampler.mag_filter) {
    case GL_NEAREST:
        res.magFilter = Soul::GPU::TextureFilter::NEAREST;
        break;
    case GL_LINEAR:
    default:
        res.magFilter = Soul::GPU::TextureFilter::LINEAR;
        break;
    }

    return res;

}

static bool GetTopology(cgltf_primitive_type in,
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

static SoulFila::LightRadiationType GetLightType(const cgltf_light_type light) {
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

static SoulFila::AlphaMode GetAlphaMode(const cgltf_alpha_mode alphaMode) {
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

// Sometimes a glTF bufferview includes unused data at the end (e.g. in skinning.gltf) so we need to
// compute the correct size of the vertex buffer. Filament automatically infers the size of
// driver-level vertex buffers from the attribute data (stride, count, offset) and clients are
// expected to avoid uploading data blobs that exceed this size. Since this information doesn't
// exist in the glTF we need to compute it manually. This is a bit of a cheat, cgltf_calc_size is
// private but its implementation file is available in this cpp file.
uint32_t ComputeBindingSize(const cgltf_accessor& accessor) {
    cgltf_size element_size = cgltf_calc_size(accessor.type, accessor.component_type);
    return uint32_t(accessor.stride * (accessor.count - 1) + element_size);
}

uint32 ComputeBindingOffset(const cgltf_accessor& accessor) {
    return uint32_t(accessor.offset + accessor.buffer_view->offset);
}

template <typename DstType, typename SrcType>
static Soul::GPU::BufferID CreateIndexBuffer(Soul::GPU::System* _gpuSystem, Soul::GPU::BufferDesc& indexBufferDesc, const cgltf_accessor& indices) {
    const byte* bufferData = (const uint8*)indices.buffer_view->buffer->data + ComputeBindingOffset(indices);

    using IndexType = DstType;
    indexBufferDesc.typeSize = sizeof(IndexType);
    indexBufferDesc.typeAlignment = alignof(IndexType);
    indexBufferDesc.count = indices.count;
    uint64 indexStride = indices.stride;

    return _gpuSystem->bufferCreate(indexBufferDesc,
        [bufferData, indexStride](int i, void* data) {
            const auto index = (IndexType*)data;
            (*index) = IndexType(*(SrcType*)(bufferData + indexStride * i));
        }
    );
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
static bool GetElementType(cgltf_type type, cgltf_component_type ctype,
    Soul::GPU::VertexElementType* permitType,
    Soul::GPU::VertexElementType* actualType) {
    using namespace Soul::GPU;
    switch (type) {
    case cgltf_type_scalar:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = VertexElementType::BYTE;
            *actualType = VertexElementType::BYTE;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = VertexElementType::UBYTE;
            *actualType = VertexElementType::UBYTE;
            return true;
        case cgltf_component_type_r_16:
            *permitType = VertexElementType::SHORT;
            *actualType = VertexElementType::SHORT;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = VertexElementType::USHORT;
            *actualType = VertexElementType::USHORT;
            return true;
        case cgltf_component_type_r_32u:
            *permitType = VertexElementType::UINT;
            *actualType = VertexElementType::UINT;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = VertexElementType::FLOAT;
            *actualType = VertexElementType::FLOAT;
            return true;
        default:
            return false;
        }
        break;
    case cgltf_type_vec2:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = VertexElementType::BYTE2;
            *actualType = VertexElementType::BYTE2;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = VertexElementType::UBYTE2;
            *actualType = VertexElementType::UBYTE2;
            return true;
        case cgltf_component_type_r_16:
            *permitType = VertexElementType::SHORT2;
            *actualType = VertexElementType::SHORT2;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = VertexElementType::USHORT2;
            *actualType = VertexElementType::USHORT2;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = VertexElementType::FLOAT2;
            *actualType = VertexElementType::FLOAT2;
            return true;
        default:
            return false;
        }
        break;
    case cgltf_type_vec3:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = VertexElementType::FLOAT3;
            *actualType = VertexElementType::BYTE3;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = VertexElementType::FLOAT3;
            *actualType = VertexElementType::UBYTE3;
            return true;
        case cgltf_component_type_r_16:
            *permitType = VertexElementType::FLOAT3;
            *actualType = VertexElementType::SHORT3;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = VertexElementType::FLOAT3;
            *actualType = VertexElementType::USHORT3;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = VertexElementType::FLOAT3;
            *actualType = VertexElementType::FLOAT3;
            return true;
        default:
            return false;
        }
        break;
    case cgltf_type_vec4:
        switch (ctype) {
        case cgltf_component_type_r_8:
            *permitType = VertexElementType::BYTE4;
            *actualType = VertexElementType::BYTE4;
            return true;
        case cgltf_component_type_r_8u:
            *permitType = VertexElementType::UBYTE4;
            *actualType = VertexElementType::UBYTE4;
            return true;
        case cgltf_component_type_r_16:
            *permitType = VertexElementType::SHORT4;
            *actualType = VertexElementType::SHORT4;
            return true;
        case cgltf_component_type_r_16u:
            *permitType = VertexElementType::USHORT4;
            *actualType = VertexElementType::USHORT4;
            return true;
        case cgltf_component_type_r_32f:
            *permitType = VertexElementType::FLOAT4;
            *actualType = VertexElementType::FLOAT4;
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

inline static bool RequiresConversion(cgltf_type type, cgltf_component_type ctype) {
    Soul::GPU::VertexElementType permitted;
    Soul::GPU::VertexElementType actual;
    bool supported = GetElementType(type, ctype, &permitted, &actual);
    return supported && permitted != actual;
}

static const char* GetNodeName(const cgltf_node* node, const char* defaultNodeName) {
	if (node->name) return node->name;
	if (node->mesh && node->mesh->name) return node->mesh->name;
	if (node->light && node->light->name) return node->light->name;
	if (node->camera && node->camera->name) return node->camera->name;
	return defaultNodeName;
}

static void ComputeUriPath(char* uriPath, const char* gltfPath, const char* uri) {
	cgltf_combine_paths(uriPath, gltfPath, uri);

	// after combining, the tail of the resulting path is a uri; decode_uri converts it into path
	cgltf_decode_uri(uriPath + strlen(uriPath) - strlen(uri));
}

namespace SoulFila {
    static void AddAttributeToPrimitive(Primitive* primitive, VertexAttribute attrType, Soul::GPU::BufferID gpuBuffer,
        Soul::GPU::VertexElementType type, Soul::GPU::VertexElementFlags flags, uint8 attributeStride) {
        primitive->vertexBuffers[primitive->vertexBindingCount] = gpuBuffer;
        primitive->attributes[attrType] = {
            0,
            attributeStride,
            primitive->vertexBindingCount,
            type,
            flags
        };
        primitive->vertexBindingCount++;
        primitive->activeAttribute |= (1 << attrType);
    }
}


void SoulFila::Scene::importFromGLTF(const char* path) {

    char* uriPath = (char*)Soul::Runtime::GetTempAllocator()->allocate(strlen(path) + GLTF_URI_MAX_LENGTH + 1, alignof(char));

    cgltf_options options = {};
    cgltf_data* asset = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path, &asset);
    SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf json");

    result = cgltf_load_buffers(&options, asset, path);
    SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf buffers");

    _textures.clear();
    _textures.reserve(asset->textures_count);

    struct TexCacheKey {
        const cgltf_texture* gltfTexture;
        bool srgb;
        
        inline bool operator==(const TexCacheKey& other) const {
            return (memcmp(this, &other, sizeof(TexCacheKey)) == 0);
        }

        inline bool operator!=(const TexCacheKey& other) const {
            return (memcmp(this, &other, sizeof(TexCacheKey)) != 0);
        }

        uint64 hash() const {
            return hashFNV1((uint8*)(this), sizeof(TexCacheKey));
        }
    };
    using TexCache = Soul::HashMap<TexCacheKey, TextureID>;
    TexCache texCache;
    auto _createTexture = [this, &texCache, uriPath, path]
    (const cgltf_texture& srcTexture, bool srgb)->TextureID {

        TexCacheKey key = { &srcTexture, srgb };
        if (texCache.isExist(key)) {
            return texCache[key];
        }

        const cgltf_buffer_view& bv = *srcTexture.image->buffer_view;
        void** data = &bv ? &bv.buffer->data : nullptr;
        uint32_t totalSize = uint32_t(&bv ? bv.size : 0);

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
            ComputeUriPath(uriPath, path, srcTexture.image->uri);
            int width, height, comp;
            texels = stbi_load(uriPath, &width, &height, &comp, 4);
            SOUL_ASSERT(0, texels != nullptr, "Fail to load texels");
            texDesc.width = width;
            texDesc.height = height;
            totalSize = texDesc.width * texDesc.height * 4;
        }
        texDesc.mipLevels = floorLog2(std::max(texDesc.width, texDesc.height));

        Soul::GPU::SamplerDesc defaultSampler;
        defaultSampler.wrapU = Soul::GPU::TextureWrap::REPEAT;
        defaultSampler.wrapV = Soul::GPU::TextureWrap::REPEAT;
        defaultSampler.minFilter = Soul::GPU::TextureFilter::LINEAR;
        defaultSampler.magFilter = Soul::GPU::TextureFilter::LINEAR;
        defaultSampler.mipmapFilter = Soul::GPU::TextureFilter::LINEAR;
        Soul::GPU::SamplerDesc samplerDesc = srcTexture.sampler != nullptr ? GetSamplerDesc(*srcTexture.sampler) : defaultSampler;

        Texture tex{_gpuSystem->textureCreate(texDesc, texels, totalSize), samplerDesc};

        TextureID texID = TextureID(_textures.add(tex));

        texCache.add(key, texID);
        return texID;
    };


    _materials.clear();
    _materials.reserve(asset->materials_count);
    struct MatCacheKey {
        intptr_t key;
        inline bool operator==(const MatCacheKey& other) {
            return this->key == other.key;
        }
        inline bool operator!=(const MatCacheKey& other) {
            return this->key != other.key;
        }
        uint64 hash() const {
            return uint64(key);
        }
    };
    struct MatCacheEntry {
        MaterialID materialID;
        UvMap uvmap;
    };
    using MatCache = Soul::HashMap<MatCacheKey, MatCacheEntry>;
    MatCache matCache;
    auto createMaterial = [this, &matCache, &_createTexture]
        (const cgltf_material* inputMat, bool vertexColor, UvMap* uvmap) -> MaterialID {
        
        MatCacheKey key = { ((intptr_t)inputMat) ^ (vertexColor ? 1 : 0) };
        
        if (matCache.isExist(key)) {
            const MatCacheEntry& entry = matCache[key];
            *uvmap = entry.uvmap;
            return entry.materialID;
        }

        MaterialID materialID = MaterialID(_materials.add({}));
        Material& dstMaterial = _materials[materialID.id];

        static constexpr cgltf_material kDefaultMat = GetDefaultCGLTFMaterial();
        inputMat = inputMat ? inputMat : &kDefaultMat;

        auto mrConfig = inputMat->pbr_metallic_roughness;
        auto sgConfig = inputMat->pbr_specular_glossiness;
        auto ccConfig = inputMat->clearcoat;
        auto trConfig = inputMat->transmission;
        auto shConfig = inputMat->sheen;

        bool hasTextureTransforms =
            sgConfig.diffuse_texture.has_transform ||
            sgConfig.specular_glossiness_texture.has_transform ||
            mrConfig.base_color_texture.has_transform ||
            mrConfig.metallic_roughness_texture.has_transform ||
            inputMat->normal_texture.has_transform ||
            inputMat->occlusion_texture.has_transform ||
            inputMat->emissive_texture.has_transform ||
            ccConfig.clearcoat_texture.has_transform ||
            ccConfig.clearcoat_roughness_texture.has_transform ||
            ccConfig.clearcoat_normal_texture.has_transform ||
            shConfig.sheen_color_texture.has_transform ||
            shConfig.sheen_roughness_texture.has_transform ||
            trConfig.transmission_texture.has_transform;

        cgltf_texture_view baseColorTexture = mrConfig.base_color_texture;
        cgltf_texture_view metallicRoughnessTexture = mrConfig.metallic_roughness_texture;

        GPUProgramKey programKey = {};
        programKey.doubleSided = !!inputMat->double_sided;
        programKey.unlit = !!inputMat->unlit;
        programKey.hasVertexColors = vertexColor;
        programKey.hasBaseColorTexture = !!baseColorTexture.texture;
        programKey.hasNormalTexture = !!inputMat->normal_texture.texture;
        programKey.hasOcclusionTexture = !!inputMat->occlusion_texture.texture;
        programKey.hasEmissiveTexture = !!inputMat->emissive_texture.texture;
        programKey.enableDiagnostics = true;
        programKey.baseColorUV = (uint8_t)baseColorTexture.texcoord;
        programKey.hasClearCoatTexture = !!ccConfig.clearcoat_texture.texture;
        programKey.clearCoatUV = (uint8_t)ccConfig.clearcoat_texture.texcoord;
        programKey.hasClearCoatRoughnessTexture = !!ccConfig.clearcoat_roughness_texture.texture;
        programKey.clearCoatRoughnessUV = (uint8_t)ccConfig.clearcoat_roughness_texture.texcoord;
        programKey.hasClearCoatNormalTexture = !!ccConfig.clearcoat_normal_texture.texture;
        programKey.clearCoatNormalUV = (uint8_t)ccConfig.clearcoat_normal_texture.texcoord;
        programKey.hasClearCoat = !!inputMat->has_clearcoat;
        programKey.hasTransmission = !!inputMat->has_transmission;
        programKey.hasTextureTransforms = hasTextureTransforms;
        programKey.emissiveUV = (uint8_t)inputMat->emissive_texture.texcoord;
        programKey.aoUV = (uint8_t)inputMat->occlusion_texture.texcoord;
        programKey.normalUV = (uint8_t)inputMat->normal_texture.texcoord;
        programKey.hasTransmissionTexture = !!trConfig.transmission_texture.texture;
        programKey.transmissionUV = (uint8_t)trConfig.transmission_texture.texcoord;
        programKey.hasSheenColorTexture = !!shConfig.sheen_color_texture.texture;
        programKey.sheenColorUV = (uint8_t)shConfig.sheen_color_texture.texcoord;
        programKey.hasSheenRoughnessTexture = !!shConfig.sheen_roughness_texture.texture;
        programKey.sheenRoughnessUV = (uint8_t)shConfig.sheen_roughness_texture.texcoord;
        programKey.hasSheen = !!inputMat->has_sheen;

        if (inputMat->has_pbr_specular_glossiness) {
            programKey.useSpecularGlossiness = true;
            if (sgConfig.diffuse_texture.texture) {
                baseColorTexture = sgConfig.diffuse_texture;
                programKey.hasBaseColorTexture = true;
                programKey.baseColorUV = (uint8_t)baseColorTexture.texcoord;
            }
            if (sgConfig.specular_glossiness_texture.texture) {
                metallicRoughnessTexture = sgConfig.specular_glossiness_texture;
                programKey.hasSpecularGlossinessTexture = true;
                programKey.specularGlossinessUV = (uint8_t)metallicRoughnessTexture.texcoord;
            }
        }
        else {
            programKey.hasMetallicRoughnessTexture = !!metallicRoughnessTexture.texture;
            programKey.metallicRoughnessUV = (uint8_t)metallicRoughnessTexture.texcoord;
        }

        switch (inputMat->alpha_mode) {
        case cgltf_alpha_mode_opaque:
            programKey.alphaMode = AlphaMode::OPAQUE;
            break;
        case cgltf_alpha_mode_mask:
            programKey.alphaMode = AlphaMode::MASK;
            break;
        case cgltf_alpha_mode_blend:
            programKey.alphaMode = AlphaMode::BLEND;
            break;
        }

        ConstrainGPUProgramKey(&programKey, uvmap);
        dstMaterial.programSetID = _programRegistry->createProgramSet(programKey);
        
        MaterialUBO& matBuf = dstMaterial.buffer;
        MaterialTextures& matTexs = dstMaterial.textures;

        matBuf.baseColorFactor = Soul::Vec4f(mrConfig.base_color_factor);
        matBuf.emissiveFactor = Soul::Vec3f(inputMat->emissive_factor);
        matBuf.metallicFactor = mrConfig.metallic_factor;
        matBuf.roughnessFactor = mrConfig.roughness_factor;

        if (programKey.useSpecularGlossiness) {
            matBuf.baseColorFactor = Soul::Vec4f(sgConfig.diffuse_factor);
            matBuf.specularFactor = Soul::Vec3f(sgConfig.specular_factor);
            matBuf.roughnessFactor = mrConfig.roughness_factor;
        }

        if (programKey.hasBaseColorTexture) {
            matTexs.baseColorTexture = _createTexture(*baseColorTexture.texture, true);
            if (programKey.hasTextureTransforms) {
                const cgltf_texture_transform& uvt = baseColorTexture.transform;
                matBuf.baseColorUvMatrix = MatrixFromUVTransform(uvt);
            }
        }

        if (programKey.hasMetallicRoughnessTexture) {
            bool srgb = inputMat->has_pbr_specular_glossiness;
            matTexs.metallicRoughnessTexture = _createTexture(*metallicRoughnessTexture.texture, srgb);
            if (programKey.hasTextureTransforms) {
                const cgltf_texture_transform& uvt = metallicRoughnessTexture.transform;
                matBuf.metallicRoughnessUvMatrix = MatrixFromUVTransform(uvt);
            }
        }

        if (programKey.hasNormalTexture) {
            matTexs.normalTexture = _createTexture(*inputMat->normal_texture.texture, false);
            if (programKey.hasTextureTransforms) {
                const cgltf_texture_transform& uvt = inputMat->normal_texture.transform;
                matBuf.normalUvMatrix = MatrixFromUVTransform(uvt);
            }
        }
        else {
            matBuf.normalScale = 1.0f;
        }

        if (programKey.hasOcclusionTexture) {
            matTexs.occlusionTexture = _createTexture(*inputMat->occlusion_texture.texture, false);
            if (programKey.hasTextureTransforms) {
                matBuf.occlusionUvMatrix = MatrixFromUVTransform(inputMat->occlusion_texture.transform);
            }
            matBuf.aoStrength = inputMat->occlusion_texture.scale;
        }
        else {
            matBuf.aoStrength = 1.0f;
        }

        if (programKey.hasEmissiveTexture) {
            matTexs.emissiveTexture = _createTexture(*inputMat->emissive_texture.texture, true);
            if (programKey.hasTextureTransforms) {
                matBuf.emissiveUvMatrix = MatrixFromUVTransform(inputMat->emissive_texture.transform);
            }
        }

        if (programKey.hasClearCoat) {
            matBuf.clearCoatFactor = ccConfig.clearcoat_factor;
            matBuf.clearCoatRoughnessFactor = ccConfig.clearcoat_roughness_factor;

            if (programKey.hasClearCoatTexture) {
                matTexs.clearCoatTexture = _createTexture(*ccConfig.clearcoat_texture.texture, false);
                if (programKey.hasTextureTransforms) {
                    matBuf.clearCoatUvMatrix = MatrixFromUVTransform(ccConfig.clearcoat_texture.transform);
                }
            }

            if (programKey.hasClearCoatRoughnessTexture) {
                matTexs.clearCoatRoughnessTexture = _createTexture(*ccConfig.clearcoat_roughness_texture.texture, false);
                if (programKey.hasTextureTransforms) {
                    matBuf.clearCoatRoughnessMatrix = MatrixFromUVTransform(ccConfig.clearcoat_roughness_texture.transform);
                }
            }

            if (programKey.hasClearCoatNormalTexture) {
                matTexs.clearCoatNormalTexture = _createTexture(*ccConfig.clearcoat_normal_texture.texture, false);
                if (programKey.hasClearCoatNormalTexture) {
                    matBuf.clearCoatNormalUvMatrix = MatrixFromUVTransform(ccConfig.clearcoat_normal_texture.transform);
                }
                matBuf.clearCoatNormalScale = ccConfig.clearcoat_normal_texture.scale;
            }
        }

        if (programKey.hasSheen) {
            matBuf.sheenColorFactor = Soul::Vec3f(shConfig.sheen_color_factor);
            matBuf.sheenRoughnessFactor = shConfig.sheen_roughness_factor;

            if (programKey.hasSheenColorTexture) {
                matTexs.sheenColorTexture = _createTexture(*shConfig.sheen_color_texture.texture, true);
                if (programKey.hasTextureTransforms) {
                    matBuf.sheenColorUvMatrix = MatrixFromUVTransform(shConfig.sheen_color_texture.transform);
                }
            }

            if (programKey.hasSheenRoughnessTexture) {
                matTexs.sheenRoughnessTexture = _createTexture(*shConfig.sheen_roughness_texture.texture, false);
                if (programKey.hasTextureTransforms) {
                    matBuf.sheenRoughnessUvMatrix = MatrixFromUVTransform(shConfig.sheen_roughness_texture.transform);
                }
            }
        }

        if (programKey.hasTransmission) {
            matBuf.transmissionFactor = trConfig.transmission_factor;
            if (programKey.hasTransmissionTexture) {
                matTexs.transmissionTexture = _createTexture(*trConfig.transmission_texture.texture, false);
                if (programKey.hasTextureTransforms) {
                    matBuf.transmissionUvMatrix = MatrixFromUVTransform(trConfig.transmission_texture.transform);
                }
            }
        }

        matCache.add(key, { materialID, *uvmap });

        return materialID;
    };
    
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
	        Runtime::ScopeAllocator<> primitiveScopeAllocator("Loading Attribute Allocation");

            TangentFrameComputeInput qtangentComputeInput;

            const cgltf_primitive& srcPrimitive = srcMesh.primitives[primitiveIndex];

            if (srcPrimitive.has_draco_mesh_compression) {
                SOUL_NOT_IMPLEMENTED();
            }

            UvMap uvmap{};
            Primitive& dstPrimitive = dstMesh.primitives[primitiveIndex];
            bool hasVertexColor = PrimitiveHasVertexColor(&srcPrimitive);
            dstPrimitive.materialID = createMaterial(srcPrimitive.material, hasVertexColor, &uvmap);

            bool getTopologySuccess = GetTopology(srcPrimitive.type, &dstPrimitive.topology);
            SOUL_ASSERT(0, getTopologySuccess, "");

            // build index buffer
            if (srcPrimitive.indices) {

                const cgltf_accessor& indices = *srcPrimitive.indices;

                switch (srcPrimitive.indices->component_type) {
                case cgltf_component_type_r_8u: {
                    dstPrimitive.indexBuffer = CreateIndexBuffer<uint16, uint8>(_gpuSystem, indexBufferDesc, indices);
                    break;
                }
                case cgltf_component_type_r_16u: {
                    dstPrimitive.indexBuffer = CreateIndexBuffer<uint16, uint16>(_gpuSystem, indexBufferDesc, indices);
                    break;
                }
                case cgltf_component_type_r_32u: {
                    dstPrimitive.indexBuffer = CreateIndexBuffer<uint32, uint32>(_gpuSystem, indexBufferDesc, indices);
                    break;
                }
                case cgltf_component_type_r_8:
                case cgltf_component_type_r_16:
                case cgltf_component_type_r_32f:
                case cgltf_component_type_invalid:
                    SOUL_NOT_IMPLEMENTED();
                }

                qtangentComputeInput.triangles32 = (Vec3ui32*)primitiveScopeAllocator.allocate(indices.count * sizeof(uint32), alignof(Vec3ui32));
                for (size_t tri = 0, indexIdx = 0; tri < indices.count / 3; ++tri) {
                    auto& triangle = qtangentComputeInput.triangles32[tri];
                    triangle.x = Soul::Util::Cast<uint32>(cgltf_accessor_read_index(&indices, indexIdx++));
                    triangle.y = Soul::Util::Cast<uint32>(cgltf_accessor_read_index(&indices, indexIdx++));
                    triangle.z = Soul::Util::Cast<uint32>(cgltf_accessor_read_index(&indices, indexIdx++));
                }

            }
            else if (srcPrimitive.attributes_count > 0) {

                using IndexType = uint32;
                indexBufferDesc.count = srcPrimitive.attributes[0].data->count;

                const uint64 indexDataSize = indexBufferDesc.count * sizeof(IndexType);
                auto indexData = (IndexType*)primitiveScopeAllocator.allocate(indexDataSize, alignof(IndexType));
                for (uint64 i = 0; i < indexBufferDesc.count; ++i) {
                    indexData[i] = i;
                }
                indexBufferDesc.typeSize = sizeof(IndexType);
                indexBufferDesc.typeAlignment = alignof(IndexType);

                dstPrimitive.indexBuffer = _gpuSystem->bufferCreate(indexBufferDesc,
                    [](int i, void* data) {
                        const auto index = (IndexType*)data;
                        (*index) = IndexType(i);
                    }
                );

                qtangentComputeInput.triangles32 = (Vec3ui32*)indexData;

            }

            qtangentComputeInput.vertexCount = srcPrimitive.attributes[0].data->count;
            qtangentComputeInput.triangleCount = indexBufferDesc.count / 3;
            bool hasNormal = false;
            bool hasUv0 = false;
            for (cgltf_size aindex = 0; aindex < srcPrimitive.attributes_count; aindex++) {
                const cgltf_attribute& srcAttribute = srcPrimitive.attributes[aindex];
                const cgltf_accessor& accessor = *srcAttribute.data;

                if (srcAttribute.type == cgltf_attribute_type_weights) {
                    auto normalize = [](const cgltf_accessor* data) {
                        if (data->type != cgltf_type_vec4 || data->component_type != cgltf_component_type_r_32f) {
                            SOUL_LOG_ERROR("Attribute type is not supported");
                            SOUL_NOT_IMPLEMENTED();
                        }
                        auto bytes = (uint8*)data->buffer_view->buffer->data;
                        bytes += data->offset + data->buffer_view->offset;
                        for (cgltf_size i = 0, n = data->count; i < n; ++i, bytes += data->stride) {
                            auto weights = (Vec4f*)bytes;
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
                    byte* generated = (byte*)primitiveScopeAllocator.allocate(numBytes, alignof(float));
                    cgltf_accessor_unpack_floats(&accessor, (float*)generated, numFloats);

                    attributeData = generated;
                    attributeDataCount = accessor.count;
                    attributeTypeSize = cgltf_num_components(accessor.type) * sizeof(float);
                    attributeTypeAlignment = sizeof(float);
                    attributeStride = attributeTypeSize;
                }
                else {
                    const byte* bufferData = (const byte*)accessor.buffer_view->buffer->data;
                    attributeData = bufferData + ComputeBindingOffset(accessor);
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
                    hasNormal = true;
                    continue;
                }

                if (srcAttribute.type == cgltf_attribute_type_texcoord && srcAttribute.index == 0) {
                    cgltf_size numFloats = accessor.count * cgltf_num_components(accessor.type);
                    cgltf_size numBytes = sizeof(float) * numFloats;
                    Vec2f* generated = (Vec2f*)primitiveScopeAllocator.allocate(numBytes, alignof(float));
                    cgltf_accessor_unpack_floats(&accessor, (float*)generated, numFloats);
                    qtangentComputeInput.uvs = (Vec2f*)generated;
                }

                if (srcAttribute.type == cgltf_attribute_type_position) {
                    SOUL_ASSERT(0, sizeof(Vec3f) == attributeStride, "");
                    qtangentComputeInput.positions = (Vec3f*)attributeData;
                    dstPrimitive.aabb = aabbCombine(dstPrimitive.aabb, AABB(Vec3f(accessor.min), Vec3f(accessor.max)));
                }

                GPU::BufferDesc gpuDesc;
                gpuDesc.count = attributeDataCount;
                gpuDesc.typeSize = attributeTypeSize;
                gpuDesc.typeAlignment = attributeTypeAlignment;
                gpuDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
                gpuDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
                GPU::BufferID attributeGPUBuffer = _gpuSystem->bufferCreate(gpuDesc,
                    [attributeData, attributeTypeSize, attributeStride](int index, void* data)
                    {
                        uint64 offset = index * attributeStride;
                        memcpy(data, attributeData + offset, attributeTypeSize);
                    });

                VertexAttribute attrType;
                bool attrSupported = GetVertexAttrType(srcAttribute.type, srcAttribute.index, uvmap, &attrType, &hasUv0);
                SOUL_ASSERT(0, attrSupported, "Attribute type not supported");
                Soul::GPU::VertexElementType permitted;
                Soul::GPU::VertexElementType actual;
                GetElementType(accessor.type, accessor.component_type, &permitted, &actual);

                Soul::GPU::VertexElementFlags flags;
                if (accessor.normalized) {
                    flags |= GPU::VERTEX_ELEMENT_NORMALIZED;
                }
                if (attrType == VertexAttribute::BONE_INDICES) flags |= GPU::VERTEX_ELEMENT_INTEGER_TARGET;

                AddAttributeToPrimitive(&dstPrimitive, attrType, attributeGPUBuffer, actual, flags, attributeTypeSize);

            }

            uint64 qtangentBufferSize = qtangentComputeInput.vertexCount * sizeof(Quaternionf);
            Quaternionf* qtangents = (Quaternionf*)primitiveScopeAllocator.allocate(qtangentBufferSize, alignof(Quaternionf));
            if (hasNormal || srcPrimitive.material && !srcPrimitive.material->unlit) {
                if (ComputeTangentFrame(qtangentComputeInput, qtangents)) {
                    GPU::BufferDesc qtangentsBufferDesc = {
                         qtangentComputeInput.vertexCount,
                         sizeof(Quaternionf),
                         alignof(Quaternionf),
                         GPU::BUFFER_USAGE_VERTEX_BIT,
                         GPU::QUEUE_GRAPHIC_BIT
                    };
                    Soul::GPU::BufferID qtangentsGPUBUffer = _gpuSystem->bufferCreate(qtangentsBufferDesc, qtangents);
                    AddAttributeToPrimitive(&dstPrimitive, VertexAttribute::TANGENTS, qtangentsGPUBUffer, GPU::VertexElementType::SHORT4, GPU::VERTEX_ELEMENT_NORMALIZED, sizeof(Quaternionf));
                }
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
                    byte* generated = (byte*)primitiveScopeAllocator.allocate(numBytes, alignof(float));
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
                            [attributeData, attributeTypeSize, attributeStride](int index, void* data)
                            {
                                uint64 offset = index * attributeStride;
                                memcpy(data, attributeData + offset, attributeTypeSize);
                            });

                        Soul::GPU::VertexElementType permitted;
                        Soul::GPU::VertexElementType actual;
                        GetElementType(accessor.type, accessor.component_type, &permitted, &actual);

                        Soul::GPU::VertexElementFlags flags;
                        if (accessor.normalized) {
                            flags |= GPU::VERTEX_ELEMENT_NORMALIZED;
                        }

                        AddAttributeToPrimitive(&dstPrimitive, attrType, attributeGPUBuffer, actual, flags, attributeTypeSize);
                        
                        dstPrimitive.aabb = aabbCombine(dstPrimitive.aabb, AABB(Vec3f(accessor.min), Vec3f(accessor.max)));
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

                uint64 qtangentBufferSize = qtangentComputeInput.vertexCount * sizeof(Quaternionf);
                auto qtangents = (Quaternionf*)primitiveScopeAllocator.allocate(qtangentBufferSize, alignof(Quaternionf));
                if (ComputeTangentFrame(qtangentComputeInput, qtangents)) {
                    GPU::BufferDesc qtangentsBufferDesc = {
                         qtangentComputeInput.vertexCount,
                         sizeof(Quaternionf),
                         alignof(Quaternionf),
                         GPU::BUFFER_USAGE_VERTEX_BIT,
                         GPU::QUEUE_GRAPHIC_BIT
                    };
                    Soul::GPU::BufferID qtangentsGPUBUffer = _gpuSystem->bufferCreate(qtangentsBufferDesc, qtangents);
                    AddAttributeToPrimitive(&dstPrimitive, (VertexAttribute)(baseTangentsAttr + targetIndex), qtangentsGPUBUffer, GPU::VertexElementType::SHORT4, GPU::VERTEX_ELEMENT_NORMALIZED ,sizeof(Quaternionf));
                }


            }

            dstMesh.aabb = aabbCombine(dstMesh.aabb, dstPrimitive.aabb);

        }
	}

	_rootEntity = _registry.create();

	const cgltf_scene* scene = asset->scene ? asset->scene : asset->scenes;
	if (!scene) {
		return;
	}

	// create root entity
	_rootEntity = _registry.create();
    _registry.emplace<NameComponent>(_rootEntity, "Root");
	_registry.emplace<TransformComponent>(_rootEntity, mat4Identity(), mat4Identity(), _rootEntity, ENTITY_ID_NULL, ENTITY_ID_NULL, ENTITY_ID_NULL);

    Soul::HashMap<_CGLTFNodeKey, EntityID> nodeMap;
    for (cgltf_size i = 0, len = asset->nodes_count; i < len; ++i) {
        cgltf_node* nodes = asset->nodes;
        _createEntity(&nodeMap, asset, &nodes[i], _rootEntity);
    }

    _animations.clear();
    _animations.resize(asset->animations_count);
    for (cgltf_size animIdx = 0; animIdx < asset->animations_count; animIdx++) {
        const cgltf_animation& srcAnim = asset->animations[animIdx];
        Animation& dstAnim = _animations[animIdx];
        dstAnim.name = srcAnim.name == nullptr ? "Unnamed" : srcAnim.name;

        dstAnim.samplers.resize(srcAnim.samplers_count);
        for (cgltf_size samplerIdx = 0; samplerIdx < srcAnim.samplers_count; samplerIdx++) {
            const cgltf_animation_sampler& srcSampler = srcAnim.samplers[samplerIdx];
            AnimationSampler& dstSampler = dstAnim.samplers[samplerIdx];
            
            const cgltf_accessor& timelineAccessor = *srcSampler.input;
            auto timelineBlob = (const uint8*)timelineAccessor.buffer_view->buffer->data;
            auto timelineFloats = (const float*)(timelineBlob + timelineAccessor.offset +
                timelineAccessor.buffer_view->offset);
            
            dstSampler.times.resize(timelineAccessor.count);
            memcpy(dstSampler.times.data(), timelineFloats, timelineAccessor.count * sizeof(float));

            const cgltf_accessor& valuesAccessor = *srcSampler.output;
            switch (valuesAccessor.type) {
            case cgltf_type_scalar:
                dstSampler.values.resize(valuesAccessor.count);
                cgltf_accessor_unpack_floats(srcSampler.output, dstSampler.values.data(), valuesAccessor.count);
                break;
            case cgltf_type_vec3:
                dstSampler.values.resize(valuesAccessor.count * 3);
                cgltf_accessor_unpack_floats(srcSampler.output, dstSampler.values.data(), valuesAccessor.count * 3);
                break;
            case cgltf_type_vec4:
                dstSampler.values.resize(valuesAccessor.count * 4);
                cgltf_accessor_unpack_floats(srcSampler.output, dstSampler.values.data(), valuesAccessor.count * 4);
                break;
            default:
                SOUL_LOG_WARN("Unknown animation type.");
                return;
            }

            switch (srcSampler.interpolation) {
            case cgltf_interpolation_type_linear:
                dstSampler.interpolation = AnimationSampler::LINEAR;
                break;
            case cgltf_interpolation_type_step:
                dstSampler.interpolation = AnimationSampler::STEP;
                break;
            case cgltf_interpolation_type_cubic_spline:
                dstSampler.interpolation = AnimationSampler::CUBIC;
                break;
            }
        }

        dstAnim.duration = 0;
        dstAnim.channels.resize(srcAnim.channels_count);
        for (cgltf_size channelIdx = 0; channelIdx < srcAnim.channels_count; channelIdx++) {
            const cgltf_animation_channel& srcChannel = srcAnim.channels[channelIdx];
            AnimationChannel& dstChannel = dstAnim.channels[channelIdx];
            dstChannel.samplerIdx = Soul::Util::Cast<uint32>(srcChannel.sampler - srcAnim.samplers);
            dstChannel.entity = nodeMap[srcChannel.target_node];
            switch (srcChannel.target_path) {
            case cgltf_animation_path_type_translation:
                dstChannel.transformType = AnimationChannel::TRANSLATION;
                break;
            case cgltf_animation_path_type_rotation:
                dstChannel.transformType = AnimationChannel::ROTATION;
                break;
            case cgltf_animation_path_type_scale:
                dstChannel.transformType = AnimationChannel::SCALE;
                break;
            case cgltf_animation_path_type_weights:
                dstChannel.transformType = AnimationChannel::WEIGHTS;
                break;
            case cgltf_animation_path_type_invalid:
                SOUL_LOG_WARN("Unsupported channel path.");
                break;
            }
            float channelDuration = dstAnim.samplers[dstChannel.samplerIdx].times.back();
            dstAnim.duration = std::max(dstAnim.duration, channelDuration);
        }

    }

    auto importSkins = [this](const cgltf_data* asset, const Soul::HashMap<_CGLTFNodeKey, EntityID>& nodeMap) {
        _skins.resize(asset->skins_count);
        for (cgltf_size skinIdx = 0; skinIdx < asset->skins_count; skinIdx++) {
            const cgltf_skin& srcSkin = asset->skins[skinIdx];
            Skin& dstSkin = _skins[skinIdx];
            if (srcSkin.name) {
                dstSkin.name = srcSkin.name;
            }

            dstSkin.invBindMatrices.resize(srcSkin.joints_count);
            dstSkin.joints.resize(srcSkin.joints_count);
            dstSkin.bones.resize(srcSkin.joints_count);

            const cgltf_accessor* srcMatrices = srcSkin.inverse_bind_matrices;

            if (srcMatrices) {
                auto dstMatrices = (uint8_t*)dstSkin.invBindMatrices.data();
                uint8_t* bytes = (uint8_t*)srcMatrices->buffer_view->buffer->data;
                if (!bytes) {
                    SOUL_NOT_IMPLEMENTED();
                }
                auto srcBuffer = (void*)(bytes + srcMatrices->offset + srcMatrices->buffer_view->offset);
                memcpy(dstMatrices, srcBuffer, srcSkin.joints_count * sizeof(Soul::Mat4f));
                for (Soul::Mat4f& matrix : dstSkin.invBindMatrices) {
                    matrix = mat4Transpose(matrix);
                }
            }
            else {
                for (Soul::Mat4f& matrix : dstSkin.invBindMatrices) {
                    matrix = mat4Identity();
                }
            }

            for (cgltf_size jointIdx = 0; jointIdx < srcSkin.joints_count; jointIdx++) {
                dstSkin.joints[jointIdx] = nodeMap[srcSkin.joints[jointIdx]];
            }
        }
    };
    importSkins(asset, nodeMap);

    auto view = _registry.view<TransformComponent, RenderComponent>();
    for (auto entity : view) {
        auto [transform, renderComp] = view.get<TransformComponent, RenderComponent>(entity);
        const Mesh& mesh = _meshes[renderComp.meshID.id];
        _boundingBox = aabbCombine(_boundingBox, aabbTransform(mesh.aabb, transform.world));
    }

    auto _fitIntoUnitCube = [](const AABB& bounds, float zoffset) -> Soul::Mat4f {
        auto minpt = bounds.min;
        auto maxpt = bounds.max;
        float maxExtent;
        maxExtent = std::max(maxpt.x - minpt.x, maxpt.y - minpt.y);
        maxExtent = std::max(maxExtent, maxpt.z - minpt.z);
        float scaleFactor = 2.0f / maxExtent;
        Vec3f center = (minpt + maxpt) / 2.0f;
        center.z += zoffset / scaleFactor;
        return mat4Scale(Vec3f(scaleFactor, scaleFactor, scaleFactor)) * mat4Translate(-center);
    };

    Mat4f fitTransform = _fitIntoUnitCube(_boundingBox, 4);
    TransformComponent& rootTransform = _registry.get<TransformComponent>(_rootEntity);
    rootTransform.local = fitTransform;
    _updateWorldTransform(_rootEntity);

    // create default camera
    EntityID defaultCamera = _registry.create();

    Soul::Mat4f defaultCameraModelMat = Soul::mat4Inverse(Soul::mat4View(Soul::Vec3f(0, 0, 1.0), Soul::Vec3f(0, 0, -4), Soul::Vec3f(0, 1, 0)));
    _registry.emplace<TransformComponent>(defaultCamera, defaultCameraModelMat, defaultCameraModelMat, _rootEntity, ENTITY_ID_NULL, ENTITY_ID_NULL, ENTITY_ID_NULL);
    CameraComponent& cameraComp = _registry.emplace<CameraComponent>(defaultCamera);
    Vec2ui32 viewport = getViewport();
    cameraComp.setLensProjection(28.0f, float(viewport.x) / float(viewport.y), 0.1f, 100.0f);
    _registry.emplace<NameComponent>(defaultCamera, "Default camera");
    _setActiveCamera(defaultCamera);

	cgltf_free(asset);
}

void SoulFila::Scene::_createEntity(Soul::HashMap<_CGLTFNodeKey, EntityID>* nodeMap, const cgltf_data* asset, const cgltf_node* node, EntityID parent) {
	EntityID entity = _registry.create();
    if (nodeMap->isExist(node)) {
        return;
    }
    nodeMap->add(node, entity);

	Soul::Mat4f localTransform;
	if (node->has_matrix) {
        localTransform = mat4Transpose(mat4(node->matrix));
	}
	else {
		const Soul::Vec3f translation(node->translation);
		const Soul::Vec3f scale(node->scale);
		const Soul::Quaternionf rotation(node->rotation);
		localTransform = Soul::mat4Transform(Soul::Transformf{ translation, scale, rotation });
	}

	TransformComponent& parentTransform = _registry.get<TransformComponent>(parent);
	const Mat4f worldTransform = parentTransform.world * localTransform;
    EntityID nextEntity = parentTransform.firstChild;
    parentTransform.firstChild = entity;
    if (nextEntity != ENTITY_ID_NULL) {
        TransformComponent& nextTransform = _registry.get<TransformComponent>(nextEntity);
        nextTransform.prev = entity;
    }

    _registry.emplace<NameComponent>(entity, node->name == nullptr ? "Unnamed" : node->name);
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
        _createEntity(nodeMap, asset, node->children[i], entity);
    }

}

void SoulFila::Scene::_createRenderable(const cgltf_data* asset, const cgltf_node* node, EntityID entity) {

    Visibility visibility = {};
    visibility.priority = 0x4;
    visibility.castShadows = true;
    visibility.receiveShadows = true;
    visibility.culling = true;

    SOUL_ASSERT(0, node->mesh != nullptr, "");

    cgltf_mesh& srcMesh = *node->mesh;
    auto meshID = MeshID(node->mesh - asset->meshes);

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

    SkinID skinID = node->skin != nullptr ? SkinID(node->skin - asset->skins) : SkinID();
    visibility.skinning = skinID.isNull() ? false : true;

    _registry.emplace<RenderComponent>(entity, visibility, meshID, skinID, morphWeights);
}

void SoulFila::Scene::_createLight(const cgltf_data* asset, const cgltf_node* node, EntityID entity) {
    
    SOUL_ASSERT(0, node->light != nullptr, "");
    const cgltf_light& light = *node->light;
    
    LightType lightType = {};
    lightType.type = GetLightType(light.type);
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
	    const float innerClamped = std::min(std::abs(light.spot_inner_cone_angle), Fconst::PI_2);
        float outerClamped = std::min(std::abs(light.spot_outer_cone_angle), Fconst::PI_2);

        // outer must always be bigger than inner
        outerClamped = std::max(innerClamped, outerClamped);

        const float cosOuter = std::cos(outerClamped);
        const float cosInner = std::cos(innerClamped);
        const float cosOuterSquared = cosOuter * cosOuter;
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
            luminousIntensity = luminousPower * Fconst::ONE_OVER_PI * 0.25f;
            break;
        case LightRadiationType::FOCUSED_SPOT: {
            const float cosOuter = std::sqrt(spotParams.cosOuterSquared);
            // intensity specified directly in candela, no conversion needed
            luminousIntensity = luminousPower;
            // lp = li * (2 * pi * (1 -  cos(cone_outer / 2)))
            luminousPower = luminousIntensity * (Fconst::TAU * (1.0f - cosOuter));
            spotParams.luminousPower = luminousPower;
            break;
        }
        case LightRadiationType::SPOT:
            luminousIntensity = luminousPower;
            break;
        default:
            SOUL_NOT_IMPLEMENTED();
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
        const double far = srcPerspective.zfar > 0.0f ? srcPerspective.zfar : 100000000.0f;
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

void SoulFila::Scene::_renderEntityTreeNode(EntityID entityID) {
    if (entityID == ENTITY_ID_NULL) return;
    const auto& [transformComp, nameComp] = _registry.get<TransformComponent, NameComponent>(entityID);
    ImGuiTreeNodeFlags flags = SCENE_TREE_FLAGS;
    if (_selectedEntity == entityID) flags |= ImGuiTreeNodeFlags_Selected;
    if (transformComp.firstChild == ENTITY_ID_NULL) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    const bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entityID, flags, "%s", nameComp.name.data());
    if (ImGui::IsItemClicked()) {
        _selectedEntity = entityID;
    }
    if (nodeOpen && transformComp.firstChild != ENTITY_ID_NULL) {
        _renderEntityTreeNode(transformComp.firstChild);
        ImGui::TreePop();
    }
    _renderEntityTreeNode(transformComp.next);
}

void SoulFila::Scene::renderPanels() {

    if (ImGui::Begin("Scene configuration")) {
        auto renderCameraList = [this]() {
            auto view = _registry.view<CameraComponent, NameComponent>();
            char* comboLabel = _activeCamera == ENTITY_ID_NULL ? "No camera" : _registry.get<NameComponent>(_activeCamera).name.data();
            if (ImGui::BeginCombo("Camera List", comboLabel, ImGuiComboFlags_PopupAlignLeft))
            {
                for (auto entity : view) {
                    const bool isSelected = (_activeCamera == entity);
                    if (ImGui::Selectable(view.get<NameComponent>(entity).name.data(), isSelected)) {
                        _setActiveCamera(entity);
                    }

                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        };
        renderCameraList();

        auto renderAnimationList = [this]() {
            static const char* noActiveAnimationLabel = "No active animation";
            const char* comboLabel = _activeAnimation.isNull() ? noActiveAnimationLabel : _animations[_activeAnimation.id].name.data();
            if (ImGui::BeginCombo("Animation List", comboLabel, 0)) {
                for (uint64 animIdx = 0; animIdx < _animations.size(); animIdx++) {
                    const bool isSelected = (_activeAnimation.id == animIdx);
                    if (ImGui::Selectable(_animations[animIdx].name.data(), isSelected)) {
                        _setActiveAnimation(AnimationID(animIdx));
                    }

                    if (isSelected) ImGui::SetItemDefaultFocus();
                }

                const bool isSelected = (_activeAnimation.isNull());
                if (ImGui::Selectable(noActiveAnimationLabel, isSelected)) {
                    _setActiveAnimation(AnimationID());
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
                ImGui::EndCombo();
            }
        };
        renderAnimationList();
    }
    ImGui::End();

    if (ImGui::Begin("Scene Tree")) {
        ItemRowsBackground();
        _renderEntityTreeNode(_rootEntity);
    }
    ImGui::End();

    if (ImGui::Begin("Entity Components")) {
        if (_selectedEntity != ENTITY_ID_NULL) {
            auto [transformComp, nameComp] = _registry.get<TransformComponent, NameComponent>(_selectedEntity);
            nameComp.renderUI();
            transformComp.renderUI();
        }
    }
    ImGui::End();
}

void SoulFila::Scene::_setActiveAnimation(AnimationID animationID) {
    _activeAnimation = animationID;
    _animationDelta = 0;
    _resetAnimation = true;
    if (animationID.isNull()) return;
    _channelCursors.resize(_animations[animationID.id].channels.size());
    for (uint64& cursor : _channelCursors) {
        cursor = 0;
    }
}

void SoulFila::Scene::_setActiveCamera(EntityID camera) {
    _activeCamera = camera;
    TransformComponent& transformComp = _registry.get<TransformComponent>(camera);
    Soul::Vec3f cameraPosition = Soul::Vec3f(transformComp.world.elem[0][3], transformComp.world.elem[1][3], transformComp.world.elem[2][3]);
    Soul::Vec3f cameraForward = unit(Soul::Vec3f(transformComp.world.elem[0][2], transformComp.world.elem[1][2], transformComp.world.elem[2][2]) * -1.0f);
    Soul::Vec3f cameraUp = unit(Soul::Vec3f(transformComp.world.elem[0][1], transformComp.world.elem[1][1], transformComp.world.elem[2][1]));
    Soul::Plane groundPlane(Soul::Vec3f(0, 1, 0), Soul::Vec3f(0, 0, 0));
    Soul::Ray cameraRay(cameraPosition, cameraForward);
	const auto [intersectPoint, isIntersect] = IntersectRayPlane(cameraRay, groundPlane);
    Soul::Vec3f cameraTarget = intersectPoint;
    if (!isIntersect)
    {
	    cameraTarget = cameraPosition + 5.0f * cameraForward;
    }
    _cameraMan.setCamera(cameraPosition, cameraTarget, cameraUp);
}

bool SoulFila::Scene::update(const Demo::Input& input) {

    auto handleViewPopup = [this](const Demo::Input& input) {
        if (input.keysDown[Demo::Input::KEY_GRAVE_ACCENT]) {
            ImGui::OpenPopup("##ViewPopup");
        }
        enum class CameraViewDirection : uint8 {
            RIGHT,
            BOTTOM,
            FRONT,
            LEFT,
            BACK,
            TOP,
            COUNT
        };
        static const char* VIEW_PIE_ITEMS[] = { "Right", "Bottom", "Front", "Left", "Front", "Top" };
        static constexpr  Soul::EnumArray<CameraViewDirection, Soul::Vec3f> CAMERA_VIEW_DIR({
            Soul::Vec3f(1.0f, 0.0f, 0.0f),
            Soul::Vec3f(0.0f, -1.0f, 0.0f),
            Soul::Vec3f(0.0f, 0.0f, 1.0f),
            Soul::Vec3f(-1.0f, 0.0f, 0.0f),
            Soul::Vec3f(0.0f, 0.0f, -1.0f),
            Soul::Vec3f(0.0f, 1.0f, 0.0f)
        });
        static constexpr Soul::EnumArray<CameraViewDirection, Soul::Vec3f> CAMERA_UP_DIR({
            Soul::Vec3f(0.0f, 1.0f, 0.0f),
            Soul::Vec3f(0.0f, 0.0f, 1.0f),
            Soul::Vec3f(0.0f, 1.0f, 0.0f),
            Soul::Vec3f(0.0f, 1.0f, 0.0f),
            Soul::Vec3f(0.0f, 1.0f, 0.0f),
            Soul::Vec3f(0.0f, 0.0f, -1.0f)
        });
        
        int viewPopupSelected = Demo::UI::PiePopupSelectMenu("##ViewPopup", VIEW_PIE_ITEMS, sizeof(VIEW_PIE_ITEMS) / sizeof(*VIEW_PIE_ITEMS), Demo::Input::KEY_GRAVE_ACCENT);
        if (viewPopupSelected >= 0 && _selectedEntity != ENTITY_ID_NULL) {
            TransformComponent& transformComp = _registry.get<TransformComponent>(_selectedEntity);
            Transformf transform = transformMat4(transformComp.world);
            Soul::Vec3f cameraTarget = transform.position;
            Soul::Vec3f cameraPos = transform.position + 5.0f * rotate(transform.rotation, CAMERA_VIEW_DIR[CameraViewDirection(viewPopupSelected)]);
            Soul::Vec3f cameraUp = rotate(transform.rotation, CAMERA_UP_DIR[CameraViewDirection(viewPopupSelected)]);
            _cameraMan.setCamera(cameraPos, cameraTarget, cameraUp);
        }
    };
    handleViewPopup(input);

    if (!_resetAnimation) {
        _animationDelta += input.deltaTime;
    }
    else {
        _resetAnimation = false;
    }

    auto applyAnimation = [this]() {
        const Animation& activeAnimation = _animations[_activeAnimation.id];

        if (_animationDelta > activeAnimation.duration) {
            _animationDelta = fmod(_animationDelta, activeAnimation.duration);
            for (uint64& cursor : _channelCursors) {
                cursor = 0;
            }
        }

        for (uint64 channelIdx = 0; channelIdx < activeAnimation.channels.size(); channelIdx++) {
            uint64& cursor = _channelCursors[channelIdx];

            const AnimationChannel& channel = activeAnimation.channels[channelIdx];
            const AnimationSampler& sampler = activeAnimation.samplers[channel.samplerIdx];

            while (cursor < sampler.times.size() && sampler.times[cursor] < _animationDelta) {
                cursor++;
            }

            float t = 0.0f;
            uint64 prevIndex;
            uint64 nextIndex;
            if (cursor == 0) {
                nextIndex = 0;
                prevIndex = 0;
            }
            else if (cursor == sampler.times.size()) {
                nextIndex = sampler.times.size() - 1;
                prevIndex = nextIndex;
            }
            else {
                nextIndex = cursor;
                prevIndex = cursor - 1;

                float deltaTime = sampler.times[nextIndex] - sampler.times[prevIndex];
                SOUL_ASSERT(0, deltaTime >= 0, "");

                if (deltaTime > 0) {
                    t = (_animationDelta - sampler.times[prevIndex]) / deltaTime;
                }
            }

            if (sampler.interpolation == AnimationSampler::STEP) {
                t = 0.0f;
            }

            SOUL_ASSERT(0, t <= 1 && t >= 0, "");

            TransformComponent& transformComp = _registry.get<TransformComponent>(channel.entity);
            Transformf transform = transformMat4(transformComp.local);

            switch (channel.transformType) {

            case AnimationChannel::SCALE: {
                const Vec3f* srcVec3 = (const Vec3f*)sampler.values.data();
                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    Vec3f vert0 = srcVec3[prevIndex * 3 + 1];
                    Vec3f tang0 = srcVec3[prevIndex * 3 + 2];
                    Vec3f tang1 = srcVec3[nextIndex * 3];
                    Vec3f vert1 = srcVec3[nextIndex * 3 + 1];
                    transform.scale = cubicSpline(vert0, tang0, vert1, tang1, t);
                }
                else {
                    transform.scale = (srcVec3[prevIndex] * (1 - t)) + (srcVec3[nextIndex] * t);
                }
                break;
            }

            case AnimationChannel::TRANSLATION: {
                const Vec3f* srcVec3 = (const Vec3f*)sampler.values.data();
                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    Vec3f vert0 = srcVec3[prevIndex * 3 + 1];
                    Vec3f tang0 = srcVec3[prevIndex * 3 + 2];
                    Vec3f tang1 = srcVec3[nextIndex * 3];
                    Vec3f vert1 = srcVec3[nextIndex * 3 + 1];
                    transform.position = cubicSpline(vert0, tang0, vert1, tang1, t);
                }
                else {
                    transform.position = (srcVec3[prevIndex] * (1 - t)) + (srcVec3[nextIndex] * t);
                }
                break;
            }

            case AnimationChannel::ROTATION: {
                const Quaternionf* srcQuat = (const Quaternionf*)sampler.values.data();
                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    Quaternionf vert0 = srcQuat[prevIndex * 3 + 1];
                    Quaternionf tang0 = srcQuat[prevIndex * 3 + 2];
                    Quaternionf tang1 = srcQuat[nextIndex * 3];
                    Quaternionf vert1 = srcQuat[nextIndex * 3 + 1];
                    transform.rotation = unit(cubicSpline(vert0, tang0, vert1, tang1, t));
                }
                else {
                    transform.rotation = slerp(srcQuat[prevIndex], srcQuat[nextIndex], t);
                }
                break;
            }

            case AnimationChannel::WEIGHTS: {
                const float* const samplerValues = sampler.values.data();
                const uint64 valuesPerKeyframe = sampler.values.size() / sampler.times.size();

                float weights[MAX_MORPH_TARGETS];
                uint64 numMorphTargets;

                if (sampler.interpolation == AnimationSampler::CUBIC) {
                    SOUL_ASSERT(0, valuesPerKeyframe % 3 == 0, "");
                    numMorphTargets = valuesPerKeyframe / 3;
                    const float* const inTangents = samplerValues;
                    const float* const splineVerts = samplerValues + numMorphTargets;
                    const float* const outTangents = samplerValues + numMorphTargets * 2;

                    for (soul_size comp = 0; comp < std::min(numMorphTargets, MAX_MORPH_TARGETS); ++comp) {
                        float vert0 = splineVerts[comp + prevIndex * valuesPerKeyframe];
                        float tang0 = outTangents[comp + prevIndex * valuesPerKeyframe];
                        float tang1 = inTangents[comp + nextIndex * valuesPerKeyframe];
                        float vert1 = splineVerts[comp + nextIndex * valuesPerKeyframe];
                        weights[comp] = cubicSpline(vert0, tang0, vert1, tang1, t);
                    }
                }
                else {
                    numMorphTargets = valuesPerKeyframe;
                    for (soul_size comp = 0; comp < std::min(valuesPerKeyframe, MAX_MORPH_TARGETS); ++comp) {
                        float previous = samplerValues[comp + prevIndex * valuesPerKeyframe];
                        float current = samplerValues[comp + nextIndex * valuesPerKeyframe];
                        weights[comp] = (1 - t) * previous + t * current;
                    }
                }

                RenderComponent& renderComp = _registry.get<RenderComponent>(channel.entity);
                for (uint64 weightIdx = 0; weightIdx < std::min(numMorphTargets, MAX_MORPH_TARGETS); weightIdx++) {
                    renderComp.morphWeights.mem[weightIdx] = weights[weightIdx];
                }
            }
            }
            
            Mat4f tmp = mat4Transform(transform);
            SOUL_ASSERT(0, std::none_of(tmp.mem, tmp.mem + 9, [](float i) { return std::isnan(i); }), "");
            transformComp.local = mat4Transform(transform);
        }
    };
    if (!_activeAnimation.isNull()) applyAnimation();

    if (_activeCamera != ENTITY_ID_NULL) {

        TransformComponent& transformComp = _registry.get<TransformComponent>(_activeCamera);
        
        if (input.mouseDragging[Demo::Input::MOUSE_BUTTON_MIDDLE]) {
            // orbit camera
            ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
            if (input.keyShift) {
                _cameraMan.pan(mouseDelta.x, mouseDelta.y);
            }
            else {
                _cameraMan.orbit(mouseDelta.x, mouseDelta.y);
            }
        }

        if (input.keyShift) {
            _cameraMan.zoom(input.mouseWheel);
        }

        transformComp.world = _cameraMan.getTransformMatrix();
        if (transformComp.parent == ENTITY_ID_NULL) {
            transformComp.local = transformComp.world;
        }
        else {
            const TransformComponent& parentComp = _registry.get<TransformComponent>(transformComp.parent);
            transformComp.local = mat4Inverse(parentComp.world) * transformComp.world;
        }
    }

    _updateWorldTransform(_rootEntity);
    _updateBones();

    return true;
}

void SoulFila::Scene::_updateWorldTransform(EntityID entityID) {
    if (entityID == ENTITY_ID_NULL) return;
    if (entityID == _rootEntity) {
        TransformComponent& comp = _registry.get<TransformComponent>(entityID);
        comp.world = comp.local;
        SOUL_ASSERT(0, std::none_of(comp.world.mem, comp.world.mem + 9, [](float i) { return std::isnan(i); }), "");
        _updateWorldTransform(comp.firstChild);
    }
    else {
        TransformComponent& comp = _registry.get<TransformComponent>(entityID);
        TransformComponent& parentComp = _registry.get<TransformComponent>(comp.parent);
        comp.world = parentComp.world * comp.local;
        SOUL_ASSERT(0, std::none_of(comp.world.mem, comp.world.mem + 9, [](float i) { return std::isnan(i); }), "");
        _updateWorldTransform(comp.next);
        _updateWorldTransform(comp.firstChild);
    }
}

void SoulFila::Scene::_updateBones() {

    for (Skin& skin : _skins) {
        for (uint64 boneIdx = 0; boneIdx < skin.bones.size(); boneIdx++) {
            EntityID joint = skin.joints[boneIdx];
            const TransformComponent* transformComp = _registry.try_get<TransformComponent>(joint);
            if (transformComp == nullptr) continue;
            Mat4f boneMat = transformComp->world * skin.invBindMatrices[boneIdx];
            MakeBone(&skin.bones[boneIdx], boneMat);
        }
    }
}

void SoulFila::CameraComponent::setLensProjection(double focalLengthInMillimeters, double aspect, double near, double far) {
    double h = (0.5 * near) * ((SENSOR_SIZE * 1000.0) / focalLengthInMillimeters);
    double fovRadian = 2 * std::atan(h / near);
    setPerspectiveProjection(fovRadian, aspect, near, far);
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

void SoulFila::CameraComponent::setScaling(Vec2f vec) {
    this->scaling = vec;
}

Soul::Vec2f SoulFila::CameraComponent::getScaling() const {
    return this->scaling;
}

Soul::Mat4f SoulFila::CameraComponent::getProjectionMatrix() const {
    return this->projection;
}

Soul::Mat4f SoulFila::CameraComponent::getCullingProjectionMatrix() const {
    return this->projectionForCulling;
}

void SoulFila::TransformComponent::renderUI() {
    Transformf localTransform = Soul::transformMat4(local);
    Transformf worldTransform = Soul::transformMat4(world);

    Mat4f parentWorldMat = world * mat4Inverse(local);

    if (ImGui::CollapsingHeader("Transform Component")) {
        bool localTransformChange = false;
        ImGui::Text("Local Transform");
        localTransformChange |= ImGui::InputFloat3("Position##local", (float*)&localTransform.position);
        localTransformChange |= ImGui::InputFloat3("Scale##local", (float*)&localTransform.scale);
        localTransformChange |= ImGui::InputFloat4("Rotation##local", (float*)&localTransform.rotation);
        if (localTransformChange) {
            local = mat4Transform(localTransform);
            world = parentWorldMat * local;
        }

        bool worldTransformChange = false;
        ImGui::Text("World Transform");
        worldTransformChange |= ImGui::InputFloat3("Position##world", (float*)&worldTransform.position);
        worldTransformChange |= ImGui::InputFloat3("Scale##world", (float*)&worldTransform.scale);
        worldTransformChange |= ImGui::InputFloat4("Rotation##world", (float*)&worldTransform.rotation);
        if (worldTransformChange) {
            world = mat4Transform(worldTransform);
            local = mat4Inverse(parentWorldMat) * world;
        }
    }
}

void SoulFila::NameComponent::renderUI() {
    char nameBuffer[1024];
    std::copy(name.data(), name.data() + name.size() + 1, nameBuffer);
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        name = nameBuffer;
    }
}