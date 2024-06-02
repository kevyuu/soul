#include <limits>
#include <ranges>

#include "core/vec.h"
#include "gltf_importer.h"

#include "core/config.h"
#include "core/log.h"
#include "core/panic.h"
#include "core/string.h"
#include "core/util.h"
#include "gpu/type.h"
#include "math/aabb.h"
#include "misc/image_data.h"
#include "runtime/runtime.h"
#include "runtime/scope_allocator.h"

#include "scene.h"
#include "type.h"
#include "type.shared.hlsl"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

constexpr u32 GLTF_URI_MAX_LENGTH = 1000;

namespace renderlab
{
  constexpr auto GLTF_FRONT_FACE = gpu::FrontFace::COUNTER_CLOCKWISE;

  namespace
  {
    void cgltf_combine_paths(char* path, const char* base, const char* uri)
    {
      const char* s0    = strrchr(base, '/');
      const char* s1    = strrchr(base, '\\');
      const char* slash = (s0 != nullptr) ? ((s1 != nullptr) && s1 > s0 ? s1 : s0) : s1;

      if (slash != nullptr)
      {
        size_t prefix = slash - base + 1;

        strncpy_s(path, GLTF_URI_MAX_LENGTH, base, prefix);
        strcpy_s(path + prefix, GLTF_URI_MAX_LENGTH, uri);
      } else
      {
        strcpy_s(path, GLTF_URI_MAX_LENGTH, uri);
      }
    }

    void compute_uri_path(char* uri_path, const char* gltf_path, const char* uri)
    {
      cgltf_combine_paths(uri_path, gltf_path, uri);

      // after combining, the tail of the resulting path is a uri; decode_uri converts it into path
      cgltf_decode_uri(uri_path + strlen(uri_path) - strlen(uri));
    }

    auto get_vertex_count(const cgltf_primitive& primitive) -> usize
    {
      return primitive.attributes[0].data->count;
    }

    // Sometimes a glTF bufferview includes unused data at the end (e.g. in skinning.gltf) so we
    // need to compute the correct size of the vertex buffer. Filament automatically infers the size
    // of driver-level vertex buffers from the attribute data (stride, count, offset) and clients
    // are expected to avoid uploading data blobs that exceed this size. Since this information
    // doesn't exist in the glTF we need to compute it manually. This is a bit of a cheat,
    // cgltf_calc_size is private but its implementation file is available in this cpp file.
    auto compute_binding_size(const cgltf_accessor& accessor) -> u32
    {
      const cgltf_size element_size = cgltf_calc_size(accessor.type, accessor.component_type);
      return cast<u32>(accessor.stride * (accessor.count - 1) + element_size);
    }

    auto compute_binding_offset(const cgltf_accessor& accessor) -> u32
    {
      return cast<u32>(accessor.offset + accessor.buffer_view->offset);
    }

