#include "core/dev_util.h"

#include "gpu/render_graph.h"

#include "gpu/system.h"

namespace soul::gpu
{

	using namespace impl;

	TextureNodeID RenderGraph::import_texture(const char* name, TextureID texture_id) {
		const auto node_id = TextureNodeID(texture_nodes_.add(TextureNode()));
		TextureNode& node = texture_nodes_.back();
		const auto resource_index = soul::cast<uint32>(external_textures_.add(RGExternalTexture(
			{
				.name = name,
				.texture_id = texture_id
			}
		)));
		node.resource_id = RGResourceID::external_id(resource_index);
		return node_id;
	}

	TextureNodeID RenderGraph::create_texture(const char* name, const RGTextureDesc& desc) {
		TextureNodeID node_id = TextureNodeID(texture_nodes_.add(TextureNode()));
		TextureNode& node = texture_nodes_.back();

		uint32 resource_index = soul::cast<uint32>(internal_textures_.add(RGInternalTexture()));
		RGInternalTexture& internal_texture = internal_textures_.back();
		internal_texture.name = name;
		internal_texture.type = desc.type;
		internal_texture.extent = desc.extent;
		internal_texture.mip_levels = desc.mip_levels;
		internal_texture.format = desc.format;
		internal_texture.clear = desc.clear;
		internal_texture.clear_value = desc.clear_value;
		internal_texture.sample_count = desc.sample_count;

		node.resource_id = RGResourceID::internal_id(resource_index);

		return node_id;
	}

	BufferNodeID RenderGraph::import_buffer(const char* name, BufferID buffer_id) {
		BufferNodeID node_id = BufferNodeID(buffer_nodes_.add(BufferNode()));
		BufferNode& node = buffer_nodes_.back();

		const soul_size resource_index = external_buffers_.add(RGExternalBuffer());
		RGExternalBuffer& external_buffer = external_buffers_.back();
		external_buffer.name = name;
		external_buffer.buffer_id = buffer_id;

		node.resource_id = RGResourceID::external_id(resource_index);

		return node_id;
	}

	BufferNodeID RenderGraph::create_buffer(const char* name, const RGBufferDesc& desc) {
		BufferNodeID node_id = BufferNodeID(buffer_nodes_.add(BufferNode()));
		BufferNode& node = buffer_nodes_.back();

		uint32 resource_index = soul::cast<uint32>(internal_buffers_.add(RGInternalBuffer()));
		RGInternalBuffer& internalBuffer = internal_buffers_.back();
		internalBuffer.name = name;
		internalBuffer.count = desc.count;
		internalBuffer.type_size = desc.typeSize;
		internalBuffer.type_alignment = desc.typeAlignment;

		node.resource_id = RGResourceID::internal_id(resource_index);

		return node_id;
	}

