#include "mesh_preprocessor.h"
#include <limits>
#include "type.h"

#include "core/log.h"
#include "core/vector.h"
#include "math/math.h"
#include "runtime/scope_allocator.h"

#include "mikktspace/mikktspace.h"
#include "type.shared.hlsl"

namespace renderlab
{
  class MikkTComputation
  {
    NotNull<const MeshDesc*> mesh_desc_;
    vec4f32* tangents_;

    [[nodiscard]]
    auto get_index(i32 face, i32 vert) const -> u32
    {
      return mesh_desc_->get_vertex_index(face, vert);
    }

    void get_position(float position[], i32 face, i32 vert) const
    {
      vec3f32 src_position = mesh_desc_->get_position(face, vert);
      std::memcpy(position, &src_position, sizeof(vec3f32));
    }

    void get_normal(float normal[], i32 face, i32 vert) const
    {
      vec3f32 src_normal = mesh_desc_->get_normal(face, vert);
      std::memcpy(normal, &src_normal, sizeof(vec3f32));
    }

    void get_tex_coord(float tex_coord[], i32 face, i32 vert) const
    {
      vec2f32 src_tex_coord = mesh_desc_->get_tex_coord(face, vert);
      std::memcpy(tex_coord, &src_tex_coord, sizeof(vec2f32));
    }

    [[nodiscard]]
    auto get_face_count() const -> i32
    {
      return cast<i32>(mesh_desc_->get_face_count());
    }

    void set_tangent(const float tangent[], f32 sign, i32 face, i32 vert)
    {
      vec3f32 val                = *cast<const vec3f32*>(tangent);
      tangents_[face * 3 + vert] = vec4f32(math::normalize(val), sign);
    }

    MikkTComputation(const MeshDesc& mesh_desc, NotNull<memory::Allocator*> allocator)
        : mesh_desc_(&mesh_desc),
          tangents_(allocator->allocate_array<vec4f32>(cast<usize>(mesh_desc.get_index_count())))
    {
    }

  public:
    static auto GenerateTangents(const MeshDesc& mesh_desc, NotNull<memory::Allocator*> allocator)
      -> vec4f32*
    {
      MikkTComputation computation(mesh_desc, allocator);
      SMikkTSpaceInterface mikktspace = {};
      mikktspace.m_getNumFaces        = [](const SMikkTSpaceContext* context)
      {
        return cast<MikkTComputation*>(context->m_pUserData)->get_face_count();
      };

      mikktspace.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, i32 face)
      {
        return 3;
      };

      mikktspace.m_getPosition =
        [](const SMikkTSpaceContext* context, f32 position[], i32 face, i32 vert)
      {
        cast<MikkTComputation*>(context->m_pUserData)->get_position(position, face, vert);
      };

      mikktspace.m_getNormal =
        [](const SMikkTSpaceContext* context, f32 normal[], i32 face, i32 vert)
      {
        cast<MikkTComputation*>(context->m_pUserData)->get_normal(normal, face, vert);
      };

      mikktspace.m_getTexCoord =
        [](const SMikkTSpaceContext* context, float tex_coord[], i32 face, i32 vert)
      {
        cast<MikkTComputation*>(context->m_pUserData)->get_tex_coord(tex_coord, face, vert);
      };

      mikktspace.m_setTSpaceBasic = [](
                                      const SMikkTSpaceContext* context,
                                      const float tangent[],
                                      float sign,
                                      int32_t face,
                                      int32_t vert)
      {
        ((MikkTComputation*)(context->m_pUserData))->set_tangent(tangent, sign, face, vert);
      };

      SMikkTSpaceContext context = {};
      context.m_pInterface       = &mikktspace;
      context.m_pUserData        = &computation;