    auto try_get_topology(const cgltf_primitive_type in) -> Option<gpu::Topology>
    {
      using namespace soul::gpu;
      switch (in)
      { // NOLINT
      case cgltf_primitive_type_points: return someopt(Topology::POINT_LIST);
      case cgltf_primitive_type_lines: return someopt(Topology::LINE_LIST);
      case cgltf_primitive_type_triangles: return someopt(Topology::TRIANGLE_LIST);
      case cgltf_primitive_type_line_loop:
      case cgltf_primitive_type_line_strip:
      case cgltf_primitive_type_triangle_strip:
      case cgltf_primitive_type_triangle_fan: return nilopt;
      }
      return nilopt;
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

    auto get_wrap_mode(cgltf_int wrap) -> soul::gpu::TextureWrap
    {
      switch (wrap)
      {
      case GL_REPEAT: return soul::gpu::TextureWrap::REPEAT;
      case GL_MIRRORED_REPEAT: return soul::gpu::TextureWrap::MIRRORED_REPEAT;
      case GL_CLAMP_TO_EDGE: return soul::gpu::TextureWrap::CLAMP_TO_EDGE;
      default: return soul::gpu::TextureWrap::REPEAT;
      }
    }

    auto get_sampler_desc(const cgltf_sampler& srcSampler) -> soul::gpu::SamplerDesc
    {
      soul::gpu::SamplerDesc res;
      res.wrap_u = get_wrap_mode(srcSampler.wrap_s);
      res.wrap_v = get_wrap_mode(srcSampler.wrap_t);
      switch (srcSampler.min_filter)
      {
      case GL_NEAREST: res.min_filter = soul::gpu::TextureFilter::NEAREST; break;
      case GL_LINEAR: res.min_filter = soul::gpu::TextureFilter::LINEAR; break;
      case GL_NEAREST_MIPMAP_NEAREST:
        res.min_filter    = soul::gpu::TextureFilter::NEAREST;
        res.mipmap_filter = soul::gpu::TextureFilter::NEAREST;
        break;
      case GL_LINEAR_MIPMAP_NEAREST:
        res.min_filter    = soul::gpu::TextureFilter::LINEAR;
        res.mipmap_filter = soul::gpu::TextureFilter::NEAREST;
        break;
      case GL_NEAREST_MIPMAP_LINEAR:
        res.min_filter    = soul::gpu::TextureFilter::NEAREST;
        res.mipmap_filter = soul::gpu::TextureFilter::LINEAR;
        break;
      case GL_LINEAR_MIPMAP_LINEAR:
      default:
        res.min_filter    = soul::gpu::TextureFilter::LINEAR;
        res.mipmap_filter = soul::gpu::TextureFilter::LINEAR;
        break;
      }

      switch (srcSampler.mag_filter)
      {
      case GL_NEAREST: res.mag_filter = soul::gpu::TextureFilter::NEAREST; break;
      case GL_LINEAR:
      default: res.mag_filter = soul::gpu::TextureFilter::LINEAR; break;
      }
      return res;
    }

    struct AttributeBuffer
    {
      const byte* data;
      usize data_count;
      usize stride;
      usize type_size;
      usize type_alignment;
    };

    auto create_attribute_buffer(
      NotNull<memory::Allocator*> allocator,
      const cgltf_attribute& src_attribute,
      const cgltf_accessor& accessor) -> AttributeBuffer
    {
      if (
        (accessor.is_sparse != 0) || src_attribute.type == cgltf_attribute_type_tangent ||
        src_attribute.type == cgltf_attribute_type_normal ||
        src_attribute.type == cgltf_attribute_type_position)
      {

        const cgltf_size num_floats = accessor.count * cgltf_num_components(accessor.type);

        f32* generated = allocator->allocate_array<f32>(num_floats);
        cgltf_accessor_unpack_floats(&accessor, generated, num_floats);
        usize type_size = cgltf_num_components(accessor.type) * sizeof(f32);

        return {cast<byte*>(generated), accessor.count, type_size, type_size, sizeof(f32)};
      }

      auto buffer_data = cast<const byte*>(accessor.buffer_view->buffer->data);
      return {
        buffer_data + compute_binding_offset(accessor),
        accessor.count,
        accessor.stride,
        cgltf_calc_size(accessor.type, accessor.component_type),
        cgltf_component_size(accessor.component_type)};
    }

    struct AttributeData
    {
      vec3f32* positions  = nullptr;
      vec3f32* normals    = nullptr;
      vec4f32* tangents   = nullptr;
      vec2f32* tex_coords = nullptr;
      usize vertex_count  = 0;
      math::AABB aabb;
    };

    auto create_attribute_data(
      const cgltf_primitive& src_primitive, NotNull<soul::memory::Allocator*> allocator)
      -> AttributeData
    {
      usize vertex_count = get_vertex_count(src_primitive);
      AttributeData attribute_data;
      attribute_data.vertex_count = vertex_count;
      for (cgltf_size attr_index = 0; attr_index < src_primitive.attributes_count; attr_index++)
      {
        const cgltf_attribute& src_attribute = src_primitive.attributes[attr_index];
        const cgltf_accessor& accessor       = *src_attribute.data;

        AttributeBuffer attribute_buffer =
          create_attribute_buffer(allocator, src_attribute, accessor);

        if (src_attribute.type == cgltf_attribute_type_position)
        {

          SOUL_ASSERT(0, sizeof(vec3f32) == attribute_buffer.stride);
          attribute_data.positions = cast<vec3f32*>(attribute_buffer.data);
          attribute_data.aabb =
            math::AABB(vec3f32::FromData(accessor.min), vec3f32::FromData(accessor.max));

        } else if (src_attribute.type == cgltf_attribute_type_tangent)
        {

          SOUL_ASSERT(0, sizeof(vec4f32) == attribute_buffer.stride);
          attribute_data.tangents = cast<vec4f32*>(attribute_buffer.data);

        } else if (src_attribute.type == cgltf_attribute_type_normal)
        {

          SOUL_ASSERT(0, sizeof(vec3f32) == attribute_buffer.stride);
          attribute_data.normals = cast<vec3f32*>(attribute_buffer.data);

        } else if (src_attribute.type == cgltf_attribute_type_texcoord && src_attribute.index == 0)
        {

          SOUL_ASSERT(
            src_attribute.index == 0,
            "Texture coordinate index other than 0 is not supported yet!");
          const cgltf_size num_floats = accessor.count * cgltf_num_components(accessor.type);
          auto generated              = allocator->allocate_array<float>(num_floats);
          cgltf_accessor_unpack_floats(&accessor, generated, num_floats);
          attribute_data.tex_coords = cast<vec2f32*>(generated);
        }
      }

      return attribute_data;
    }

    auto create_static_vertexes(
      const cgltf_primitive& src_primitive, NotNull<memory::Allocator*> allocator)
      -> soul::Vector<StaticVertexData>
    {
      const auto attribute_data     = create_attribute_data(src_primitive, allocator);
      const auto vertex_count       = get_vertex_count(src_primitive);
      auto static_vertex_from_index = [=](usize i) -> StaticVertexData
      {
        return {
          .position  = attribute_data.positions[i],
          .normal    = attribute_data.normals[i],
          .tangent   = attribute_data.tangents[i],
          .tex_coord = attribute_data.tex_coords[i],
        };
      };
      return soul::Vector<StaticVertexData>::TransformIndex(
        0ull, vertex_count, static_vertex_from_index, allocator);
    }

    template <typename DstT, typename SrcT>
    auto create_index_buffer(const cgltf_accessor& indices, NotNull<memory::Allocator*> allocator)
      -> IndexData
    {
      using Index   = DstT;
      using Indexes = Vector<Index>;

      const auto buffer_data_raw =
        cast<const byte*>(indices.buffer_view->buffer->data) + compute_binding_offset(indices);
      const auto buffer_data = cast<const SrcT*>(buffer_data_raw);

      SOUL_ASSERT(0, indices.stride % sizeof(SrcT) == 0, "Stride must be multiple of source type.");
      const u64 index_stride = indices.stride / sizeof(SrcT);
      auto transform_fn      = [=](usize i)
      {
        return Index(*(buffer_data + index_stride * i));
      };
      return Indexes::TransformIndex(0ull, indices.count, transform_fn, allocator);
    }

    auto create_index_data_from_primitive(
      const cgltf_primitive& src_primitive, NotNull<memory::Allocator*> allocator) -> IndexData
    {
      using IndexType = u32;
      if (src_primitive.indices != nullptr)
      {

        const cgltf_accessor& src_indices = *src_primitive.indices;

        switch (src_primitive.indices->component_type)
        {
        case cgltf_component_type_r_8u:
        {
          return create_index_buffer<u16, u8>(src_indices, allocator);
        }
        case cgltf_component_type_r_16u:
        {
          return create_index_buffer<u16, u16>(src_indices, allocator);
        }
        case cgltf_component_type_r_32u:
        {
          return create_index_buffer<u32, u32>(src_indices, allocator);
        }
        case cgltf_component_type_r_8:
        case cgltf_component_type_r_16:
        case cgltf_component_type_r_32f:
        case cgltf_component_type_invalid:
        default: SOUL_NOT_IMPLEMENTED(); return soul::Vector<u16>(allocator);
        }
      };

      if (src_primitive.attributes_count > 0)
      {

        const usize vertex_count = src_primitive.attributes[0].data->count;
        if (vertex_count < std::numeric_limits<u16>::max())
        {
          return Vector<u16>::From(std::views::iota(u16(0), vertex_count), allocator);
        } else
        {
          return Vector<u32>::From(std::views::iota(u32(0), vertex_count), allocator);
        }
      }

      return Vector<u16>(allocator);
    };

    auto create_primitive_name(const char* mesh_name, usize primitive_idx) -> String
    {
      if (mesh_name != nullptr)
      {
        if (primitive_idx == 0)
        {
          return String::From(mesh_name);
        } else
        {
          return String::Format("{}_{}", mesh_name, primitive_idx);
        }
      } else
      {
        return String::From("Unnamed");
      }
    };

    auto get_entity_name(const cgltf_node& entity, StringView default_entity_name) -> StringView
    {
      if (entity.name != nullptr)
      {
        return StringView(entity.name);
      }
      if ((entity.mesh != nullptr) && (entity.mesh->name != nullptr))
      {
        return StringView(entity.mesh->name);
      }
      if ((entity.light != nullptr) && (entity.light->name != nullptr))
      {
        return StringView(entity.light->name);
      }
      if ((entity.camera != nullptr) && (entity.camera->name != nullptr))
      {
        return StringView(entity.camera->name);
      }
      return default_entity_name;
    }

  } // namespace

