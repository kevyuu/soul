#include "runtime/scope_allocator.h"

#include "gltf_importer.h"
#include "scene.h"

constexpr const char* CGLTF_ALLOCATION_NAME = "cgltf";

static void* cgltf_malloc(size_t size)
{
  void* ptr = malloc(size);
  SOUL_MEMPROFILE_REGISTER_ALLOCATION(CGLTF_ALLOCATION_NAME, "", ptr, size);
  return ptr;
}

static void cgltf_free(void* ptr)
{
  SOUL_MEMPROFILE_REGISTER_DEALLOCATION(CGLTF_ALLOCATION_NAME, ptr, 0);
  free(ptr);
}

#define CGLTF_MALLOC(sz) cgltf_malloc(sz)
#define CGLTF_FREE(ptr)  cgltf_free(ptr)
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

constexpr const char* STBI_ALLOCATION_NAME = "stbi";

static void* stbi_malloc(size_t size)
{
  void* ptr = malloc(size);
  SOUL_MEMPROFILE_REGISTER_ALLOCATION(STBI_ALLOCATION_NAME, "", ptr, size);
  return ptr;
}

static void stbi_free(void* ptr)
{
  SOUL_MEMPROFILE_REGISTER_DEALLOCATION(STBI_ALLOCATION_NAME, ptr, 0);
  free(ptr);
}

static void* stbi_realloc(void* ptr, size_t size)
{
  SOUL_MEMPROFILE_REGISTER_DEALLOCATION(STBI_ALLOCATION_NAME, ptr, size);
  void* new_ptr = realloc(ptr, size);
  SOUL_MEMPROFILE_REGISTER_ALLOCATION(STBI_ALLOCATION_NAME, "", new_ptr, size);
  return new_ptr;
}

#define STBI_MALLOC(sz)        stbi_malloc(sz)
#define STBI_REALLOC(p, newsz) stbi_realloc(p, newsz)
#define STBI_FREE(p)           stbi_free(p)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr u32 GLTF_URI_MAX_LENGTH = 1000;

namespace soul_fila
{
  static void cgltf_combine_paths(char* path, const char* base, const char* uri)
  {
    const char* s0    = strrchr(base, '/');
    const char* s1    = strrchr(base, '\\');
    const char* slash = s0 ? (s1 && s1 > s0 ? s1 : s0) : s1;

    if (slash)
    {
      size_t prefix = slash - base + 1;

      strncpy(path, base, prefix);
      strcpy(path + prefix, uri);
    } else
    {
      strcpy(path, uri);
    }
  }

  static const char* get_node_name(const cgltf_node* node, const char* default_node_name)
  {
    if (node->name)
    {
      return node->name;
    }
    if (node->mesh && node->mesh->name)
    {
      return node->mesh->name;
    }
    if (node->light && node->light->name)
    {
      return node->light->name;
    }
    if (node->camera && node->camera->name)
    {
      return node->camera->name;
    }
    return default_node_name;
  }

  static void compute_uri_path(char* uri_path, const char* gltf_path, const char* uri)
  {
    cgltf_combine_paths(uri_path, gltf_path, uri);

    // after combining, the tail of the resulting path is a uri; decode_uri converts it into path
    cgltf_decode_uri(uri_path + strlen(uri_path) - strlen(uri));
  }

  static uint8 get_num_uv_sets(const UvMap& uvmap)
  {
    return std::max({
      uvmap[0],
      uvmap[1],
      uvmap[2],
      uvmap[3],
      uvmap[4],
      uvmap[5],
      uvmap[6],
      uvmap[7],
    });
  };

  static bool get_vertex_attr_type(
    const cgltf_attribute_type src_type,
    uint32 index,
    const UvMap& uvmap,
    soul_fila::VertexAttribute* attr_type,
    bool* hasUv0)
  {
    using namespace soul_fila;
    switch (src_type)
    {
    case cgltf_attribute_type_position: *attr_type = VertexAttribute::POSITION; return true;
    case cgltf_attribute_type_texcoord:
    {
      switch (uvmap[index])
      {
      case UvSet::UV0:
        *hasUv0    = true;
        *attr_type = VertexAttribute::UV0;
        return true;
      case UvSet::UV1: *attr_type = VertexAttribute::UV1; return true;
      case UvSet::UNUSED:
        if (!hasUv0 && get_num_uv_sets(uvmap) == 0)
        {
          *hasUv0    = true;
          *attr_type = VertexAttribute::UV0;
          return true;
        }
        return false;
      }
    }
    case cgltf_attribute_type_color: *attr_type = VertexAttribute::COLOR; return true;
    case cgltf_attribute_type_joints: *attr_type = VertexAttribute::BONE_INDICES; return true;
    case cgltf_attribute_type_weights: *attr_type = VertexAttribute::BONE_WEIGHTS; return true;
    case cgltf_attribute_type_invalid: SOUL_NOT_IMPLEMENTED();
    case cgltf_attribute_type_normal:
    case cgltf_attribute_type_tangent:
    default: return false;
    }
  };

  // This converts a cgltf component type into a Filament Attribute type.
  //
  // This function has two out parameters. One result is a safe "permitted type" which we know is
  // universally accepted across GPU's and backends, but may require conversion (see Transcoder).
  // The other result is the "actual type" which requires no conversion.
  //
  // Returns false if the given component type is invalid.
  static bool get_element_type(
    const cgltf_type type,
    const cgltf_component_type ctype,
    soul::gpu::VertexElementType* permit_type,
    soul::gpu::VertexElementType* actual_type)
  {
    using namespace soul::gpu;
    switch (type)
    {
    case cgltf_type_scalar:
      switch (ctype)
      {
      case cgltf_component_type_r_8:
        *permit_type = VertexElementType::BYTE;
        *actual_type = VertexElementType::BYTE;
        return true;
      case cgltf_component_type_r_8u:
        *permit_type = VertexElementType::UBYTE;
        *actual_type = VertexElementType::UBYTE;
        return true;
      case cgltf_component_type_r_16:
        *permit_type = VertexElementType::SHORT;
        *actual_type = VertexElementType::SHORT;
        return true;
      case cgltf_component_type_r_16u:
        *permit_type = VertexElementType::USHORT;
        *actual_type = VertexElementType::USHORT;
        return true;
      case cgltf_component_type_r_32u:
        *permit_type = VertexElementType::UINT;
        *actual_type = VertexElementType::UINT;
        return true;
      case cgltf_component_type_r_32f:
        *permit_type = VertexElementType::FLOAT;
        *actual_type = VertexElementType::FLOAT;
        return true;
      case cgltf_component_type_invalid:
      default: return false;
      }
    case cgltf_type_vec2:
      switch (ctype)
      {
      case cgltf_component_type_r_8:
        *permit_type = VertexElementType::BYTE2;
        *actual_type = VertexElementType::BYTE2;
        return true;
      case cgltf_component_type_r_8u:
        *permit_type = VertexElementType::UBYTE2;
        *actual_type = VertexElementType::UBYTE2;
        return true;
      case cgltf_component_type_r_16:
        *permit_type = VertexElementType::SHORT2;
        *actual_type = VertexElementType::SHORT2;
        return true;
      case cgltf_component_type_r_16u:
        *permit_type = VertexElementType::USHORT2;
        *actual_type = VertexElementType::USHORT2;
        return true;
      case cgltf_component_type_r_32f:
        *permit_type = VertexElementType::FLOAT2;
        *actual_type = VertexElementType::FLOAT2;
        return true;
      case cgltf_component_type_r_32u:
      case cgltf_component_type_invalid:
      default: return false;
      }
    case cgltf_type_vec3:
      switch (ctype)
      {
      case cgltf_component_type_r_8:
        *permit_type = VertexElementType::FLOAT3;
        *actual_type = VertexElementType::BYTE3;
        return true;
      case cgltf_component_type_r_8u:
        *permit_type = VertexElementType::FLOAT3;
        *actual_type = VertexElementType::UBYTE3;
        return true;
      case cgltf_component_type_r_16:
        *permit_type = VertexElementType::FLOAT3;
        *actual_type = VertexElementType::SHORT3;
        return true;
      case cgltf_component_type_r_16u:
        *permit_type = VertexElementType::FLOAT3;
        *actual_type = VertexElementType::USHORT3;
        return true;
      case cgltf_component_type_r_32f:
        *permit_type = VertexElementType::FLOAT3;
        *actual_type = VertexElementType::FLOAT3;
        return true;
      case cgltf_component_type_r_32u:
      case cgltf_component_type_invalid:
      default: return false;
      }
    case cgltf_type_vec4:
      switch (ctype)
      {
      case cgltf_component_type_r_8:
        *permit_type = VertexElementType::BYTE4;
        *actual_type = VertexElementType::BYTE4;
        return true;
      case cgltf_component_type_r_8u:
        *permit_type = VertexElementType::UBYTE4;
        *actual_type = VertexElementType::UBYTE4;
        return true;
      case cgltf_component_type_r_16:
        *permit_type = VertexElementType::SHORT4;
        *actual_type = VertexElementType::SHORT4;
        return true;
      case cgltf_component_type_r_16u:
        *permit_type = VertexElementType::USHORT4;
        *actual_type = VertexElementType::USHORT4;
        return true;
      case cgltf_component_type_r_32f:
        *permit_type = VertexElementType::FLOAT4;
        *actual_type = VertexElementType::FLOAT4;
        return true;
      case cgltf_component_type_r_32u:
      case cgltf_component_type_invalid:
      default: return false;
      }
    case cgltf_type_mat2:
    case cgltf_type_mat3:
    case cgltf_type_mat4:
    case cgltf_type_invalid:
    default: return false;
    }
  }

#define GL_NEAREST                0x2600
#define GL_LINEAR                 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST  0x2701
#define GL_NEAREST_MIPMAP_LINEAR  0x2702
#define GL_LINEAR_MIPMAP_LINEAR   0x2703
#define GL_REPEAT                 0x2901
#define GL_MIRRORED_REPEAT        0x8370
#define GL_CLAMP_TO_EDGE          0x812F

