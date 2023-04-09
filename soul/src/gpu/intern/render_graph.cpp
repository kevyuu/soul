#include "gpu/render_graph.h"
#include "core/dev_util.h"
#include "gpu/system.h"

namespace soul::gpu
{

  using namespace impl;

  TextureNodeID RenderGraph::import_texture(const char* name, TextureID texture_id)
  {
    const auto resource_index = soul::cast<uint32>(
      external_textures_.add(RGExternalTexture({.name = name, .texture_id = texture_id})));
    return create_resource_node<RGResourceType::TEXTURE>(RGResourceID::external_id(resource_index));
  }

  TextureNodeID RenderGraph::create_texture(const char* name, const RGTextureDesc& desc)
  {
    const auto resource_index = soul::cast<uint32>(internal_textures_.add(
      {.name = name,
       .type = desc.type,
       .format = desc.format,
       .extent = desc.extent,
       .mip_levels = desc.mip_levels,
       .sample_count = desc.sample_count,
       .clear = desc.clear,
       .clear_value = desc.clear_value}));
    return create_resource_node<RGResourceType::TEXTURE>(RGResourceID::internal_id(resource_index));
  }

  BufferNodeID RenderGraph::import_buffer(const char* name, const BufferID buffer_id)
  {
    const auto resource_index =
      soul::cast<uint32>(external_buffers_.add({.name = name, .buffer_id = buffer_id}));
    return create_resource_node<RGResourceType::BUFFER>(RGResourceID::external_id(resource_index));
  }

  BufferNodeID RenderGraph::create_buffer(const char* name, const RGBufferDesc& desc)
  {
    SOUL_ASSERT(
      0, desc.size > 0, "Render Graph buffer size must be greater than zero!, name = %s", name);

    const auto resource_index =
      soul::cast<uint32>(internal_buffers_.add({.name = name, .size = desc.size}));
    return create_resource_node<RGResourceType::BUFFER>(RGResourceID::internal_id(resource_index));
  }

  TlasNodeID RenderGraph::import_tlas(const char* name, TlasID tlas_id)
  {
    const auto resource_index =
      soul::cast<uint32>(external_tlas_list_.add({.name = name, .tlas_id = tlas_id}));

    return create_resource_node<RGResourceType::TLAS>(RGResourceID::external_id(resource_index));
  }

  BlasGroupNodeID RenderGraph::import_blas_group(const char* name, BlasGroupID blas_group_id)
  {
    const auto resource_index = soul::cast<uint32>(
      external_blas_group_list_.add({.name = name, .blas_group_id = blas_group_id}));

    return create_resource_node<RGResourceType::BLAS_GROUP>(
      RGResourceID::external_id(resource_index));
  }

  RGTextureDesc RenderGraph::get_texture_desc(
    TextureNodeID node_id, const gpu::System& system) const
  {
    const auto& node = get_resource_node(node_id.id);
    if (node.resource_id.is_external()) {
      const RGExternalTexture& external_texture = external_textures_[node.resource_id.get_index()];
      const gpu::TextureDesc& desc = system.get_texture(external_texture.texture_id).desc;
      return {
        .type = desc.type,
        .format = desc.format,
        .extent = desc.extent,
        .mip_levels = desc.mip_levels,
        .layer_count = desc.layer_count,
        .sample_count = desc.sample_count,
        .clear = external_texture.clear,
        .clear_value = external_texture.clear_value};
    }
    const RGInternalTexture& internal_texture = internal_textures_[node.resource_id.get_index()];
    return {
      .type = internal_texture.type,
      .format = internal_texture.format,
      .extent = internal_texture.extent,
      .mip_levels = internal_texture.mip_levels,
      .layer_count = internal_texture.layer_count,
      .sample_count = internal_texture.sample_count,
      .clear = internal_texture.clear,
      .clear_value = internal_texture.clear_value};
  }

  RGBufferDesc RenderGraph::get_buffer_desc(BufferNodeID node_id, const gpu::System& system) const
  {
    const auto& node = get_resource_node(node_id.id);
    if (node.resource_id.is_external()) {
      const RGExternalBuffer& external_buffer = external_buffers_[node.resource_id.get_index()];
      const gpu::BufferDesc& desc = system.get_buffer(external_buffer.buffer_id).desc;
      return {.size = desc.size};
    }
    const RGInternalBuffer& internal_buffer = internal_buffers_[node.resource_id.get_index()];
    return {.size = internal_buffer.size};
  }

  ResourceNodeID RenderGraph::create_resource_node(
    RGResourceType resource_type, impl::RGResourceID resource_id)
  {
    return ResourceNodeID(resource_nodes_.add(ResourceNode(resource_type, resource_id)));
  }

  RenderGraph::~RenderGraph()
  {
    for (auto* pass_node : pass_nodes_) {
      allocator_->destroy(pass_node);
    }
  }

  void RenderGraph::read_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id)
  {
    get_resource_node(resource_node_id).readers.push_back(pass_node_id);
  }

  ResourceNodeID RenderGraph::write_resource_node(
    ResourceNodeID resource_node_id, PassNodeID pass_node_id)
  {
    auto& src_resource_node = get_resource_node(resource_node_id);
    if (src_resource_node.writer.is_null()) {
      src_resource_node.writer = pass_node_id;
      const auto dst_resource_node_id = ResourceNodeID(resource_nodes_.add(ResourceNode(
        src_resource_node.resource_type, src_resource_node.resource_id, pass_node_id)));
      src_resource_node.write_target_node = dst_resource_node_id;
    }

    return src_resource_node.write_target_node;
  }

  const impl::ResourceNode& RenderGraph::get_resource_node(ResourceNodeID node_id) const
  {
    return resource_nodes_[node_id.id];
  }

  impl::ResourceNode& RenderGraph::get_resource_node(ResourceNodeID node_id)
  {
    return resource_nodes_[node_id.id];
  }

  std::span<const impl::ResourceNode> RenderGraph::get_resource_nodes() const
  {
    return resource_nodes_;
  }

} // namespace soul::gpu