      if (genTangSpaceDefault(&context) == 0)
      {
        SOUL_PANIC("Fail to generate tangent");
      }
      return computation.tangents_;
    }
  };

  auto MeshPreprocessor::GenerateVertexes(const MeshDesc& mesh_desc) -> Result
  {
    runtime::ScopeAllocator scope_allocator("generate_static_vertex_data");

    const u32 max_index = mesh_desc.indexes.visit(
      [](const auto& data) -> u32
      {
        return *std::ranges::max_element(data);
      });

    const auto tangent_attributes = [&mesh_desc, &scope_allocator]() -> MeshDesc::Attribute<vec4f32>
    {
      if (mesh_desc.tangents.data == nullptr)
      {
        return {
          .data      = MikkTComputation::GenerateTangents(mesh_desc, &scope_allocator),
          .frequency = MeshDesc::AttributeFrequency::FACE_VARYING};
      } else
      {
        return mesh_desc.tangents;
      }
    }();
    auto mesh_desc_local     = mesh_desc;
    mesh_desc_local.tangents = tangent_attributes;

    auto is_vertex_equal =
      [](const StaticVertexData& lhs, const StaticVertexData& rhs, float threshold = 1e-6f)
    {
      if (any(lhs.position != rhs.position))
      {
        return false; // Position need to be exact to avoid cracks
      }
      if (lhs.tangent.w != rhs.tangent.w)
      {
        return false;
      }
      const auto threshold_vec3  = vec3f32(threshold);
      const auto normal_diff_vec = math::abs(lhs.normal - rhs.normal);
      if (any(normal_diff_vec > threshold_vec3))
      {
        return false;
      }
      const auto tangent_diff_vec = math::abs(lhs.tangent.xyz() - rhs.tangent.xyz());
      if (any(tangent_diff_vec > threshold_vec3))
      {
        return false;
      }
      const auto threshold_vec2     = vec2f32(threshold);
      const auto tex_coord_diff_vec = math::abs(lhs.tex_coord - rhs.tex_coord);
      if (any(tex_coord_diff_vec > threshold_vec2))
      {
        return false;
      }
      return true;
    };

    // We build a linked list for each index and use it to find any duplicate.
    u32 sentinel_node_index = std::numeric_limits<u32>::max();

    struct VertexNode
    {
      StaticVertexData vertex;
      u32 next = sentinel_node_index;
    };

    auto nodes      = Vector<VertexNode>::WithCapacity(mesh_desc_local.vertex_count);
    auto list_heads = Vector<u32>::FillN(mesh_desc_local.vertex_count, sentinel_node_index);
    auto indices    = Vector<u32>::WithSize(mesh_desc_local.get_index_count() * 3);

    for (u32 face = 0; face < mesh_desc_local.get_face_count(); face++)
    {
      for (u32 vert = 0; vert < 3; vert++)
      {
        const auto v             = mesh_desc_local.get_static_vertex_data(face, vert);
        const u32 original_index = mesh_desc_local.get_vertex_index(face, vert);

        uint32_t index = list_heads[original_index];
        bool found     = false;

        while (index != sentinel_node_index)
        {
          if (is_vertex_equal(v, nodes[index].vertex))
          {
            found = true;
            break;
          }
          index = nodes[index].next;
        }

        // Insert new vertex if we couldn't find it.
        if (!found)
        {
          SOUL_ASSERT(0, nodes.size() < std::numeric_limits<u32>::max());
          index = cast<u32>(nodes.size());
          nodes.push_back(VertexNode{v, list_heads[original_index]});

          list_heads[original_index] = index;
        }

        // Store new vertex index.
        indices[face * 3 + vert] = index;
      }
    }

    auto index_data = [&indices]() -> IndexData
    {
      if (false)
      {
        return IndexData::From(Vector<u16>::Transform(
          indices,
          [](u32 index) -> u16
          {
            return cast<u16>(index);
          }));
      } else
      {
        return IndexData::From(std::move(indices));
      }
    }();

    auto node_to_vertex = [](const VertexNode& node)
    {
      return node.vertex;
    };
    return {
      .vertexes = Vector<StaticVertexData>::Transform(nodes, node_to_vertex),
      .indexes  = std::move(index_data),
    };
  }
} // namespace renderlab