  static soul::gpu::TextureWrap get_wrap_mode(cgltf_int wrap)
  {
    switch (wrap)
    {
    case GL_REPEAT: return soul::gpu::TextureWrap::REPEAT;
    case GL_MIRRORED_REPEAT: return soul::gpu::TextureWrap::MIRRORED_REPEAT;
    case GL_CLAMP_TO_EDGE: return soul::gpu::TextureWrap::CLAMP_TO_EDGE;
    default: return soul::gpu::TextureWrap::REPEAT;
    }
  }

  static soul::gpu::SamplerDesc get_sampler_desc(const cgltf_sampler& srcSampler)
  {
    soul::gpu::SamplerDesc res;
    res.wrapU = get_wrap_mode(srcSampler.wrap_s);
    res.wrapV = get_wrap_mode(srcSampler.wrap_t);
    switch (srcSampler.min_filter)
    {
    case GL_NEAREST: res.minFilter = soul::gpu::TextureFilter::NEAREST; break;
    case GL_LINEAR: res.minFilter = soul::gpu::TextureFilter::LINEAR; break;
    case GL_NEAREST_MIPMAP_NEAREST:
      res.minFilter    = soul::gpu::TextureFilter::NEAREST;
      res.mipmapFilter = soul::gpu::TextureFilter::NEAREST;
      break;
    case GL_LINEAR_MIPMAP_NEAREST:
      res.minFilter    = soul::gpu::TextureFilter::LINEAR;
      res.mipmapFilter = soul::gpu::TextureFilter::NEAREST;
      break;
    case GL_NEAREST_MIPMAP_LINEAR:
      res.minFilter    = soul::gpu::TextureFilter::NEAREST;
      res.mipmapFilter = soul::gpu::TextureFilter::LINEAR;
      break;
    case GL_LINEAR_MIPMAP_LINEAR:
    default:
      res.minFilter    = soul::gpu::TextureFilter::LINEAR;
      res.mipmapFilter = soul::gpu::TextureFilter::LINEAR;
      break;
    }

    switch (srcSampler.mag_filter)
    {
    case GL_NEAREST: res.magFilter = soul::gpu::TextureFilter::NEAREST; break;
    case GL_LINEAR:
    default: res.magFilter = soul::gpu::TextureFilter::LINEAR; break;
    }
    return res;
  }

  static soul::GLSLmat3f32 matrix_from_uv_transform(const cgltf_texture_transform& uvt)
  {
    float tx = uvt.offset[0];
    float ty = uvt.offset[1];
    float sx = uvt.scale[0];
    float sy = uvt.scale[1];
    float c  = cos(uvt.rotation);
    float s  = sin(uvt.rotation);
    soul::mat3f32 mat_transform;
    mat_transform.elem[0][0] = sx * c;
    mat_transform.elem[0][1] = -sy * s;
    mat_transform.elem[0][2] = 0.0f;
    mat_transform.elem[1][0] = sx * s;
    mat_transform.elem[1][1] = sy * c;
    mat_transform.elem[1][2] = 0.0f;
    mat_transform.elem[2][0] = tx;
    mat_transform.elem[2][1] = ty;
    mat_transform.elem[2][2] = 1.0f;
    return soul::GLSLmat3f32(mat_transform);
  }

  static constexpr cgltf_material get_default_cgltf_material()
  {
    cgltf_material kDefaultMat{};
    kDefaultMat.name                        = nullptr;
    kDefaultMat.has_pbr_metallic_roughness  = true;
    kDefaultMat.has_pbr_specular_glossiness = false;
    kDefaultMat.has_clearcoat               = false;
    kDefaultMat.has_transmission            = false;
    kDefaultMat.has_ior                     = false;
    kDefaultMat.has_specular                = false;
    kDefaultMat.has_sheen                   = false;
    kDefaultMat.pbr_metallic_roughness      = {{}, {}, {1.0, 1.0, 1.0, 1.0}, 1.0, 1.0, {}};
    return kDefaultMat;
  }