  auto GLTFImporter::get_or_create_material_texture(
    const cgltf_texture* src_texture, gpu::TextureFormat format, ImportContext import_context)
    -> MaterialTextureID
  {
    if (src_texture == nullptr)
    {
      return MaterialTextureID();
    }
    const auto cgltf_texture_index = src_texture - import_context.asset->textures;
    auto& texture_data             = texture_map_[cgltf_texture_index];
    SOUL_ASSERT(0, format == gpu::TextureFormat::RGBA8 || format == gpu::TextureFormat::SRGBA8);
    if (!texture_data.id.is_null())
    {
      SOUL_ASSERT(
        0,
        texture_data.format == format,
        "Cannot create same texture data with multiple format yet!");
      return texture_data.id;
    };

    struct TextureSource
    {
      const byte* data = nullptr;
      vec2u32 dimension;
      u32 total_data_size = 0;
    };

    auto load_texels = [import_context](const cgltf_texture& src_texture) -> ImageData
    {
      soul::runtime::ScopeAllocator<> scope_allocator("Loading texture"_str);
      const cgltf_buffer_view* bv = src_texture.image->buffer_view;
      const auto data = soul::cast<const u8**>(bv != nullptr ? &bv->buffer->data : nullptr);

      int width, height, comp; // NOLINT
      auto total_size = soul::cast<u64>(bv != nullptr ? bv->size : 0);
      if (data != nullptr)
      {
        const u64 offset        = bv != nullptr ? bv->offset : 0;
        const byte* source_data = offset + *data;
        return ImageData::FromRawBytes({source_data, total_size}, 4);
      } else
      {
        auto uri_path =
          soul::cast<char*>(scope_allocator.allocate(GLTF_URI_MAX_LENGTH + 1, alignof(char)));
        compute_uri_path(uri_path, import_context.path->string().data(), src_texture.image->uri);
        return ImageData::FromFile(Path::From(StringView(uri_path)), 4);
      }
    };
    const auto image_data = load_texels(*src_texture);

    const auto tex_desc = MaterialTextureDesc{
      .name      = StringView(src_texture->name),
      .format    = format,
      .dimension = image_data.dimension(),
      .data      = image_data.cdata(),
    };
    const auto texture_id             = import_context.scene->create_material_texture(tex_desc);
    texture_map_[cgltf_texture_index] = {
      .id     = texture_id,
      .format = format,
    };
    return texture_id;
  }