	RGTextureDesc RenderGraph::get_texture_desc(TextureNodeID node_id, const gpu::System& system) const
	{
		const impl::TextureNode& node = texture_nodes_[node_id.id];
		if (node.resource_id.is_external())
		{
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
				.clear_value = external_texture.clear_value
			};
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
			.clear_value = internal_texture.clear_value
		};
	}

	RGBufferDesc RenderGraph::get_buffer_desc(BufferNodeID node_id, const gpu::System& system) const
	{
		const impl::BufferNode& node = buffer_nodes_[node_id.id];
		if (node.resource_id.is_external())
		{
			const RGExternalBuffer& external_buffer = external_buffers_[node.resource_id.get_index()];
			const gpu::BufferDesc& desc = system.get_buffer(external_buffer.buffer_id).desc;
			return {
				.count = desc.count,
				.typeSize = desc.type_size,
				.typeAlignment = desc.type_alignment
			};
		}
		const RGInternalBuffer& internal_buffer = internal_buffers_[node.resource_id.get_index()];
		return {
			.count = internal_buffer.count,
			.typeSize = internal_buffer.type_size,
			.typeAlignment = internal_buffer.type_alignment
		};
	}

	void RenderGraph::cleanup() {
		SOUL_PROFILE_ZONE();
		for (PassNode* passNode : pass_nodes_) {
			allocator_->destroy(passNode);
		}
		pass_nodes_.cleanup();

		buffer_nodes_.cleanup();

		texture_nodes_.cleanup();

		internal_buffers_.cleanup();
		internal_textures_.cleanup();
		external_buffers_.cleanup();
		external_textures_.cleanup();
	}

	RenderGraph::~RenderGraph()
	{
		cleanup();
	}

	void RenderGraph::read_buffer_node(BufferNodeID buffer_node_id, PassNodeID pass_node_id) {
		get_buffer_node(buffer_node_id).readers.push_back(pass_node_id);
	}

	BufferNodeID RenderGraph::write_buffer_node(BufferNodeID buffer_node_id, PassNodeID pass_node_id) {
		BufferNode& src_buffer_node = get_buffer_node(buffer_node_id);

		if (src_buffer_node.writer.is_null())
		{
			src_buffer_node.writer = pass_node_id;
			const BufferNodeID dst_buffer_node_id = BufferNodeID(soul::cast<uint16>(buffer_nodes_.add(BufferNode())));
			BufferNode& dst_buffer_node = get_buffer_node(dst_buffer_node_id);
			dst_buffer_node.resource_id = get_buffer_node(buffer_node_id).resource_id;
			dst_buffer_node.creator = pass_node_id;
			src_buffer_node.write_target_node = dst_buffer_node_id;
		}
		SOUL_ASSERT(0, src_buffer_node.writer == pass_node_id, "");

		return src_buffer_node.write_target_node;
	}

	void RenderGraph::read_texture_node(TextureNodeID node_id, PassNodeID pass_node_id) {
		get_texture_node(node_id).readers.push_back(pass_node_id);
	}

	TextureNodeID RenderGraph::write_texture_node(TextureNodeID texture_node_id, PassNodeID pass_node_id) {
		TextureNode& src_texture_node = get_texture_node(texture_node_id);
		if (src_texture_node.writer.is_null())
		{
			src_texture_node.writer = pass_node_id;
			const TextureNodeID dst_texture_node_id = TextureNodeID(texture_nodes_.add(TextureNode()));
			TextureNode& dst_texture_node = get_texture_node(dst_texture_node_id);
			dst_texture_node.resource_id = get_texture_node(texture_node_id).resource_id;
			dst_texture_node.creator = pass_node_id;
			src_texture_node.write_target_node = dst_texture_node_id;
		}
		return src_texture_node.write_target_node;
	}

	const BufferNode& RenderGraph::get_buffer_node(const BufferNodeID node_id) const
	{
		return buffer_nodes_[node_id.id];
	}

	BufferNode& RenderGraph::get_buffer_node(const BufferNodeID node_id)
	{
		return buffer_nodes_[node_id.id];
	}

	const TextureNode& RenderGraph::get_texture_node(const TextureNodeID node_id) const
	{
		return texture_nodes_[node_id.id];
	}

	TextureNode& RenderGraph::get_texture_node(const TextureNodeID node_id)
	{
		return texture_nodes_[node_id.id];
	}

	BufferNodeID RGShaderPassDependencyBuilder::add_vertex_buffer(const BufferNodeID node_id) {
		render_graph_.read_buffer_node(node_id, pass_id_);
		shader_node_.vertex_buffers_.push_back(node_id);
		return node_id;
	}

	BufferNodeID RGShaderPassDependencyBuilder::add_index_buffer(const BufferNodeID node_id) {
		render_graph_.read_buffer_node(node_id, pass_id_);
		shader_node_.index_buffers_.push_back(node_id);
		return node_id;
	}

	BufferNodeID RGShaderPassDependencyBuilder::add_shader_buffer(const BufferNodeID node_id, const ShaderStageFlags stage_flags, const ShaderBufferReadUsage usage) {
		render_graph_.read_buffer_node(node_id, pass_id_);
		shader_node_.shader_buffer_read_accesses_.push_back({ node_id, stage_flags, usage });
		return node_id;
	}

	BufferNodeID RGShaderPassDependencyBuilder::add_shader_buffer(const BufferNodeID node_id, const ShaderStageFlags stage_flags, const ShaderBufferWriteUsage usage) {
		BufferNodeID out_node_id = render_graph_.write_buffer_node(node_id, pass_id_);
		shader_node_.shader_buffer_write_accesses_.push_back({ node_id, out_node_id, stage_flags, usage });
		return out_node_id;
	}

	TextureNodeID RGShaderPassDependencyBuilder::add_shader_texture(const TextureNodeID node_id, const ShaderStageFlags stage_flags, const ShaderTextureReadUsage usage, const SubresourceIndexRange view_range) {
		SOUL_ASSERT(0, node_id.is_valid(), "");
		render_graph_.read_texture_node(node_id, pass_id_);
		shader_node_.shader_texture_read_accesses_.push_back({ node_id, stage_flags, usage , view_range});
		return node_id;
	}

	TextureNodeID RGShaderPassDependencyBuilder::add_shader_texture(const TextureNodeID node_id, const ShaderStageFlags stage_flags, const ShaderTextureWriteUsage usage, const SubresourceIndexRange view_range) {
		TextureNodeID out_node_id = render_graph_.write_texture_node(node_id, pass_id_);
		shader_node_.shader_texture_write_accesses_.add({ node_id, out_node_id, stage_flags, usage , view_range});
		return out_node_id;
	}

	BufferNodeID RGCopyPassDependencyBuilder::add_src_buffer(const BufferNodeID node_id)
    {
		render_graph_.read_buffer_node(node_id, pass_id_);
		copy_base_node_.source_buffers_.add({ node_id });
		return node_id;
	}

	BufferNodeID RGCopyPassDependencyBuilder::add_dst_buffer(const BufferNodeID node_id)
    {
		BufferNodeID out_node_id = render_graph_.write_buffer_node(node_id, pass_id_);
		copy_base_node_.destination_buffers_.add({.input_node_id = node_id, .output_node_id = out_node_id});
		return out_node_id;
	}

	TextureNodeID RGCopyPassDependencyBuilder::add_src_texture(const TextureNodeID node_id)
    {
		render_graph_.read_texture_node(node_id, pass_id_);
		copy_base_node_.source_textures_.add({node_id});
		return node_id;
	}

	TextureNodeID RGCopyPassDependencyBuilder::add_dst_texture(const TextureNodeID node_id)
    {
		TextureNodeID out_node_id = render_graph_.write_texture_node(node_id, pass_id_);
		copy_base_node_.destination_textures_.add({.input_node_id = node_id, .output_node_id = out_node_id});
		return out_node_id;
	}

}
