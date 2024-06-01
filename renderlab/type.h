#pragma once
#include "core/builtins.h"
#include "core/compiler.h"
#include "core/path.h"
#include "core/type.h"
#include "core/variant.h"

#include "gpu/sl_type.h"
#include "gpu/type.h"

using namespace soul::builtin;
#include "ecs.h"
#include "scene.hlsl"
#include "type.shared.hlsl"

using namespace soul;

namespace renderlab
{

  using MeshID            = ID<struct mesh_id_tag, u32>;
  using MeshGroupID       = ID<struct mesh_group_id_tag, u32>;
  using MaterialID        = ID<struct material_id_tag, u32>;
  using MaterialTextureID = ID<struct material_texture_id_tag, u32>;

  struct MaterialTextureDesc
  {
    StringView name           = ""_str;
    gpu::TextureFormat format = gpu::TextureFormat::COUNT;
    vec2u32 dimension;
    const byte* data = nullptr;
  };

  struct MaterialDesc
  {
    StringView name = ""_str;
    MaterialTextureID base_color_texture_id;
    MaterialTextureID metallic_roughness_texture_id;
    MaterialTextureID normal_texture_id;
    MaterialTextureID emissive_texture_id;

    vec4f32 base_color_factor;
    f32 metallic_factor     = 0.0f;
    f32 roughness_factor    = 0.0f;
    vec3f32 emissive_factor = vec3f32(0.0f);
  };

  using IndexData    = Variant<Vector<u16>, Vector<u32>>;
  using u16IndexSpan = Span<const u16*>;
  using u32IndexSpan = Span<const u32*>;
  using IndexSpan    = Variant<u16IndexSpan, u32IndexSpan>;

  struct MeshDesc
  {

    enum class AttributeFrequency
    {
      CONSTANT,
      UNIFORM,
      VERTEX,
      FACE_VARYING,
      COUNT
    };

    template <typename T>
    struct Attribute
    {
      const T* data                = nullptr;
      AttributeFrequency frequency = AttributeFrequency::COUNT;
    };

    gpu::Topology topology    = gpu::Topology::COUNT;
    gpu::FrontFace front_face = gpu::FrontFace::COUNT;

    usize vertex_count = 0;
    Attribute<vec3f32> positions;
    Attribute<vec3f32> normals;
    Attribute<vec4f32> tangents;
    Attribute<vec2f32> tex_coords;

    IndexSpan indexes;
    MaterialID material_id; // index in material array

    math::AABB aabb;

    [[nodiscard]]
    auto get_index_count() const -> usize
    {
      return indexes.visit(
        [](const auto& data) -> usize
        {
          return data.size();
        });
    }

    [[nodiscard]]
    auto get_face_count() const -> usize
    {
      return get_index_count() / 3;
    }

    [[nodiscard]]
    auto get_vertex_index(u32 face, u32 vert) const -> u32
    {
      return indexes.visit(
        [=](const auto& data) -> u32
        {
          return data[face * 3 + vert];
        });
    }

    template <typename T>
    [[nodiscard]]
    auto get_attribute_index(const Attribute<T>& attribute, u32 face, u32 vert) const -> u32
    {
      switch (attribute.frequency)
      {
      case AttributeFrequency::CONSTANT: return 0;
      case AttributeFrequency::UNIFORM: return face;
      case AttributeFrequency::VERTEX: return get_vertex_index(face, vert);
      case AttributeFrequency::FACE_VARYING: return face * 3 + vert;
      default: unreachable();
      }
    }

    template <typename T>
    [[nodiscard]]
    auto get(const Attribute<T>& attribute, u32 index) const -> T
    {
      if (attribute.data)
      {
        return attribute.data[index];
      }
      return T{};
    }

    template <typename T>
    [[nodiscard]]
    auto get(const Attribute<T>& attribute, u32 face, u32 vert) const -> T
    {
      if (attribute.data)
      {
        return get(attribute, get_attribute_index(attribute, face, vert));
      }
      return T{};
    }

    template <typename T>
    auto get_attribute_count(const Attribute<T>& attribute) -> usize
    {
      switch (attribute.frequency)
      {
      case AttributeFrequency::CONSTANT: return 1;
      case AttributeFrequency::UNIFORM: return get_face_count();
      case AttributeFrequency::VERTEX: return vertex_count;
      case AttributeFrequency::FACE_VARYING: return 3 * get_face_count();
      default: unreachable();
      }
      return 0;
    }