  auto GLTFImporter::create_material(
    const cgltf_material& src_material, ImportContext import_context) -> MaterialID
  {
    SOUL_ASSERT(
      src_material.has_pbr_metallic_roughness,
      "Currently only supported metallic roughness material");

    const auto mr_config = src_material.pbr_metallic_roughness;
    MaterialDesc material_desc;
    material_desc.name =
      src_material.name == nullptr ? "Unnamed"_str : StringView(src_material.name);

    material_desc.base_color_factor = vec4f32(
      mr_config.base_color_factor[0],
      mr_config.base_color_factor[1],
      mr_config.base_color_factor[2],
      mr_config.base_color_factor[3]);
    material_desc.roughness_factor = mr_config.roughness_factor;
    material_desc.metallic_factor  = mr_config.metallic_factor;
    const auto emissive_strength   = src_material.has_emissive_strength != 0
                                       ? src_material.emissive_strength.emissive_strength
                                       : 0;
    SOUL_LOG_INFO(
      "Emissive material, Name : {}, strength : {}", material_desc.name, emissive_strength);
    material_desc.emissive_factor =
      vec3f32::FromData(src_material.emissive_factor) * emissive_strength;

    cgltf_texture_view base_color_texture         = mr_config.base_color_texture;
    cgltf_texture_view metallic_roughness_texture = mr_config.metallic_roughness_texture;
    cgltf_texture_view normal_texture             = src_material.normal_texture;
    cgltf_texture_view emissive_texture           = src_material.emissive_texture;

    material_desc.base_color_texture_id = get_or_create_material_texture(
      base_color_texture.texture, gpu::TextureFormat::SRGBA8, import_context);
    material_desc.metallic_roughness_texture_id = get_or_create_material_texture(
      metallic_roughness_texture.texture, gpu::TextureFormat::RGBA8, import_context);
    material_desc.normal_texture_id = get_or_create_material_texture(
      normal_texture.texture, gpu::TextureFormat::RGBA8, import_context);
    material_desc.emissive_texture_id = get_or_create_material_texture(
      emissive_texture.texture, gpu::TextureFormat::SRGBA8, import_context);

    const auto cgltf_material_index = cast<u32>(&src_material - import_context.asset->materials);
    const auto material_id          = import_context.scene->create_material(material_desc);
    material_map_[cgltf_material_index] = material_id;
    return material_id;
  };