  static void constrain_gpu_program_key(soul_fila::GPUProgramKey* key, UvMap* uvmap)
  {
    constexpr int MAX_INDEX = 2;
    UvMap retval{};
    int index = 1;

    if (key->hasBaseColorTexture)
    {
      retval[key->baseColorUV] = static_cast<UvSet>(index++);
    }
    key->baseColorUV = retval[key->baseColorUV];

    if (key->brdf.metallicRoughness.hasTexture && retval[key->brdf.metallicRoughness.UV] == UNUSED)
    {
      retval[key->brdf.metallicRoughness.UV] = static_cast<UvSet>(index++);
    }
    key->brdf.metallicRoughness.UV = retval[key->brdf.metallicRoughness.UV];

    auto update_key_and_map =
      [&retval, &index](bool has_texture, const uint8 uv_index_key) -> std::pair<bool, UvSet>
    {
      if (has_texture && retval[uv_index_key] == UNUSED)
      {
        if (index > MAX_INDEX)
        {
          has_texture = false;
        } else
        {
          retval[uv_index_key] = static_cast<UvSet>(index++);
        }
      }
      return {has_texture, retval[uv_index_key]};
    };

    auto [has_texture, uv_index] = update_key_and_map(key->hasNormalTexture, key->normalUV);
    key->hasNormalTexture        = has_texture;
    key->normalUV                = uv_index;

    std::tie(has_texture, uv_index) = update_key_and_map(key->hasOcclusionTexture, key->aoUV);
    key->hasOcclusionTexture        = has_texture;
    key->aoUV                       = uv_index;

    std::tie(has_texture, uv_index) = update_key_and_map(key->hasEmissiveTexture, key->emissiveUV);
    key->hasEmissiveTexture         = has_texture;
    key->emissiveUV                 = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasTransmissionTexture, key->transmissionUV);
    key->hasTransmissionTexture = has_texture;
    key->transmissionUV         = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasClearCoatTexture, key->clearCoatUV);
    key->hasClearCoatTexture = has_texture;
    key->clearCoatUV         = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasClearCoatRoughnessTexture, key->clearCoatRoughnessUV);
    key->hasClearCoatRoughnessTexture = has_texture;
    key->clearCoatRoughnessUV         = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasClearCoatNormalTexture, key->clearCoatNormalUV);
    key->hasClearCoatNormalTexture = has_texture;
    key->clearCoatNormalUV         = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasSheenColorTexture, key->sheenColorUV);
    key->hasSheenColorTexture = has_texture;
    key->sheenColorUV         = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasSheenRoughnessTexture, key->sheenRoughnessUV);
    key->hasSheenRoughnessTexture = has_texture;
    key->sheenRoughnessUV         = uv_index;

    std::tie(has_texture, uv_index) =
      update_key_and_map(key->hasVolumeThicknessTexture, key->volumeThicknessUV);
    key->hasVolumeThicknessTexture = has_texture;
    key->volumeThicknessUV         = uv_index;

    // NOTE: KHR_materials_clearcoat does not provide separate UVs, we'll assume UV0
    *uvmap = retval;
  }

  static bool primitive_has_vertex_color(const cgltf_primitive* in_prim)
  {
    for (cgltf_size slot = 0; slot < in_prim->attributes_count; slot++)
    {
      const cgltf_attribute& inputAttribute = in_prim->attributes[slot];
      if (inputAttribute.type == cgltf_attribute_type_color)
      {
        return true;
      }
    }
    return false;
  }

  static bool get_topology(const cgltf_primitive_type in, soul::gpu::Topology* out)
  {
    using namespace soul::gpu;
    switch (in)
    {
    case cgltf_primitive_type_points: *out = Topology::POINT_LIST; return true;
    case cgltf_primitive_type_lines: *out = Topology::LINE_LIST; return true;
    case cgltf_primitive_type_triangles: *out = Topology::TRIANGLE_LIST; return true;
    case cgltf_primitive_type_line_loop:
    case cgltf_primitive_type_line_strip:
    case cgltf_primitive_type_triangle_strip:
    case cgltf_primitive_type_triangle_fan: return false;
    }
    return false;
  }

  static soul_fila::LightRadiationType get_light_type(const cgltf_light_type light)
  {
    using namespace soul_fila;
    switch (light)
    {
    case cgltf_light_type_directional: return LightRadiationType::DIRECTIONAL;
    case cgltf_light_type_point: return LightRadiationType::POINT;
    case cgltf_light_type_spot: return LightRadiationType::FOCUSED_SPOT;
    case cgltf_light_type_invalid:
    default: SOUL_NOT_IMPLEMENTED(); return LightRadiationType::COUNT;
    }
  }

  // Sometimes a glTF bufferview includes unused data at the end (e.g. in skinning.gltf) so we need
  // to compute the correct size of the vertex buffer. Filament automatically infers the size of
  // driver-level vertex buffers from the attribute data (stride, count, offset) and clients are
  // expected to avoid uploading data blobs that exceed this size. Since this information doesn't
  // exist in the glTF we need to compute it manually. This is a bit of a cheat, cgltf_calc_size is
  // private but its implementation file is available in this cpp file.
  uint32_t compute_binding_size(const cgltf_accessor& accessor)
  {
    const cgltf_size element_size = cgltf_calc_size(accessor.type, accessor.component_type);
    return soul::cast<uint32>(accessor.stride * (accessor.count - 1) + element_size);
  }

  uint32 compute_binding_offset(const cgltf_accessor& accessor)
  {
    return soul::cast<uint32>(accessor.offset + accessor.buffer_view->offset);
  }

  template <typename DstType, typename SrcType>
  static soul::gpu::BufferID create_index_buffer(
    soul::gpu::System& gpu_system, const cgltf_accessor& indices)
  {
    soul::runtime::ScopeAllocator<> scope_allocator("Create Index Buffer"_str);

    const auto buffer_data_raw =
      soul::cast<const uint8*>(indices.buffer_view->buffer->data) + compute_binding_offset(indices);
    const auto buffer_data = soul::cast<const SrcType*>(buffer_data_raw);

    using IndexType                         = DstType;
    const gpu::BufferDesc index_buffer_desc = {
      .count         = indices.count,
      .typeSize      = sizeof(IndexType),
      .typeAlignment = alignof(IndexType),
      .usageFlags    = {gpu::BufferUsage::INDEX},
      .queueFlags    = {gpu::QueueType::GRAPHIC}};

    SOUL_ASSERT(
      0, indices.stride % sizeof(SrcType) == 0, "Stride must be multiple of source type.");
    const uint64 index_stride = indices.stride / sizeof(SrcType);

    soul::Array<IndexType> indexes(&scope_allocator);
    indexes.resize(indices.count);
    for (soul_size i = 0; i < indices.count; i++)
    {
      indexes[i] = IndexType(*(buffer_data + index_stride * i));
    }

    const soul::gpu::BufferID buffer_id =
      gpu_system.create_buffer(index_buffer_desc, indexes.data());
    gpu_system.finalize_buffer(buffer_id);
    return buffer_id;
  }

  static void add_attribute_to_primitive(
    Primitive* primitive,
    const VertexAttribute attr_type,
    const soul::gpu::BufferID gpu_buffer,
    const soul::gpu::VertexElementType type,
    const soul::gpu::VertexElementFlags flags,
    const uint8 attribute_stride)
  {

    primitive->vertexBuffers[primitive->vertexBindingCount] = gpu_buffer;
    primitive->attributes[to_underlying(attr_type)]         = {
      0, attribute_stride, primitive->vertexBindingCount, type, flags};
    primitive->vertexBindingCount++;
    primitive->activeAttribute |= (1 << attr_type);
  }

  static void normalize(cgltf_accessor* data)
  {
    if (data->type != cgltf_type_vec4 || data->component_type != cgltf_component_type_r_32f)
    {
      SOUL_LOG_ERROR("Attribute type is not supported");
      SOUL_NOT_IMPLEMENTED();
    }
    auto bytes = soul::cast<uint8*>(data->buffer_view->buffer->data);
    bytes += data->offset + data->buffer_view->offset;
    for (cgltf_size i = 0, n = data->count; i < n; ++i, bytes += data->stride)
    {
      auto weights    = soul::cast<vec4f32*>(bytes);
      const float sum = weights->x + weights->y + weights->z + weights->w;
      *weights /= sum;
    }
  };

  struct AttributeBuffer
  {
    const byte* data;
    soul_size data_count;
    soul_size stride;
    soul_size type_size;
    soul_size type_alignment;
  };

  AttributeBuffer get_attribute_buffer(
    soul::memory::Allocator& allocator,
    const cgltf_attribute& src_attribute,
    const cgltf_accessor& accessor)
  {
    if (
      accessor.is_sparse || src_attribute.type == cgltf_attribute_type_tangent ||
      src_attribute.type == cgltf_attribute_type_normal ||
      src_attribute.type == cgltf_attribute_type_position)
    {

      const cgltf_size num_floats = accessor.count * cgltf_num_components(accessor.type);

      float* generated = allocator.create_raw_array<float>(num_floats);
      cgltf_accessor_unpack_floats(&accessor, generated, num_floats);
      soul_size type_size = cgltf_num_components(accessor.type) * sizeof(float);

      return {soul::cast<byte*>(generated), accessor.count, type_size, type_size, sizeof(float)};
    }

    auto buffer_data = soul::cast<const byte*>(accessor.buffer_view->buffer->data);
    return {
      buffer_data + compute_binding_offset(accessor),
      accessor.count,
      accessor.stride,
      cgltf_calc_size(accessor.type, accessor.component_type),
      cgltf_component_size(accessor.component_type)};
  }

  soul_size get_vertex_count(const cgltf_primitive& primitive)
  {
    return primitive.attributes[0].data->count;
  }

  struct IndexData
  {
    vec3u32* triangles32    = nullptr;
    soul_size triangleCount = 0;
    soul::gpu::BufferID gpu_handle;
  };

  IndexData create_index_buffer(
    soul::memory::Allocator& allocator,
    soul::gpu::System& gpu_system,
    const cgltf_primitive& src_primitive)
  {
    using IndexType = uint32;
    if (src_primitive.indices)
    {

      const cgltf_accessor& src_indices = *src_primitive.indices;

      const soul::gpu::BufferID index_buffer = [&gpu_system, src_primitive, src_indices]()
      {
        switch (src_primitive.indices->component_type)
        {
        case cgltf_component_type_r_8u:
        {
          return create_index_buffer<uint16, uint8>(gpu_system, src_indices);
        }
        case cgltf_component_type_r_16u:
        {
          return create_index_buffer<uint16, uint16>(gpu_system, src_indices);
        }
        case cgltf_component_type_r_32u:
        {
          return create_index_buffer<uint32, uint32>(gpu_system, src_indices);
        }
        case cgltf_component_type_r_8:
        case cgltf_component_type_r_16:
        case cgltf_component_type_r_32f:
        case cgltf_component_type_invalid:
        default: SOUL_NOT_IMPLEMENTED(); return soul::gpu::BufferID::Null();
        }
      }();

      IndexType* indexes = allocator.create_raw_array<IndexType>(src_indices.count);
      for (cgltf_size index_idx = 0; index_idx < src_indices.count; ++index_idx)
      {
        indexes[index_idx] = soul::cast<uint32>(cgltf_accessor_read_index(&src_indices, index_idx));
      }
      return {soul::cast<vec3u32*>(indexes), src_indices.count / 3, index_buffer};
    }
    if (src_primitive.attributes_count > 0)
    {
      const gpu::BufferDesc index_buffer_desc = {
        .count         = src_primitive.attributes[0].data->count,
        .typeSize      = sizeof(IndexType),
        .typeAlignment = alignof(IndexType),
        .usageFlags    = {gpu::BufferUsage::INDEX},
        .queueFlags    = {gpu::QueueType::GRAPHIC}};

      IndexType* indexes = allocator.create_raw_array<IndexType>(index_buffer_desc.count);
      std::iota(indexes, indexes + index_buffer_desc.count, 0);

      const soul::gpu::BufferID index_buffer = gpu_system.create_buffer(index_buffer_desc, indexes);
      gpu_system.finalize_buffer(index_buffer);

      return {soul::cast<vec3u32*>(indexes), index_buffer_desc.count / 3, index_buffer};
    }

    return {nullptr, 0, soul::gpu::BufferID::Null()};
  }

  struct AttributeData
  {
    vec3f32* normals   = nullptr;
    vec4f32* tangents  = nullptr;
    vec2f32* uvs       = nullptr;
    vec3f32* positions = nullptr;
    Vec4i16* qtangents = nullptr;
  };

  static AttributeData add_attributes_to_primitive(
    soul::memory::Allocator& allocator,
    soul::gpu::System& gpu_system,
    Primitive& dst_primitive,
    const cgltf_primitive& src_primitive,
    const UvMap& uvmap,
    const IndexData& index_data)
  {
    soul_size vertex_count = get_vertex_count(src_primitive);
    AttributeData attribute_data;
    bool has_uv0 = false;
    for (cgltf_size attr_index = 0; attr_index < src_primitive.attributes_count; attr_index++)
    {
      const cgltf_attribute& src_attribute = src_primitive.attributes[attr_index];
      const cgltf_accessor& accessor       = *src_attribute.data;

      if (src_attribute.type == cgltf_attribute_type_weights)
      {
        normalize(src_attribute.data);
      }

      AttributeBuffer attribute_buffer = get_attribute_buffer(allocator, src_attribute, accessor);

      if (src_attribute.type == cgltf_attribute_type_tangent)
      {
        SOUL_ASSERT(0, sizeof(vec4f32) == attribute_buffer.stride);
        attribute_data.tangents = soul::cast<vec4f32*>(attribute_buffer.data);
        continue;
      }
      if (src_attribute.type == cgltf_attribute_type_normal)
      {
        SOUL_ASSERT(0, sizeof(vec3f32) == attribute_buffer.stride);
        attribute_data.normals = soul::cast<vec3f32*>(attribute_buffer.data);
        continue;
      }

      if (src_attribute.type == cgltf_attribute_type_texcoord && src_attribute.index == 0)
      {
        const cgltf_size num_floats = accessor.count * cgltf_num_components(accessor.type);
        auto generated              = allocator.create_raw_array<float>(num_floats);
        cgltf_accessor_unpack_floats(&accessor, soul::cast<float*>(generated), num_floats);
        attribute_data.uvs = soul::cast<vec2f32*>(generated);
      }

      if (src_attribute.type == cgltf_attribute_type_position)
      {
        SOUL_ASSERT(0, sizeof(vec3f32) == attribute_buffer.stride);
        attribute_data.positions = soul::cast<vec3f32*>(attribute_buffer.data);
        dst_primitive.aabb =
          aabbCombine(dst_primitive.aabb, AABB(vec3f32(accessor.min), vec3f32(accessor.max)));
      }

      VertexAttribute attr_type;
      const bool attr_supported =
        get_vertex_attr_type(src_attribute.type, src_attribute.index, uvmap, &attr_type, &has_uv0);
      if (!attr_supported)
      {
        continue;
      }

      const gpu::BufferDesc gpu_desc = {
        .count         = attribute_buffer.data_count,
        .typeSize      = soul::cast<uint16>(attribute_buffer.type_size),
        .typeAlignment = soul::cast<uint16>(attribute_buffer.type_alignment),
        .usageFlags    = {gpu::BufferUsage::VERTEX},
        .queueFlags    = {gpu::QueueType::GRAPHIC}};

      const soul_size attribute_data_size =
        attribute_buffer.type_size * attribute_buffer.data_count;
      void* attribute_gpu_data =
        allocator.allocate(attribute_data_size, attribute_buffer.type_alignment);
      for (soul_size attribute_idx = 0; attribute_idx < attribute_buffer.data_count;
           attribute_idx++)
      {
        const uint64 offset = attribute_idx * attribute_buffer.stride;
        memcpy(
          soul::cast<byte*>(attribute_gpu_data) + (attribute_idx * attribute_buffer.type_size),
          attribute_buffer.data + offset,
          attribute_buffer.type_size);
      }

      const gpu::BufferID attribute_gpu_buffer =
        gpu_system.create_buffer(gpu_desc, attribute_gpu_data);
      gpu_system.finalize_buffer(attribute_gpu_buffer);

      soul::gpu::VertexElementType permitted;
      soul::gpu::VertexElementType actual;
      get_element_type(accessor.type, accessor.component_type, &permitted, &actual);

      soul::gpu::VertexElementFlags flags = 0;
      if (accessor.normalized)
      {
        flags |= gpu::VERTEX_ELEMENT_NORMALIZED;
      }
      if (attr_type == VertexAttribute::BONE_INDICES)
      {
        flags |= gpu::VERTEX_ELEMENT_INTEGER_TARGET;
      }

      add_attribute_to_primitive(
        &dst_primitive,
        attr_type,
        attribute_gpu_buffer,
        actual,
        flags,
        soul::cast<uint8>(attribute_buffer.type_size));
    }

    if (
      attribute_data.normals != nullptr || src_primitive.material && !src_primitive.material->unlit)
    {
      const soul_size qtangent_buffer_size = vertex_count * sizeof(Quaternionf);
      auto qtangents = allocator.create_raw_array<Quaternionf>(qtangent_buffer_size);
      if (ComputeTangentFrame(
            TangentFrameComputeInput(
              vertex_count,
              attribute_data.normals,
              attribute_data.tangents,
              attribute_data.uvs,
              attribute_data.positions,
              index_data.triangles32,
              index_data.triangleCount),
            qtangents))
      {

        attribute_data.qtangents = allocator.create_raw_array<Vec4i16>(vertex_count);
        std::transform(
          qtangents,
          qtangents + vertex_count,
          attribute_data.qtangents,
          [](const Quaternionf& qtangent)
          {
            return packSnorm16(qtangent.xyzw);
          });

        const gpu::BufferDesc qtangents_buffer_desc = {
          vertex_count,
          sizeof(Vec4i16),
          alignof(Vec4i16),
          {gpu::BufferUsage::VERTEX},
          {gpu::QueueType::GRAPHIC}};
        const soul::gpu::BufferID qtangents_gpu_buffer =
          gpu_system.create_buffer(qtangents_buffer_desc, attribute_data.qtangents);
        gpu_system.finalize_buffer(qtangents_gpu_buffer);
        add_attribute_to_primitive(
          &dst_primitive,
          VertexAttribute::QTANGENTS,
          qtangents_gpu_buffer,
          gpu::VertexElementType::SHORT4,
          gpu::VERTEX_ELEMENT_NORMALIZED,
          sizeof(Vec4i16));
      }
    }

    return attribute_data;
  }

  static void add_morph_attributes_to_primitive(
    soul::memory::Allocator& allocator,
    soul::gpu::System& gpu_system,
    AttributeData& attribute_data,
    Primitive& dst_primitive,
    const cgltf_primitive& src_primitive,
    const UvMap& uvmap,
    const IndexData& index_data)
  {
    cgltf_size targets_count = src_primitive.targets_count;
    if (targets_count > MAX_MORPH_TARGETS)
    {
      SOUL_LOG_WARN(
        "Cannot load all the morph targets. num target = %d, max target = %d",
        targets_count,
        MAX_MORPH_TARGETS);
    }
    targets_count = targets_count > MAX_MORPH_TARGETS ? MAX_MORPH_TARGETS : targets_count;

    const soul_size vertex_count = get_vertex_count(src_primitive);

    constexpr auto base_tangents_attr = to_underlying(VertexAttribute::MORPH_BASE_TANGENTS);
    constexpr auto base_position_attr = to_underlying(VertexAttribute::MORPH_BASE_POSITION);
    for (cgltf_size target_index = 0; target_index < targets_count; target_index++)
    {
      const cgltf_morph_target& morph_target = src_primitive.targets[target_index];

      enum class MorphTargetType : uint8
      {
        POSITION,
        NORMAL,
        TANGENT,
        COUNT
      };

      auto get_morph_target_type =
        [](cgltf_attribute_type atype, MorphTargetType* target_type) -> bool
      {
        switch (atype)
        {
        case cgltf_attribute_type_position: *target_type = MorphTargetType::POSITION; return true;
        case cgltf_attribute_type_tangent: *target_type = MorphTargetType::TANGENT; return true;
        case cgltf_attribute_type_normal: *target_type = MorphTargetType::NORMAL; return true;
        case cgltf_attribute_type_color:
        case cgltf_attribute_type_weights:
        case cgltf_attribute_type_joints:
        case cgltf_attribute_type_texcoord:
        case cgltf_attribute_type_invalid:
        default: return false;
        }
      };

      soul::EnumArray<MorphTargetType, const byte*> morph_target_attributes(nullptr);

      for (cgltf_size attribute_index = 0; attribute_index < morph_target.attributes_count;
           attribute_index++)
      {
        const cgltf_attribute& src_attribute = morph_target.attributes[attribute_index];
        const cgltf_accessor& accessor       = *src_attribute.data;

        MorphTargetType morph_target_type;
        const bool success = get_morph_target_type(src_attribute.type, &morph_target_type);
        SOUL_ASSERT(0, success);

        const cgltf_size num_floats = accessor.count * cgltf_num_components(accessor.type);
        auto generated              = allocator.create_raw_array<float>(num_floats);
        cgltf_accessor_unpack_floats(&accessor, generated, num_floats);

        const uint16 attribute_type_size =
          soul::cast<uint8>(cgltf_num_components(accessor.type) * sizeof(float));
        const AttributeBuffer attribute_buffer = {
          .data           = soul::cast<const byte*>(generated),
          .data_count     = accessor.count,
          .stride         = attribute_type_size,
          .type_size      = attribute_type_size,
          .type_alignment = sizeof(float)};

        morph_target_attributes[morph_target_type] = attribute_buffer.data;

        if (src_attribute.type == cgltf_attribute_type_position)
        {
          auto attr_type = static_cast<VertexAttribute>(base_position_attr + target_index);
          const gpu::BufferDesc gpu_desc = {
            .count         = attribute_buffer.data_count,
            .typeSize      = attribute_type_size,
            .typeAlignment = soul::cast<uint16>(attribute_buffer.type_alignment),
            .usageFlags    = {gpu::BufferUsage::VERTEX},
            .queueFlags    = {gpu::QueueType::GRAPHIC}

          };
          const soul_size attribute_gpu_data_size =
            attribute_buffer.data_count * attribute_type_size;
          void* attribute_gpu_data =
            allocator.allocate(attribute_gpu_data_size, attribute_buffer.type_alignment);
          for (soul_size attribute_idx = 0; attribute_idx < attribute_buffer.data_count;
               attribute_idx++)
          {
            const uint64 offset = attribute_idx * attribute_buffer.stride;
            memcpy(
              soul::cast<byte*>(attribute_gpu_data) + (attribute_idx * attribute_type_size),
              attribute_buffer.data + offset,
              attribute_type_size);
          }
          gpu::BufferID attribute_gpu_buffer =
            gpu_system.create_buffer(gpu_desc, attribute_gpu_data);
          gpu_system.finalize_buffer(attribute_gpu_buffer);
          allocator.deallocate(attribute_gpu_data, attribute_gpu_data_size);

          soul::gpu::VertexElementType permitted;
          soul::gpu::VertexElementType actual;
          get_element_type(accessor.type, accessor.component_type, &permitted, &actual);

          soul::gpu::VertexElementFlags flags = 0;
          if (accessor.normalized)
          {
            flags |= gpu::VERTEX_ELEMENT_NORMALIZED;
          }

          add_attribute_to_primitive(
            &dst_primitive,
            attr_type,
            attribute_gpu_buffer,
            actual,
            flags,
            soul::cast<uint8>(attribute_buffer.type_size));

          dst_primitive.aabb =
            aabbCombine(dst_primitive.aabb, AABB(vec3f32(accessor.min), vec3f32(accessor.max)));
        }
        allocator.destroy_array(generated, num_floats);
      }

      if (morph_target_attributes[MorphTargetType::NORMAL])
      {
        if (attribute_data.normals)
        {

          auto normal_target =
            soul::cast<const vec3f32*>(morph_target_attributes[MorphTargetType::NORMAL]);
          if (normal_target != nullptr)
          {
            for (cgltf_size vert_index = 0; vert_index < vertex_count; vert_index++)
            {
              attribute_data.normals[vert_index] += normal_target[vert_index];
            }
          }

          auto tangent_target =
            soul::cast<const vec3f32*>(morph_target_attributes[MorphTargetType::TANGENT]);
          if (tangent_target != nullptr)
          {
            for (cgltf_size vert_index = 0; vert_index < vertex_count; vert_index++)
            {
              attribute_data.tangents[vert_index].xyz += tangent_target[vert_index];
            }
          }

          auto position_target =
            soul::cast<const vec3f32*>(morph_target_attributes[MorphTargetType::POSITION]);
          if (position_target != nullptr)
          {
            for (cgltf_size vertIndex = 0; vertIndex < vertex_count; vertIndex++)
            {
              attribute_data.positions[vertIndex] += position_target[vertIndex];
            }
          }
        }
      }

      const soul_size qtangent_buffer_size = vertex_count * sizeof(Quaternionf);
      auto qtangents = allocator.create_raw_array<Quaternionf>(qtangent_buffer_size);
      if (ComputeTangentFrame(
            TangentFrameComputeInput(
              vertex_count,
              attribute_data.normals,
              attribute_data.tangents,
              attribute_data.uvs,
              attribute_data.positions,
              index_data.triangles32,
              index_data.triangleCount),
            qtangents))
      {
        const gpu::BufferDesc qtangents_buffer_desc = {
          vertex_count,
          sizeof(Quaternionf),
          alignof(Quaternionf),
          {gpu::BufferUsage::VERTEX},
          {gpu::QueueType::GRAPHIC}};
        const soul::gpu::BufferID qtangents_gpu_buffer =
          gpu_system.create_buffer(qtangents_buffer_desc, qtangents);
        gpu_system.finalize_buffer(qtangents_gpu_buffer);
        add_attribute_to_primitive(
          &dst_primitive,
          static_cast<VertexAttribute>(base_tangents_attr + target_index),
          qtangents_gpu_buffer,
          gpu::VertexElementType::SHORT4,
          gpu::VERTEX_ELEMENT_NORMALIZED,
          sizeof(Quaternionf));
      }
      allocator.destroy_array(qtangents, qtangent_buffer_size);
    }
  }

  GLTFImporter::GLTFImporter(
    const char* gltf_path,
    soul::gpu::System& gpu_system,
    GPUProgramRegistry& program_registry,
    Scene& scene)
      : gltf_path_(gltf_path),
        gpu_system_(gpu_system),
        program_registry_(program_registry),
        scene_(scene)
  {
  }

  void GLTFImporter::import()
  {
    SOUL_PROFILE_ZONE();
    cgltf_options options = {};

    cgltf_result result = cgltf_parse_file(&options, gltf_path_, &asset_);
    SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf json");

    const cgltf_scene* scene = asset_->scene ? asset_->scene : asset_->scenes;
    if (!scene)
    {
      return;
    }

    result = cgltf_load_buffers(&options, asset_, gltf_path_);
    SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf buffers");

    scene_.create_meshes(
      asset_->meshes_count,
      [this](const soul_size mesh_index, Mesh& dst_mesh)
      {
        const cgltf_mesh& src_mesh = asset_->meshes[mesh_index];
        dst_mesh.primitives.resize(src_mesh.primitives_count);

        for (cgltf_size primitive_index = 0; primitive_index < src_mesh.primitives_count;
             primitive_index++)
        {
          soul::runtime::ScopeAllocator<> primitive_scope_allocator("Loading Attribute Allocation"_str);

          const cgltf_primitive& src_primitive = src_mesh.primitives[primitive_index];

          if (src_primitive.has_draco_mesh_compression)
          {
            SOUL_NOT_IMPLEMENTED();
          }

          UvMap uvmap{};
          Primitive& dst_primitive    = dst_mesh.primitives[primitive_index];
          const bool has_vertex_color = primitive_has_vertex_color(&src_primitive);
          dst_primitive.materialID =
            create_material(src_primitive.material, has_vertex_color, uvmap);

          const bool get_topology_success =
            get_topology(src_primitive.type, &dst_primitive.topology);
          SOUL_ASSERT(0, get_topology_success);

          const IndexData index_data =
            create_index_buffer(primitive_scope_allocator, gpu_system_, src_primitive);
          dst_primitive.indexBuffer = index_data.gpu_handle;

          AttributeData attribute_data = add_attributes_to_primitive(
            primitive_scope_allocator,
            gpu_system_,
            dst_primitive,
            src_primitive,
            uvmap,
            index_data);
          add_morph_attributes_to_primitive(
            primitive_scope_allocator,
            gpu_system_,
            attribute_data,
            dst_primitive,
            src_primitive,
            uvmap,
            index_data);
          dst_mesh.aabb = aabbCombine(dst_mesh.aabb, dst_primitive.aabb);
        }
      });
    import_textures();
    SOUL_ASSERT(1, scene_.check_resources_validity());

    import_entities();
    import_animations();
    import_skins();
    scene_.update_bounding_box();
    scene_.fit_into_unit_cube();

    cgltf_free(asset_);

    scene_.create_dfg("./assets/default_env/default_env_ibl.ktx", "Default env IBL");
    scene_.create_default_sunlight();
    if (scene_.get_default_camera() == ENTITY_ID_NULL)
    {
      scene_.create_default_camera();
    }
  }

  MaterialID GLTFImporter::create_material(
    const cgltf_material* src_material, const bool vertex_color, UvMap& uvmap)
  {
    SOUL_PROFILE_ZONE_WITH_NAME("Create Material");
    MatCacheKey key = {((intptr_t)src_material) ^ (vertex_color ? 1 : 0)};

    if (mat_cache_.contains(key))
    {
      const MatCacheEntry& entry = mat_cache_[key];
      uvmap                      = entry.uvmap;
      return entry.materialID;
    }

    MaterialID material_id = scene_.create_material();
    Material& dst_material = *scene_.get_material_ptr(material_id);

    static constexpr cgltf_material kDefaultMat = get_default_cgltf_material();
    src_material                                = src_material ? src_material : &kDefaultMat;

    auto mr_config = src_material->pbr_metallic_roughness;
    auto sg_config = src_material->pbr_specular_glossiness;
    auto cc_config = src_material->clearcoat;
    auto tr_config = src_material->transmission;
    auto sh_config = src_material->sheen;
    auto vl_config = src_material->volume;

    const bool has_texture_transforms =
      sg_config.diffuse_texture.has_transform ||
      sg_config.specular_glossiness_texture.has_transform ||
      mr_config.base_color_texture.has_transform ||
      mr_config.metallic_roughness_texture.has_transform ||
      src_material->normal_texture.has_transform || src_material->occlusion_texture.has_transform ||
      src_material->emissive_texture.has_transform || cc_config.clearcoat_texture.has_transform ||
      cc_config.clearcoat_roughness_texture.has_transform ||
      cc_config.clearcoat_normal_texture.has_transform ||
      sh_config.sheen_color_texture.has_transform ||
      sh_config.sheen_roughness_texture.has_transform ||
      tr_config.transmission_texture.has_transform;

    cgltf_texture_view base_color_texture         = mr_config.base_color_texture;
    cgltf_texture_view metallic_roughness_texture = mr_config.metallic_roughness_texture;

    GPUProgramKey program_key;
    program_key.doubleSided         = !!src_material->double_sided;
    program_key.unlit               = !!src_material->unlit;
    program_key.hasVertexColors     = vertex_color;
    program_key.hasBaseColorTexture = !!base_color_texture.texture;
    program_key.hasNormalTexture    = !!src_material->normal_texture.texture;
    program_key.hasOcclusionTexture = !!src_material->occlusion_texture.texture;
    program_key.hasEmissiveTexture  = !!src_material->emissive_texture.texture;
    program_key.enableDiagnostics   = true;
    program_key.baseColorUV         = soul::cast<uint8>(base_color_texture.texcoord);
    program_key.hasClearCoatTexture = !!cc_config.clearcoat_texture.texture;
    program_key.clearCoatUV         = soul::cast<uint8>(cc_config.clearcoat_texture.texcoord);
    program_key.hasClearCoatRoughnessTexture = !!cc_config.clearcoat_roughness_texture.texture;
    program_key.clearCoatRoughnessUV =
      soul::cast<uint8>(cc_config.clearcoat_roughness_texture.texcoord);
    program_key.hasClearCoatNormalTexture = !!cc_config.clearcoat_normal_texture.texture;
    program_key.clearCoatNormalUV = soul::cast<uint8>(cc_config.clearcoat_normal_texture.texcoord);
    program_key.hasClearCoat      = !!src_material->has_clearcoat;
    program_key.hasTransmission   = !!src_material->has_transmission;
    program_key.hasTextureTransforms = has_texture_transforms;
    program_key.emissiveUV           = soul::cast<uint8>(src_material->emissive_texture.texcoord);
    program_key.aoUV                 = soul::cast<uint8>(src_material->occlusion_texture.texcoord);
    program_key.normalUV             = soul::cast<uint8>(src_material->normal_texture.texcoord);
    program_key.hasTransmissionTexture = !!tr_config.transmission_texture.texture;
    program_key.transmissionUV         = soul::cast<uint8>(tr_config.transmission_texture.texcoord);
    program_key.hasSheenColorTexture   = !!sh_config.sheen_color_texture.texture;
    program_key.sheenColorUV           = soul::cast<uint8>(sh_config.sheen_color_texture.texcoord);
    program_key.hasSheenRoughnessTexture = !!sh_config.sheen_roughness_texture.texture;
    program_key.sheenRoughnessUV = soul::cast<uint8>(sh_config.sheen_roughness_texture.texcoord);
    program_key.hasVolumeThicknessTexture = !!vl_config.thickness_texture.texture;
    program_key.volumeThicknessUV         = soul::cast<uint8>(vl_config.thickness_texture.texcoord);
    program_key.hasSheen                  = !!src_material->has_sheen;
    program_key.hasIOR                    = !!src_material->has_ior;
    program_key.hasVolume                 = !!src_material->has_volume;

    if (src_material->has_pbr_specular_glossiness)
    {
      program_key.useSpecularGlossiness = true;
      if (sg_config.diffuse_texture.texture)
      {
        base_color_texture              = sg_config.diffuse_texture;
        program_key.hasBaseColorTexture = true;
        program_key.baseColorUV         = soul::cast<uint8>(base_color_texture.texcoord);
      }
      if (sg_config.specular_glossiness_texture.texture)
      {
        metallic_roughness_texture                     = sg_config.specular_glossiness_texture;
        program_key.brdf.specularGlossiness.hasTexture = true;
        program_key.brdf.specularGlossiness.UV =
          soul::cast<uint8>(metallic_roughness_texture.texcoord);
      }
    } else
    {
      program_key.brdf.metallicRoughness.hasTexture = !!metallic_roughness_texture.texture;
      program_key.brdf.metallicRoughness.UV =
        soul::cast<uint8>(metallic_roughness_texture.texcoord);
    }
    switch (src_material->alpha_mode)
    {
    case cgltf_alpha_mode_opaque: program_key.alphaMode = AlphaMode::OPAQUE; break;
    case cgltf_alpha_mode_mask: program_key.alphaMode = AlphaMode::MASK; break;
    case cgltf_alpha_mode_blend: program_key.alphaMode = AlphaMode::BLEND; break;
    }

    constrain_gpu_program_key(&program_key, &uvmap);
    dst_material.programSetID = program_registry_.createProgramSet(program_key);

    MaterialUBO& mat_buf           = dst_material.buffer;
    MaterialTextures& mat_textures = dst_material.textures;

    mat_buf.baseColorFactor = soul::vec4f32(mr_config.base_color_factor);
    mat_buf.emissiveFactor  = soul::vec3f32(src_material->emissive_factor);
    mat_buf.metallicFactor  = mr_config.metallic_factor;
    mat_buf.roughnessFactor = mr_config.roughness_factor;

    if (program_key.useSpecularGlossiness)
    {
      mat_buf.baseColorFactor = soul::vec4f32(sg_config.diffuse_factor);
      mat_buf.specularFactor  = soul::vec3f32(sg_config.specular_factor);
      mat_buf.roughnessFactor = mr_config.roughness_factor;
    }

    if (program_key.hasBaseColorTexture)
    {
      mat_textures.baseColorTexture = create_texture(*base_color_texture.texture, true);
      if (program_key.hasTextureTransforms)
      {
        const cgltf_texture_transform& uvt = base_color_texture.transform;
        mat_buf.baseColorUvMatrix          = matrix_from_uv_transform(uvt);
      }
    }

    if (program_key.brdf.metallicRoughness.hasTexture)
    {
      bool srgb = src_material->has_pbr_specular_glossiness;
      mat_textures.metallicRoughnessTexture =
        create_texture(*metallic_roughness_texture.texture, srgb);
      if (program_key.hasTextureTransforms)
      {
        const cgltf_texture_transform& uvt = metallic_roughness_texture.transform;
        mat_buf.metallicRoughnessUvMatrix  = matrix_from_uv_transform(uvt);
      }
    }

    if (program_key.hasNormalTexture)
    {
      mat_textures.normalTexture = create_texture(*src_material->normal_texture.texture, false);
      if (program_key.hasTextureTransforms)
      {
        const cgltf_texture_transform& uvt = src_material->normal_texture.transform;
        mat_buf.normalUvMatrix             = matrix_from_uv_transform(uvt);
      }
      mat_buf.normalScale = src_material->normal_texture.scale;
    } else
    {
      mat_buf.normalScale = 1.0f;
    }

    if (program_key.hasOcclusionTexture)
    {
      mat_textures.occlusionTexture =
        create_texture(*src_material->occlusion_texture.texture, false);
      if (program_key.hasTextureTransforms)
      {
        mat_buf.occlusionUvMatrix =
          matrix_from_uv_transform(src_material->occlusion_texture.transform);
      }
      mat_buf.aoStrength = src_material->occlusion_texture.scale;
    } else
    {
      mat_buf.aoStrength = 1.0f;
    }

    if (program_key.hasEmissiveTexture)
    {
      mat_textures.emissiveTexture = create_texture(*src_material->emissive_texture.texture, true);
      if (program_key.hasTextureTransforms)
      {
        mat_buf.emissiveUvMatrix =
          matrix_from_uv_transform(src_material->emissive_texture.transform);
      }
    }

    if (program_key.hasClearCoat)
    {
      mat_buf.clearCoatFactor          = cc_config.clearcoat_factor;
      mat_buf.clearCoatRoughnessFactor = cc_config.clearcoat_roughness_factor;

      if (program_key.hasClearCoatTexture)
      {
        mat_textures.clearCoatTexture = create_texture(*cc_config.clearcoat_texture.texture, false);
        if (program_key.hasTextureTransforms)
        {
          mat_buf.clearCoatUvMatrix =
            matrix_from_uv_transform(cc_config.clearcoat_texture.transform);
        }
      }

      if (program_key.hasClearCoatRoughnessTexture)
      {
        mat_textures.clearCoatRoughnessTexture =
          create_texture(*cc_config.clearcoat_roughness_texture.texture, false);
        if (program_key.hasTextureTransforms)
        {
          mat_buf.clearCoatRoughnessMatrix =
            matrix_from_uv_transform(cc_config.clearcoat_roughness_texture.transform);
        }
      }

      if (program_key.hasClearCoatNormalTexture)
      {
        mat_textures.clearCoatNormalTexture =
          create_texture(*cc_config.clearcoat_normal_texture.texture, false);
        if (program_key.hasClearCoatNormalTexture)
        {
          mat_buf.clearCoatNormalUvMatrix =
            matrix_from_uv_transform(cc_config.clearcoat_normal_texture.transform);
        }
        mat_buf.clearCoatNormalScale = cc_config.clearcoat_normal_texture.scale;
      }
    }

    if (program_key.hasSheen)
    {
      mat_buf.sheenColorFactor     = soul::vec3f32(sh_config.sheen_color_factor);
      mat_buf.sheenRoughnessFactor = sh_config.sheen_roughness_factor;

      if (program_key.hasSheenColorTexture)
      {
        mat_textures.sheenColorTexture =
          create_texture(*sh_config.sheen_color_texture.texture, true);
        if (program_key.hasTextureTransforms)
        {
          mat_buf.sheenColorUvMatrix =
            matrix_from_uv_transform(sh_config.sheen_color_texture.transform);
        }
      }

      if (program_key.hasSheenRoughnessTexture)
      {
        mat_textures.sheenRoughnessTexture =
          create_texture(*sh_config.sheen_roughness_texture.texture, false);
        if (program_key.hasTextureTransforms)
        {
          mat_buf.sheenRoughnessUvMatrix =
            matrix_from_uv_transform(sh_config.sheen_roughness_texture.transform);
        }
      }
    }

    if (program_key.hasVolume)
    {
      mat_buf.volumeThicknessFactor = vl_config.thickness_factor;

      // matBuf.volumeAbsorption = 0;

      if (program_key.hasVolumeThicknessTexture)
      {
        mat_textures.volumeThicknessTexture =
          create_texture(*vl_config.thickness_texture.texture, false);
        if (program_key.hasTextureTransforms)
        {
          mat_buf.volumeThicknessUvMatrix =
            matrix_from_uv_transform(vl_config.thickness_texture.transform);
        }
      }
    }

    if (program_key.hasIOR)
    {
      mat_buf.ior = src_material->ior.ior;
    }

    if (program_key.hasTransmission)
    {
      mat_buf.transmissionFactor = tr_config.transmission_factor;
      if (program_key.hasTransmissionTexture)
      {
        mat_textures.transmissionTexture =
          create_texture(*tr_config.transmission_texture.texture, false);
        if (program_key.hasTextureTransforms)
        {
          mat_buf.transmissionUvMatrix =
            matrix_from_uv_transform(tr_config.transmission_texture.transform);
        }
      }
    }

    mat_buf._specularAntiAliasingThreshold = 0.04f;
    mat_buf._specularAntiAliasingVariance  = 0.15f;
    mat_buf._maskThreshold                 = src_material->alpha_cutoff;

    mat_cache_.insert(key, {material_id, uvmap});

    return material_id;
  }

  TextureID GLTFImporter::create_texture(const cgltf_texture& src_texture, const bool srgb)
  {
    SOUL_PROFILE_ZONE_WITH_NAME("Create Texture");
    TexCacheKey key = {&src_texture, srgb};
    if (tex_cache_.contains(key))
    {
      return tex_cache_[key];
    }

    TextureID tex_id = scene_.create_texture();
    tex_cache_.insert(key, tex_id);
    tex_key_list_.push_back(key);
    return tex_id;
  }

  void GLTFImporter::import_textures()
  {
    const soul::runtime::TaskID create_gpu_textures_parent = soul::runtime::create_task();
    for (const auto tex_key : tex_key_list_)
    {
      const cgltf_texture& src_texture = *tex_key.gltfTexture;
      const TextureID tex_id           = tex_cache_[tex_key];
      runtime::create_and_run_task(
        create_gpu_textures_parent,
        [this, &src_texture, tex_id](runtime::TaskID)
        {
          Texture* texture = scene_.get_texture_ptr(tex_id);

          auto load_texels =
            [this](const cgltf_texture& src_texture) -> std::tuple<const byte*, vec2u32, uint32>
          {
            soul::runtime::ScopeAllocator<> scope_allocator("Loading texture"_str);
            const cgltf_buffer_view* bv = src_texture.image->buffer_view;
            const auto data = soul::cast<const uint8**>(bv ? &bv->buffer->data : nullptr);

            int width, height, comp;
            auto total_size = soul::cast<uint32>(bv ? bv->size : 0);
            if (data)
            {
              const uint64 offset     = bv ? bv->offset : 0;
              const uint8* sourceData = offset + *data;
              const byte* texels      = stbi_load_from_memory(
                sourceData, soul::cast<int>(total_size), &width, &height, &comp, 4);
              total_size = width * height * 4;
              SOUL_ASSERT(0, texels != nullptr, "Fail to load texels");
              return {
                texels, vec2u32(soul::cast<uint32>(width), soul::cast<uint32>(height)), total_size};
            } else
            {
              auto uri_path = soul::cast<char*>(scope_allocator.allocate(
                strlen(gltf_path_) + GLTF_URI_MAX_LENGTH + 1, alignof(char)));
              compute_uri_path(uri_path, gltf_path_, src_texture.image->uri);
              const byte* texels = stbi_load(uri_path, &width, &height, &comp, 4);
              SOUL_ASSERT(0, texels != nullptr, "Fail to load texels");
              total_size = width * height * 4;
              return {
                texels, vec2u32(soul::cast<uint32>(width), soul::cast<uint32>(height)), total_size};
            }
          };
          const auto [texels, extent, total_size] = load_texels(src_texture);

          const auto mip_levels =
            std::max<uint16>(soul::cast<uint16>(floorLog2(std::max(extent.x, extent.y))), 1);
          const auto tex_desc = gpu::TextureDesc::D2(
            "",
            gpu::TextureFormat::RGBA8,
            mip_levels,
            {gpu::TextureUsage::SAMPLED},
            {gpu::QueueType::GRAPHIC},
            extent);

          constexpr soul::gpu::SamplerDesc default_sampler = soul::gpu::SamplerDesc::SameFilterWrap(
            soul::gpu::TextureFilter::LINEAR, soul::gpu::TextureWrap::REPEAT);
          const soul::gpu::SamplerDesc sampler_desc = src_texture.sampler != nullptr
                                                        ? get_sampler_desc(*src_texture.sampler)
                                                        : default_sampler;

          gpu::TextureRegionLoad region_load = {
            .textureRegion = {
              .offset         = {0, 0, 0},
              .extent         = tex_desc.extent,
              .mipLevel       = 0,
              .baseArrayLayer = 0,
              .layerCount     = 1}};

          const gpu::TextureLoadDesc load_desc = {
            .data            = texels,
            .dataSize        = total_size,
            .regionLoadCount = 1,
            .regionLoads     = &region_load,
            .generateMipmap  = true

          };

          texture->gpuHandle = gpu_system_.create_texture(tex_desc, load_desc);
          SOUL_ASSERT(0, !texture->gpuHandle.is_null());
          texture->samplerDesc = sampler_desc;
          stbi_image_free(soul::cast<void*>(texels));
          gpu_system_.finalize_texture(texture->gpuHandle, {gpu::TextureUsage::SAMPLED});
        });
    }
    soul::runtime::run_and_wait_task(create_gpu_textures_parent);
  }

  void GLTFImporter::import_entities()
  {
    std::for_each_n(
      asset_->nodes,
      asset_->nodes_count,
      [this](const cgltf_node& node)
      {
        create_entity(&node);
      });
  }

  EntityID GLTFImporter::create_entity(const cgltf_node* node)
  {
    const CGLTFNodeKey node_key(node);
    if (node_map_.contains(node_key))
    {
      return node_map_[node_key];
    }
    const EntityID entity = scene_.create_entity(get_node_name(node, "Unnamed"));
    node_map_.insert(node_key, entity);

    const soul::mat4f32 local_transform = [](const cgltf_node* node)
    {
      if (node->has_matrix)
      {
        return mat4Transpose(soul::mat4(node->matrix));
      } else
      {
        const soul::vec3f32 translation(node->translation);
        const soul::vec3f32 scale(node->scale);
        const soul::Quaternionf rotation(node->rotation);
        return soul::mat4Transform(soul::Transformf{translation, scale, rotation});
      }
    }(node);

    const EntityID parent = node->parent ? create_entity(node->parent) : scene_.get_root_entity();

    auto& parent_transform        = scene_.get_component<TransformComponent>(parent);
    const mat4f32 world_transform = parent_transform.world * local_transform;
    const EntityID next_entity    = parent_transform.firstChild;
    parent_transform.firstChild   = entity;
    if (next_entity != ENTITY_ID_NULL)
    {
      auto& next_transform = scene_.get_component<TransformComponent>(next_entity);
      next_transform.prev  = entity;
    }

    scene_.add_component<TransformComponent>(
      entity,
      {local_transform, world_transform, parent, ENTITY_ID_NULL, next_entity, ENTITY_ID_NULL});

    if (node->mesh)
    {
      create_renderable(entity, node);
    }
    if (node->light)
    {
      create_light(entity, node);
    }
    if (node->camera)
    {
      create_camera(entity, node);
    }
    return entity;
  }

  void GLTFImporter::create_renderable(const EntityID entity, const cgltf_node* node)
  {

    Visibility visibility;
    visibility.priority       = 0x4;
    visibility.castShadows    = true;
    visibility.receiveShadows = true;
    visibility.culling        = true;

    SOUL_ASSERT(0, node->mesh != nullptr);

    cgltf_mesh& src_mesh = *node->mesh;
    const auto mesh_id   = MeshID(node->mesh - asset_->meshes);

    SOUL_ASSERT(0, src_mesh.primitives_count > 0);

    cgltf_size num_morph_targets = src_mesh.primitives[0].targets_count;
    visibility.morphing          = num_morph_targets > 0;

    visibility.screenSpaceContactShadows = false;

    soul::vec4f32 morph_weights;
    if (num_morph_targets > 0)
    {
      std::copy_n(
        src_mesh.weights, std::min(MAX_MORPH_TARGETS, src_mesh.weights_count), morph_weights.mem);
      std::copy_n(
        node->weights, std::min(MAX_MORPH_TARGETS, node->weights_count), morph_weights.mem);
    }

    const SkinID skin_id = node->skin != nullptr ? SkinID(node->skin - asset_->skins) : SkinID();
    visibility.skinning  = skin_id.is_null() ? false : true;

    scene_.add_component<RenderComponent>(
      entity, {visibility, mesh_id, skin_id, morph_weights, uint8(0x1)});
  }

  void GLTFImporter::create_camera(const EntityID entity_id, const cgltf_node* node)
  {
    CameraComponent& camera_component = scene_.add_component<CameraComponent>(entity_id, {});

    SOUL_ASSERT(0, node->camera != nullptr);
    const cgltf_camera& src_camera = *node->camera;

    if (src_camera.type == cgltf_camera_type_perspective)
    {
      const cgltf_camera_perspective& src_perspective = src_camera.data.perspective;
      const float far = src_perspective.zfar > 0.0f ? src_perspective.zfar : 10000000;
      camera_component.setPerspectiveProjection(
        src_perspective.yfov, src_perspective.aspect_ratio, src_perspective.znear, far);
    } else if (src_camera.type == cgltf_camera_type_orthographic)
    {
      const cgltf_camera_orthographic& src_orthographic = src_camera.data.orthographic;
      const float left                                  = -src_orthographic.xmag * 0.5f;
      const float right                                 = src_orthographic.xmag * 0.5f;
      const float bottom                                = -src_orthographic.ymag * 0.5f;
      const float top                                   = src_orthographic.ymag * 0.5f;
      camera_component.setOrthoProjection(
        left, right, bottom, top, src_orthographic.znear, src_orthographic.zfar);
    } else
    {
      SOUL_NOT_IMPLEMENTED();
    }
  }

  void GLTFImporter::create_light(const EntityID entity_id, const cgltf_node* node)
  {
    SOUL_ASSERT(0, node->light != nullptr);
    const cgltf_light& light = *node->light;

    const LightType light_type(get_light_type(light.type), true, true);
    constexpr soul::vec3f32 direction(0.0f, 0.0f, -1.0f);
    const soul::vec3f32 color(light.color[0], light.color[1], light.color[2]);
    const float falloff      = light.range == 0.0f ? 10.0f : light.range;
    float luminous_power     = light.intensity;
    float luminous_intensity = 0.0f;

    SpotParams spot_params;

    if (
      light_type.type == LightRadiationType::SPOT ||
      light_type.type == LightRadiationType::FOCUSED_SPOT)
    {
      const float inner_clamped = std::min(std::abs(light.spot_inner_cone_angle), M_PI_2);

      float outer_clamped = std::min(std::abs(light.spot_outer_cone_angle), M_PI_2);
      // outer must always be bigger than inner
      outer_clamped       = std::max(inner_clamped, outer_clamped);

      const float cos_outer         = std::cos(outer_clamped);
      const float cos_inner         = std::cos(inner_clamped);
      const float cos_outer_squared = cos_outer * cos_outer;
      const float scale             = 1 / std::max(1.0f / 1024.0f, cos_inner - cos_outer);
      const float offset            = -cos_outer * scale;

      spot_params.outerClamped    = outer_clamped;
      spot_params.cosOuterSquared = cos_outer_squared;
      spot_params.sinInverse      = 1 / std::sqrt(1 - cos_outer_squared);
      spot_params.scaleOffset     = {scale, offset};
    }

    switch (light_type.type)
    {
    case LightRadiationType::SUN:
    case LightRadiationType::DIRECTIONAL:
      // luminousPower is in lux, nothing to do.
      luminous_intensity = luminous_power;
      break;

    case LightRadiationType::POINT:
      luminous_intensity = luminous_power * f32const::ONE_OVER_PI * 0.25f;
      break;

    case LightRadiationType::FOCUSED_SPOT:
    {

      float cos_outer           = std::sqrt(spot_params.cosOuterSquared);
      // intensity specified directly in candela, no conversion needed
      luminous_intensity        = luminous_power;
      // lp = li * (2 * pi * (1 - cos(cone_outer / 2)))
      luminous_power            = luminous_intensity * (f32const::TAU * (1.0f - cos_outer));
      spot_params.luminousPower = luminous_power;
      break;
    }
    case LightRadiationType::SPOT: luminous_intensity = luminous_power; break;
    case LightRadiationType::COUNT:
    default: SOUL_NOT_IMPLEMENTED(); break;
    }
    scene_.add_component<LightComponent>(
      entity_id,
      {light_type,
       soul::vec3f32(0, 0, 0),
       direction,
       color,
       ShadowParams(),
       spot_params,
       0.0f,
       0.0f,
       0.0f,
       luminous_intensity,
       falloff});
  }

  AnimationSampler create_animation_sampler(const cgltf_animation_sampler& src_sampler)
  {
    AnimationSampler dst_sampler;
    const cgltf_accessor& timeline_accessor = *src_sampler.input;
    auto timeline_blob   = soul::cast<const byte*>(timeline_accessor.buffer_view->buffer->data);
    auto timeline_floats = soul::cast<const float*>(
      timeline_blob + timeline_accessor.offset + timeline_accessor.buffer_view->offset);

    dst_sampler.times.resize(timeline_accessor.count);
    memcpy(dst_sampler.times.data(), timeline_floats, timeline_accessor.count * sizeof(float));

    const cgltf_accessor& values_accessor = *src_sampler.output;
    switch (values_accessor.type)
    {
    case cgltf_type_scalar:
      dst_sampler.values.resize(values_accessor.count);
      cgltf_accessor_unpack_floats(
        src_sampler.output, dst_sampler.values.data(), values_accessor.count);
      break;
    case cgltf_type_vec3:
      dst_sampler.values.resize(values_accessor.count * 3);
      cgltf_accessor_unpack_floats(
        src_sampler.output, dst_sampler.values.data(), values_accessor.count * 3);
      break;
    case cgltf_type_vec4:
      dst_sampler.values.resize(values_accessor.count * 4);
      cgltf_accessor_unpack_floats(
        src_sampler.output, dst_sampler.values.data(), values_accessor.count * 4);
      break;
    case cgltf_type_vec2:
    case cgltf_type_mat2:
    case cgltf_type_mat3:
    case cgltf_type_mat4:
    case cgltf_type_invalid:
    default: SOUL_LOG_WARN("Unknown animation type.");
    }

    switch (src_sampler.interpolation)
    {
    case cgltf_interpolation_type_linear:
      dst_sampler.interpolation = AnimationSampler::LINEAR;
      break;
    case cgltf_interpolation_type_step: dst_sampler.interpolation = AnimationSampler::STEP; break;
    case cgltf_interpolation_type_cubic_spline:
      dst_sampler.interpolation = AnimationSampler::CUBIC;
      break;
    }
    return dst_sampler;
  }

  void GLTFImporter::import_animations()
  {
    SOUL_PROFILE_ZONE();
    auto create_animation = [this](const cgltf_animation& src_animation)
    {
      Animation dst_anim;
      std::transform(
        src_animation.samplers,
        src_animation.samplers + src_animation.samplers_count,
        std::back_inserter(dst_anim.samplers),
        create_animation_sampler);

      auto create_animation_channel =
        [this, &dst_anim, &src_animation](
          const cgltf_animation_channel& src_channel) -> AnimationChannel
      {
        AnimationChannel dst_channel = {};
        dst_channel.samplerIdx = soul::cast<uint32>(src_channel.sampler - src_animation.samplers);
        dst_channel.entity     = node_map_[CGLTFNodeKey(src_channel.target_node)];
        switch (src_channel.target_path)
        {
        case cgltf_animation_path_type_translation:
          dst_channel.transformType = AnimationChannel::TRANSLATION;
          break;
        case cgltf_animation_path_type_rotation:
          dst_channel.transformType = AnimationChannel::ROTATION;
          break;
        case cgltf_animation_path_type_scale:
          dst_channel.transformType = AnimationChannel::SCALE;
          break;
        case cgltf_animation_path_type_weights:
          dst_channel.transformType = AnimationChannel::WEIGHTS;
          break;
        case cgltf_animation_path_type_invalid: SOUL_LOG_WARN("Unsupported channel path."); break;
        }
        float channel_duration = dst_anim.samplers[dst_channel.samplerIdx].times.back();
        dst_anim.duration      = std::max(dst_anim.duration, channel_duration);
        return dst_channel;
      };

      dst_anim.duration = 0;
      std::transform(
        src_animation.channels,
        src_animation.channels + src_animation.channels_count,
        std::back_inserter(dst_anim.channels),
        create_animation_channel);
      dst_anim.name = src_animation.name;
      return dst_anim;
    };

    scene_.create_animations_parallel(
      asset_->animations_count,
      [this, create_animation](soul_size anim_index, Animation& dst_animation)
      {
        dst_animation = create_animation(asset_->animations[anim_index]);
      });
  }

  void GLTFImporter::import_skins()
  {
    SOUL_PROFILE_ZONE();
    std::for_each_n(
      asset_->skins,
      asset_->skins_count,
      [this](const cgltf_skin& src_skin)
      {
        SkinID skin_id = scene_.create_skin();
        Skin& dst_skin = *scene_.get_skin_ptr(skin_id);
        if (src_skin.name)
        {
          dst_skin.name = src_skin.name;
        }

        dst_skin.invBindMatrices.resize(src_skin.joints_count);
        dst_skin.joints.resize(src_skin.joints_count);
        dst_skin.bones.resize(src_skin.joints_count);

        if (const cgltf_accessor* srcMatrices = src_skin.inverse_bind_matrices)
        {
          const auto dst_matrices = soul::cast<uint8*>(dst_skin.invBindMatrices.data());
          const auto bytes        = soul::cast<uint8*>(srcMatrices->buffer_view->buffer->data);
          if (!bytes)
          {
            SOUL_NOT_IMPLEMENTED();
          }
          const auto src_buffer =
            soul::cast<void*>(bytes + srcMatrices->offset + srcMatrices->buffer_view->offset);
          memcpy(dst_matrices, src_buffer, src_skin.joints_count * sizeof(soul::mat4f32));
          for (soul::mat4f32& matrix : dst_skin.invBindMatrices)
          {
            matrix = mat4Transpose(matrix);
          }
        } else
        {
          for (soul::mat4f32& matrix : dst_skin.invBindMatrices)
          {
            matrix = mat4Identity();
          }
        }

        for (cgltf_size jointIdx = 0; jointIdx < src_skin.joints_count; jointIdx++)
        {
          dst_skin.joints[jointIdx] = node_map_[CGLTFNodeKey(src_skin.joints[jointIdx])];
        }
      });
  }

} // namespace soul_fila