    [[nodiscard]]
    auto get_position(u32 face, u32 vert) const -> vec3f32
    {
      return get(positions, face, vert);
    }

    [[nodiscard]]
    auto get_normal(u32 face, u32 vert) const -> vec3f32
    {
      return get(normals, face, vert);
    }

    [[nodiscard]]
    auto get_tangent(u32 face, u32 vert) const -> vec4f32
    {
      return get(tangents, face, vert);
    }

    [[nodiscard]]
    auto get_tex_coord(u32 face, u32 vert) const -> vec2f32
    {
      return get(tex_coords, face, vert);
    }

    [[nodiscard]]
    auto get_static_vertex_data(u32 face, u32 vert) const -> StaticVertexData
    {
      return {
        .position  = get_position(face, vert),
        .normal    = get_normal(face, vert),
        .tangent   = get_tangent(face, vert),
        .tex_coord = get_tex_coord(face, vert),
      };
    }
  };

  struct MeshGroupDesc
  {
    StringView name;
    Span<const MeshDesc*, u32> mesh_descs;
  };

  struct Mesh
  {
    MeshInstanceFlags flags = {};
    u32 vb_offset           = 0;
    u32 ib_offset           = 0;
    u32 vertex_count        = 0;
    u32 index_count         = 0;
    MaterialID material_id;

    math::AABB aabb;

    [[nodiscard]]
    auto get_gpu_index_type() const -> gpu::IndexType
    {
      return flags.test(MeshInstanceFlag::USE_16_BIT_INDICES) ? gpu::IndexType::UINT16
                                                              : gpu::IndexType::UINT32;
    }
  };

  struct MeshGroup
  {
    soul::String name;
    Vector<Mesh> meshes;
    math::AABB aabb;
  };

  struct RenderComponent
  {
    MeshGroupID mesh_group_id;
  };

  struct LightComponent
  {
    LightRadiationType type = LightRadiationType::COUNT;
    vec3f32 color           = {1.0f, 1.0f, 1.0f};
    f32 intensity           = 1000.0f;
    f32 inner_angle         = 0.0f;
    f32 outer_angle         = math::f32const::PI;

    static auto Point(vec3f32 color, f32 intensity) -> LightComponent
    {
      return {
        .type      = LightRadiationType::POINT,
        .color     = color,
        .intensity = intensity,
      };
    }

    static auto Directional(vec3f32 color, f32 intensity) -> LightComponent
    {
      return {
        .type      = LightRadiationType::DIRECTIONAL,
        .color     = color,
        .intensity = intensity,
      };
    }

    static auto Spot(vec3f32 color, f32 intensity, f32 inner_angle, f32 outer_angle)
      -> LightComponent
    {
      return {
        .type        = LightRadiationType::SPOT,
        .color       = color,
        .intensity   = intensity,
        .inner_angle = inner_angle,
        .outer_angle = outer_angle,
      };
    }
  };

  struct CameraTransform
  {
    vec3f32 position;
    vec3f32 target;
    vec3f32 up;

    static auto FromModelMat(mat4f32 model_mat) -> CameraTransform
    {
      const auto up       = model_mat.col(1).xyz();
      const auto position = model_mat.col(3).xyz();
      return CameraTransform{
        .position = position,
        .target   = position - model_mat.col(2).xyz(),
        .up       = up,
      };
    };
  };

  struct CameraComponent
  {
    f32 fovy;
    f32 near_z;
    f32 far_z;
    f32 aspect_ratio;
  };

  struct CameraEntityDesc
  {
    StringView name = ""_str;
    CameraTransform camera_transform;
    EntityId parent_entity_id = EntityId::Null();
    CameraComponent camera_component;
  };

  struct EnvMapSetting
  {
    mat4f32 transform;
    vec3f32 tint;
    f32 intensity;
  };

  struct EnvMap
  {
    gpu::TextureID texture_id;
    EnvMapSettingData setting_data;
  };

  struct RenderSetting
  {
    b8 enable_jitter;
  };
} // namespace renderlab