  auto GLTFImporter::create_mesh_group(const cgltf_mesh& src_mesh, ImportContext import_context)
    -> MeshGroupID
  {
    runtime::ScopeAllocator scope_allocator("GLTFImporter::create_mesh"_str);
    auto get_mesh_desc = [&, import_context](const cgltf_primitive& src_primitive) -> MeshDesc
    {
      const auto index_data     = create_index_data_from_primitive(src_primitive, &scope_allocator);
      const auto attribute_data = create_attribute_data(src_primitive, &scope_allocator);

      static_assert(ts_copy<u16IndexSpan>);
      static_assert(ts_copy<u32IndexSpan>);
      const auto indexes = index_data.visit(
        [](const auto& data) -> IndexSpan
        {
          return data.cspan();
        });
      SOUL_ASSERT(0, src_primitive.material != nullptr);
      const auto cgltf_material_index =
        cast<u32>(src_primitive.material - import_context.asset->materials);

      return MeshDesc{
        .topology     = try_get_topology(src_primitive.type).unwrap(),
        .front_face   = GLTF_FRONT_FACE,
        .vertex_count = attribute_data.vertex_count,
        .positions =
          {
            .data      = attribute_data.positions,
            .frequency = MeshDesc::AttributeFrequency::VERTEX,
          },
        .normals =
          {
            .data      = attribute_data.normals,
            .frequency = MeshDesc::AttributeFrequency::VERTEX,
          },
        .tangents =
          {
            .data      = attribute_data.tangents,
            .frequency = MeshDesc::AttributeFrequency::VERTEX,
          },
        .tex_coords =
          {
            .data      = attribute_data.tex_coords,
            .frequency = MeshDesc::AttributeFrequency::VERTEX,
          },
        .indexes     = indexes,
        .material_id = material_map_[cgltf_material_index],
        .aabb        = attribute_data.aabb,
      };
    };

    const auto mesh_descs = Vector<MeshDesc>::Transform(
      u32span(src_mesh.primitives, src_mesh.primitives_count), get_mesh_desc, scope_allocator);

    const MeshGroupDesc mesh_group_desc = {
      .name       = src_mesh.name == nullptr ? "Unnamed"_str : StringView(src_mesh.name),
      .mesh_descs = mesh_descs.span<u32>(),
    };
    const auto mesh_group_id         = import_context.scene->create_mesh_group(mesh_group_desc);
    const auto gltf_mesh_index       = &src_mesh - import_context.asset->meshes;
    mesh_group_map_[gltf_mesh_index] = mesh_group_id;
    return mesh_group_id;
  };

