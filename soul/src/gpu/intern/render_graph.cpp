#include "gpu/render_graph.h"
#include "core/panic_format.h"
#include "gpu/render_graph_registry.h"
#include "gpu/system.h"
#include "gpu/type.h"

namespace soul::gpu
{

  using namespace impl;

  auto RenderGraph::clear_texture(
    QueueType queue_type, TextureNodeID texture, ClearValue clear_value) -> TextureNodeID
  {
    struct ClearParameter
    {
      TextureNodeID clear_texture;
    };

    const auto& pass = add_non_shader_pass<ClearParameter>(
      "Clear Texture Pass",
      queue_type,
      [texture](ClearParameter& parameter, auto& builder)
      {
        parameter.clear_texture = builder.add_dst_texture(texture);
      },
      [clear_value](const ClearParameter& parameter, auto& registry, auto& command_list)
      {
        command_list.template push<RenderCommandClearTexture>({
          .dst_texture = registry.get_texture(parameter.clear_texture),
          .clear_value = clear_value,
        });
      });
    return pass.get_parameter().clear_texture;
  }

  auto RenderGraph::import_texture(const char* name, TextureID texture_id) -> TextureNodeID
  {
    const auto resource_index = soul::cast<u32>(
      external_textures_.add(RGExternalTexture({.name = name, .texture_id = texture_id})));
    return create_resource_node<RGResourceType::TEXTURE>(RGResourceID::external_id(resource_index));
  }

  auto RenderGraph::create_texture(const char* name, const RGTextureDesc& desc) -> TextureNodeID
  {
    const auto resource_index = soul::cast<u32>(internal_textures_.add(RGInternalTexture{
      .name         = name,
      .type         = desc.type,
      .format       = desc.format,
      .extent       = desc.extent,
      .mip_levels   = desc.mip_levels,
      .sample_count = desc.sample_count,
      .clear        = desc.clear,
      .clear_value  = desc.clear_value,
    }));
    return create_resource_node<RGResourceType::TEXTURE>(RGResourceID::internal_id(resource_index));
  }

  auto RenderGraph::import_buffer(const char* name, const BufferID buffer_id) -> BufferNodeID
  {
    const auto resource_index = soul::cast<u32>(external_buffers_.add(RGExternalBuffer{
      .name      = name,
      .buffer_id = buffer_id,
    }));
    return create_resource_node<RGResourceType::BUFFER>(RGResourceID::external_id(resource_index));
  }

  auto RenderGraph::create_buffer(const char* name, const RGBufferDesc& desc) -> BufferNodeID
  {
    SOUL_ASSERT_FORMAT(
      0, desc.size > 0, "Render Graph buffer size must be greater than zero!, name = {}", name);

    const auto resource_index =
      soul::cast<u32>(internal_buffers_.add(RGInternalBuffer{.name = name, .size = desc.size}));
    return create_resource_node<RGResourceType::BUFFER>(RGResourceID::internal_id(resource_index));
  }

  auto RenderGraph::import_tlas(const char* name, TlasID tlas_id) -> TlasNodeID
  {
    const auto resource_index =
      soul::cast<u32>(external_tlas_list_.add(RGExternalTlas{.name = name, .tlas_id = tlas_id}));

    return create_resource_node<RGResourceType::TLAS>(RGResourceID::external_id(resource_index));
  }

  auto RenderGraph::import_blas_group(const char* name, BlasGroupID blas_group_id)
    -> BlasGroupNodeID
  {
    const auto resource_index = soul::cast<u32>(external_blas_group_list_.add(
      RGExternalBlasGroup{.name = name, .blas_group_id = blas_group_id}));

    return create_resource_node<RGResourceType::BLAS_GROUP>(
      RGResourceID::external_id(resource_index));
  }

  auto RenderGraph::get_texture_desc(TextureNodeID node_id, const gpu::System& system) const
    -> RGTextureDesc
  {
    const auto& node = get_resource_node(node_id.id);
    if (node.resource_id.is_external())
    {
      const RGExternalTexture& external_texture = external_textures_[node.resource_id.get_index()];
      const gpu::TextureDesc& desc = system.get_texture(external_texture.texture_id).desc;
      return {
        .type         = desc.type,
        .format       = desc.format,
        .extent       = desc.extent,
        .mip_levels   = desc.mip_levels,
        .layer_count  = desc.layer_count,
        .sample_count = desc.sample_count,
        .clear        = external_texture.clear,
        .clear_value  = external_texture.clear_value,
      };
    }
    const RGInternalTexture& internal_texture = internal_textures_[node.resource_id.get_index()];
    return {
      .type         = internal_texture.type,
      .format       = internal_texture.format,
      .extent       = internal_texture.extent,
      .mip_levels   = internal_texture.mip_levels,
      .layer_count  = internal_texture.layer_count,
      .sample_count = internal_texture.sample_count,
      .clear        = internal_texture.clear,
      .clear_value  = internal_texture.clear_value,
    };
  }

  auto RenderGraph::get_buffer_desc(BufferNodeID node_id, const gpu::System& system) const
    -> RGBufferDesc
  {
    const auto& node = get_resource_node(node_id.id);
    if (node.resource_id.is_external())
    {
      const RGExternalBuffer& external_buffer = external_buffers_[node.resource_id.get_index()];
      const gpu::BufferDesc& desc             = system.get_buffer(external_buffer.buffer_id).desc;
      return {.size = desc.size};
    }
    const RGInternalBuffer& internal_buffer = internal_buffers_[node.resource_id.get_index()];
    return {.size = internal_buffer.size};
  }

  auto RenderGraph::create_resource_node(
    RGResourceType resource_type, impl::RGResourceID resource_id) -> ResourceNodeID
  {
    return ResourceNodeID(resource_nodes_.add(ResourceNode(resource_type, resource_id)));
  }

  RenderGraph::~RenderGraph()
  {
    for (auto pass_node : pass_nodes_)
    {
      allocator_->destroy(pass_node);
    }
  }

  void RenderGraph::read_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id)
  {
    get_resource_node(resource_node_id).readers.push_back(pass_node_id);
  }

  auto RenderGraph::write_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id)
    -> ResourceNodeID
  {
    auto resource_node_ref = [this, resource_node_id]() -> ResourceNode&
    {
      return get_resource_node(resource_node_id);
    };

    if (resource_node_ref().writer.is_null())
    {
      const auto dst_resource_node_id       = ResourceNodeID(resource_nodes_.add(ResourceNode(
        resource_node_ref().resource_type, resource_node_ref().resource_id, pass_node_id)));
      resource_node_ref().writer            = pass_node_id;
      resource_node_ref().write_target_node = dst_resource_node_id;
    }

    return resource_node_ref().write_target_node;
  }

  auto RenderGraph::get_resource_node(ResourceNodeID node_id) const -> const impl::ResourceNode&
  {
    return resource_nodes_[node_id.id];
  }

  auto RenderGraph::get_resource_node(ResourceNodeID node_id) -> impl::ResourceNode&
  {
    return resource_nodes_[node_id.id];
  }

  auto RenderGraph::get_resource_nodes() const -> Span<const impl::ResourceNode*>
  {
    return resource_nodes_.cspan();
  }

} // namespace soul::gpu