  auto GLTFImporter::create_entity(const cgltf_node& src_node, ImportContext import_context)
    -> EntityId
  {
    if (entity_map_.contains(&src_node))
    {
      return entity_map_[&src_node];
    }

    const auto parent_entity_id = src_node.parent == nullptr
                                    ? EntityId::Null()
                                    : create_entity(*src_node.parent, import_context);

    const auto local_transform = [](const cgltf_node& src_node) -> mat4f32
    {
      if (src_node.has_matrix != 0)
      {
        const auto transform = mat4f32::FromColumnMajorData(src_node.matrix);
        return transform;
      } else
      {
        const auto translation = vec3f32::FromData(src_node.translation);
        const auto rotation    = math::quatf32::FromData(src_node.rotation);
        const auto scale       = vec3f32::FromData(src_node.scale);
        const auto transform   = math::compose_transform(translation, rotation, scale);
        return transform;
      }
    }(src_node);

    EntityId entity_id = import_context.scene->create_entity({
      .name             = get_entity_name(src_node, "Unnamed"_str),
      .local_transform  = local_transform,
      .parent_entity_id = parent_entity_id,
    });

    entity_map_.insert(&src_node, entity_id);

    if (src_node.mesh != nullptr)
    {
      const auto mesh_group_index     = src_node.mesh - import_context.asset->meshes;
      const RenderComponent component = {
        .mesh_group_id = mesh_group_map_[mesh_group_index],
      };
      import_context.scene->add_render_component(entity_id, component);
    }

    return entity_id;
  }

  void GLTFImporter::import(const Path& asset_path, NotNull<Scene*> scene)
  {
    SOUL_PROFILE_ZONE();
    cgltf_options options = {};

    cgltf_data* asset     = nullptr;
    std::string gltf_path = asset_path.string();
    cgltf_result result   = cgltf_parse_file(&options, gltf_path.data(), &asset);
    SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf json");

    result = cgltf_load_buffers(&options, asset, gltf_path.data());
    SOUL_ASSERT(0, result == cgltf_result_success, "Fail to load gltf buffers");

    const cgltf_scene* src_scene = asset->scene != nullptr ? asset->scene : asset->scenes;
    if (src_scene == nullptr)
    {
      return;
    }

    texture_map_.clear();
    texture_map_.resize(asset->textures_count);
    material_map_.clear();
    material_map_.resize(asset->materials_count);
    mesh_group_map_.resize(asset->meshes_count);

    const auto import_context = ImportContext{
      .asset = asset,
      .path  = &asset_path,
      .scene = scene,
    };

    for (const auto& src_material : u64span(asset->materials, asset->materials_count))
    {
      SOUL_ASSERT(
        src_material.has_pbr_metallic_roughness,
        "Currently only supported metallic roughness material");
      create_material(src_material, import_context);
    }

    usize cgltf_primitive_count = 0;
    for (usize mesh_idx = 0; mesh_idx < asset->meshes_count; mesh_idx++)
    {
      const cgltf_mesh& mesh = asset->meshes[mesh_idx];
      cgltf_primitive_count += mesh.primitives_count;
    }

    // we map gltf primitive to a mesh, not gltf mesh, since our definition of mesh
    // is a collection of vertices that share the same material
    for (const auto& src_mesh : u64span(asset->meshes, asset->meshes_count))
    {
      create_mesh_group(src_mesh, import_context);
    }

    for (const auto& src_entity : u64span(asset->nodes, asset->nodes_count))
    {
      create_entity(src_entity, import_context);
    }

    cgltf_free(asset);
  };
} // namespace renderlab
