#include "core/util.h"

#include "runtime/runtime.h"
#include "runtime/scope_allocator.h"

#include "gpu/system.h"
#include "gpu/render_graph_registry.h"
#include "gpu/intern/render_graph_execution.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_compiler.h"

#include <volk.h>
#include <ranges>

namespace soul::gpu::impl
{

	auto PASS_TYPE_TO_QUEUE_TYPE_MAP = FlagMap<PassType, QueueType>::build_from_list({
		QueueType::COUNT,
		QueueType::GRAPHIC,
		QueueType::COMPUTE,
		QueueType::TRANSFER,
	});

	auto RESOURCE_OWNER_TO_PASS_TYPE_MAP = FlagMap<ResourceOwner, PassType>::build_from_list({
		PassType::NONE,
		PassType::GRAPHIC,
		PassType::COMPUTE,
		PassType::TRANSFER,
		PassType::NONE
	});

	auto SHADER_BUFFER_READ_USAGE_MAP = FlagMap<ShaderBufferReadUsage, BufferUsageFlags>::build_from_list({
		BufferUsageFlags({ BufferUsage::UNIFORM }),
		{ BufferUsage::STORAGE}
	});
	constexpr BufferUsageFlags get_buffer_usage_flags(ShaderBufferReadUsage usage)
	{
		return SHADER_BUFFER_READ_USAGE_MAP[usage];
	}

	using ShaderBufferWriteMap = FlagMap<ShaderBufferWriteUsage, BufferUsageFlags>;
	auto SHADER_BUFFER_WRITE_USAGE_MAP = ShaderBufferWriteMap::build_from_list({
        { BufferUsage::UNIFORM },
        { BufferUsage::STORAGE }
	});
	constexpr BufferUsageFlags get_buffer_usage_flags(ShaderBufferWriteUsage usage)
	{
		return SHADER_BUFFER_WRITE_USAGE_MAP[usage];
	}

	auto SHADER_TEXTURE_READ_USAGE_MAP = FlagMap<ShaderTextureReadUsage, TextureUsageFlags>::build_from_list({
		{ TextureUsage::SAMPLED },
		{ TextureUsage::STORAGE }
	});
	constexpr TextureUsageFlags get_texture_usage_flags(ShaderTextureReadUsage usage)
	{
		return SHADER_TEXTURE_READ_USAGE_MAP[usage];
	}

	using ShaderTextureWriteMap = FlagMap<ShaderTextureWriteUsage, TextureUsageFlags>;
	auto SHADER_TEXTURE_WRITE_USAGE_MAP = ShaderTextureWriteMap::build_from_list(
	{
		{ TextureUsage::STORAGE }
	});
	constexpr TextureUsageFlags get_texture_usage_flags(ShaderTextureWriteUsage usage)
	{
		return SHADER_TEXTURE_WRITE_USAGE_MAP[usage];
	}

	void update_buffer_info(const QueueType queue_type, const BufferUsageFlags usage_flags,
	                        const PassNodeID pass_id, BufferExecInfo* buffer_info) {

		buffer_info->usage_flags |= usage_flags;
		buffer_info->queue_flags |= { queue_type };
		if (buffer_info->first_pass.is_null()) {
			buffer_info->first_pass = pass_id;
		}
		buffer_info->last_pass = pass_id;
		buffer_info->passes.push_back(pass_id);
	};
	

	void update_texture_info(const QueueType queue_type, const TextureUsageFlags usage_flags,
	                         const PassNodeID pass_id, const SubresourceIndexRange view_index_range, TextureExecInfo* texture_info)
	{

		texture_info->usage_flags |= usage_flags;
		texture_info->queue_flags |= { queue_type };
		if (texture_info->first_pass.is_null()) { 
			texture_info->first_pass = pass_id;
		}
		texture_info->last_pass = pass_id;

		std::ranges::for_each(view_index_range, [pass_id, texture_info](SubresourceIndex view_index)
		{
			texture_info->get_view(view_index)->passes.push_back(pass_id);
		});
		SOUL_ASSERT(0, !texture_info->view->passes.empty(), "");
	};

	void RenderGraphExecution::init() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE_WITH_NAME("Render Graph Execution Init");
		pass_infos_.resize(render_graph_->get_pass_nodes().size());

		buffer_infos_.resize(render_graph_->get_internal_buffers().size() + render_graph_->get_external_buffers().size());
		internal_buffer_infos_.set(&buffer_infos_, 0, render_graph_->get_internal_buffers().size());
		external_buffer_infos_.set(&buffer_infos_, render_graph_->get_internal_buffers().size(), buffer_infos_.size());

		const auto& internal_textures = render_graph_->get_internal_textures();
		const auto& external_textures = render_graph_->get_external_textures();
		texture_infos_.resize(internal_textures.size() + external_textures.size());
		internal_texture_infos_.set(&texture_infos_, 0, internal_textures.size());
		external_texture_infos_.set(&texture_infos_, internal_textures.size(), texture_infos_.size());

		constexpr auto fold_internal_texture_view_count = [](soul_size count, const RGInternalTexture& internal_texture)
		{
			return count + internal_texture.get_view_count();
		};
		auto fold_external_texture_view_count = [gpu_system = this->gpu_system_](const soul_size count, const RGExternalTexture& external_texture)
		{
			const gpu::TextureDesc& desc = gpu_system->get_texture(external_texture.texture_id).desc;
			return count + desc.get_view_count();
		};

		const soul_size internal_view_count = std::accumulate(internal_textures.begin(), internal_textures.end(), 0u, fold_internal_texture_view_count);
		const soul_size external_view_count = std::accumulate(external_textures.begin(), external_textures.end(), 0u, fold_external_texture_view_count);
		texture_view_infos_.resize(internal_view_count + external_view_count);

		for (soul_size texture_info_idx = 0, view_offset = 0; texture_info_idx < internal_texture_infos_.size(); texture_info_idx++)
		{
			TextureExecInfo& texture_info = internal_texture_infos_[texture_info_idx];
			texture_info.view = texture_view_infos_.data() + view_offset;
			texture_info.mip_levels = internal_textures[texture_info_idx].mip_levels;
			texture_info.layers = internal_textures[texture_info_idx].layer_count;
			view_offset += texture_info.get_view_count();
		}

		for (soul_size texture_info_idx = 0, view_offset = internal_view_count; texture_info_idx < external_texture_infos_.size(); texture_info_idx++)
		{
			TextureExecInfo& texture_info = external_texture_infos_[texture_info_idx];
			texture_info.view = texture_view_infos_.data() + view_offset;
			const TextureDesc& desc = gpu_system_->get_texture(external_textures[texture_info_idx].texture_id).desc;
			texture_info.mip_levels = desc.mip_levels;
			texture_info.layers = desc.layer_count;
			view_offset += texture_info.get_view_count();
		}

		for (soul_size i = 0; i < pass_infos_.size(); i++) {
			const auto pass_node_id = PassNodeID(soul::cast<uint16>(i));
			const PassNode& pass_node = *render_graph_->get_pass_nodes()[i];
			PassExecInfo& pass_info = pass_infos_[i];

			switch (pass_node.get_type()) {
			case PassType::NONE: {
				break;
			}
			case PassType::COMPUTE: {
					auto compute_node = soul::downcast<const ComputeBaseNode*>(&pass_node);
                    init_shader_buffers(compute_node->get_buffer_read_accesses(), i, QueueType::COMPUTE);
                    init_shader_buffers(compute_node->get_buffer_write_accesses(), i, QueueType::COMPUTE);
                    init_shader_textures(compute_node->get_texture_read_accesses(), i, QueueType::COMPUTE);
                    init_shader_textures(compute_node->get_texture_write_accesses(), i, QueueType::COMPUTE);
				break;
			}
			case PassType::GRAPHIC: {
				auto graphic_node = soul::downcast<const GraphicBaseNode*>(&pass_node);
				init_shader_buffers(graphic_node->get_buffer_read_accesses(), i, QueueType::GRAPHIC);
				init_shader_buffers(graphic_node->get_buffer_write_accesses(), i, QueueType::GRAPHIC);
				init_shader_textures(graphic_node->get_texture_read_accesses(), i, QueueType::GRAPHIC);
				init_shader_textures(graphic_node->get_texture_write_accesses(), i, QueueType::GRAPHIC);

				for (const BufferNodeID node_id : graphic_node->get_vertex_buffers()) {
					SOUL_ASSERT(0, node_id.is_valid(), "");

					uint32 buffer_info_id = get_buffer_info_index(node_id);

					pass_info.buffer_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
						.access_flags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						.buffer_info_idx = buffer_info_id
					});

					pass_info.buffer_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
						.access_flags = 0,
						.buffer_info_idx = buffer_info_id
					});

					update_buffer_info(QueueType::GRAPHIC, { BufferUsage::VERTEX }, pass_node_id, &buffer_infos_[buffer_info_id]);
				}

				for (const BufferNodeID node_id : graphic_node->get_index_buffers()) {
					SOUL_ASSERT(0, node_id.is_valid(), "");

					const auto buffer_info_id = get_buffer_info_index(node_id);

					pass_info.buffer_invalidates.add(BufferBarrier());
					BufferBarrier& invalidate_barrier = pass_info.buffer_invalidates.back();
					invalidate_barrier.stage_flags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					invalidate_barrier.access_flags = VK_ACCESS_INDEX_READ_BIT;
					invalidate_barrier.buffer_info_idx = buffer_info_id;

					pass_info.buffer_flushes.add(BufferBarrier());
					BufferBarrier& flush_barrier = pass_info.buffer_flushes.back();
					flush_barrier.stage_flags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					flush_barrier.access_flags = 0;
					flush_barrier.buffer_info_idx = buffer_info_id;

					update_buffer_info(QueueType::GRAPHIC, { BufferUsage::INDEX }, pass_node_id, &buffer_infos_[buffer_info_id]);
				}

				const auto& [dimension, sampleCount, colorAttachments, 
					resolveAttachments, depthStencilAttachment] = graphic_node->get_render_target();

				for (const ColorAttachment& color_attachment : colorAttachments) {
					SOUL_ASSERT(0, color_attachment.out_node_id.is_valid(), "");

					const auto texture_info_id = get_texture_info_index(color_attachment.out_node_id);
					update_texture_info(QueueType::GRAPHIC, { TextureUsage::COLOR_ATTACHMENT },
						pass_node_id, { color_attachment.desc.view, 1, 1 }, & texture_infos_[texture_info_id]);

					pass_info.texture_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.access_flags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.texture_info_idx = texture_info_id,
						.view =color_attachment.desc.view
					});

					pass_info.texture_flushes.push_back({
						.stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.access_flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.texture_info_idx = texture_info_id,
						.view = color_attachment.desc.view
					});

				}

				for (const ResolveAttachment& resolve_attachment : resolveAttachments)
				{
					const auto texture_info_id = get_texture_info_index(resolve_attachment.out_node_id);
					update_texture_info(QueueType::GRAPHIC, { TextureUsage::COLOR_ATTACHMENT },
						pass_node_id, { resolve_attachment.desc.view, 1, 1 }, & texture_infos_[texture_info_id]);

					pass_info.texture_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.access_flags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.texture_info_idx = texture_info_id,
						.view = resolve_attachment.desc.view
					});

					pass_info.texture_flushes.push_back({
						.stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.access_flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.texture_info_idx = texture_info_id,
						.view = resolve_attachment.desc.view
					});

				}

				if (depthStencilAttachment.out_node_id.is_valid()) {
					const auto resource_info_index = get_texture_info_index(depthStencilAttachment.out_node_id);

					const auto texture_info_id = get_texture_info_index(depthStencilAttachment.out_node_id);
					update_texture_info(QueueType::GRAPHIC, { TextureUsage::DEPTH_STENCIL_ATTACHMENT },
						pass_node_id, { depthStencilAttachment.desc.view, 1, 1 }, & texture_infos_[texture_info_id]);

					pass_info.texture_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.access_flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.texture_info_idx = resource_info_index,
						.view = depthStencilAttachment.desc.view
					});

					pass_info.texture_flushes.push_back({
						.stage_flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.access_flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.texture_info_idx = resource_info_index,
						.view = depthStencilAttachment.desc.view
					});
				}

				break;
			}
			case PassType::TRANSFER:
			{
				const auto copy_node = soul::downcast<const TransferBaseNode*>(&pass_node);
				for (const auto& source_buffer : copy_node->get_source_buffers())
				{
					const auto resource_info_index = get_buffer_info_index(source_buffer.node_id);
					update_buffer_info(QueueType::TRANSFER, { BufferUsage::TRANSFER_SRC }, pass_node_id, &buffer_infos_[resource_info_index]);

					pass_info.buffer_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.access_flags = VK_ACCESS_TRANSFER_READ_BIT,
						.buffer_info_idx = resource_info_index
					});

					pass_info.buffer_flushes.push_back({
						.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.access_flags = 0,
						.buffer_info_idx = resource_info_index
					});
				}

				for (const auto& dst_buffer : copy_node->get_destination_buffers())
				{
					const auto resource_info_index = get_buffer_info_index(dst_buffer.output_node_id);
					update_buffer_info(QueueType::TRANSFER, { BufferUsage::TRANSFER_DST }, pass_node_id, &buffer_infos_[resource_info_index]);

					pass_info.buffer_invalidates.push_back({
						.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.access_flags = VK_ACCESS_TRANSFER_WRITE_BIT,
						.buffer_info_idx = resource_info_index
					});

					pass_info.buffer_flushes.push_back({
						.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.access_flags = VK_ACCESS_TRANSFER_WRITE_BIT,
						.buffer_info_idx = resource_info_index
					});
				}

				for(const auto& src_texture : copy_node->get_source_textures())
				{
					const auto resource_info_index = get_texture_info_index(src_texture.node_id);
					update_texture_info(QueueType::TRANSFER, { TextureUsage::TRANSFER_SRC }, pass_node_id, src_texture.view_range, &texture_infos_[resource_info_index]);

					auto generate_invalidate_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.access_flags = VK_ACCESS_TRANSFER_READ_BIT,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							.texture_info_idx = resource_info_index,
							.view = view_index
						};
					};
					std::ranges::transform(src_texture.view_range, std::back_inserter(pass_info.texture_invalidates), generate_invalidate_barrier);

					auto generate_flush_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.access_flags = 0,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							.texture_info_idx = resource_info_index,
							.view = view_index
						};
					};
					std::ranges::transform(src_texture.view_range, std::back_inserter(pass_info.texture_flushes), generate_flush_barrier);

				}

				for (const auto& dst_texture : copy_node->get_destination_textures())
				{
					const auto resource_info_index = get_texture_info_index(dst_texture.output_node_id);
					update_texture_info(QueueType::TRANSFER, { TextureUsage::TRANSFER_DST }, pass_node_id, dst_texture.view_range, &texture_infos_[resource_info_index]);

					auto generate_invalidate_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.access_flags = VK_ACCESS_TRANSFER_WRITE_BIT,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							.texture_info_idx = resource_info_index
						};
					};
					std::ranges::transform(dst_texture.view_range, std::back_inserter(pass_info.texture_invalidates), generate_invalidate_barrier);

					auto generate_flush_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.access_flags = VK_ACCESS_TRANSFER_WRITE_BIT,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							.texture_info_idx = resource_info_index
						};
					};
					std::ranges::transform(dst_texture.view_range, std::back_inserter(pass_info.texture_flushes), generate_flush_barrier);
				}
				break;
			}
			case PassType::COUNT:
				SOUL_NOT_IMPLEMENTED();
				break;
			}
		}

		for (soul_size i = 0; i < render_graph_->get_internal_buffers().size(); i++) {
			const RGInternalBuffer& rg_buffer = render_graph_->get_internal_buffers()[i];
			BufferExecInfo& buffer_info = buffer_infos_[i];

			buffer_info.buffer_id = gpu_system_->create_transient_buffer({
				.size = rg_buffer.size,
				.usage_flags = buffer_info.usage_flags,
				.queue_flags = buffer_info.queue_flags
			});
		}

		for (soul_size i = 0; i < external_buffer_infos_.size(); i++) {
			BufferExecInfo& buffer_info = external_buffer_infos_[i];
			if (buffer_info.passes.empty()) continue;
			buffer_info.buffer_id = render_graph_->get_external_buffers()[i].buffer_id;
		}

		for (soul_size i = 0; i < render_graph_->get_internal_textures().size(); i++) {
			const RGInternalTexture& rg_texture = render_graph_->get_internal_textures()[i];
			TextureExecInfo& texture_info = texture_infos_[i];

			if (texture_info.queue_flags.none()) continue;

			TextureDesc desc = {
				.type = rg_texture.type,
				.format = rg_texture.format,
				.extent = rg_texture.extent,
				.mip_levels = rg_texture.mip_levels,
				.sample_count = rg_texture.sample_count,
				.usage_flags = texture_info.usage_flags,
				.queue_flags = texture_info.queue_flags,
				.name = rg_texture.name
			};
			if (!rg_texture.clear) {
				texture_info.texture_id = gpu_system_->create_texture(desc);
			} else {
				desc.usage_flags |= { TextureUsage::SAMPLED };
				texture_info.texture_id = gpu_system_->create_texture(desc, rg_texture.clear_value);
			}
		}

		for (soul_size i = 0; i < external_texture_infos_.size(); i++) {
			TextureExecInfo& texture_info = external_texture_infos_[i];
			texture_info.texture_id = render_graph_->get_external_textures()[i].texture_id;
		}
	}

	VkRenderPass RenderGraphExecution::create_render_pass(const uint32 pass_index) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		const auto graphic_node = soul::downcast<const GraphicBaseNode*>(render_graph_->get_pass_nodes()[pass_index]);
		RenderPassKey render_pass_key = {};
		const RGRenderTarget& render_target = graphic_node->get_render_target();

		auto get_render_pass_attachment = [this, pass_index](const auto& attachment) -> Attachment
		{
            const uint32 texture_info_idx = get_texture_info_index(attachment.out_node_id);
			const TextureExecInfo& texture_info = texture_infos_[texture_info_idx];
			const Texture& texture = *gpu_system_->get_texture_ptr(texture_info.texture_id);

			Attachment render_pass_attachment = {};
			render_pass_attachment.format = texture.desc.format;
			render_pass_attachment.sampleCount = texture.desc.sample_count;
			if (texture_info.first_pass.id == pass_index) render_pass_attachment.flags |= ATTACHMENT_FIRST_PASS_BIT;
			if (texture_info.last_pass.id == pass_index) render_pass_attachment.flags |= ATTACHMENT_LAST_PASS_BIT;
			if (attachment.desc.clear) render_pass_attachment.flags |= ATTACHMENT_CLEAR_BIT;
			if (is_external(texture_info)) render_pass_attachment.flags |= ATTACHMENT_EXTERNAL_BIT;
			render_pass_attachment.flags |= ATTACHMENT_ACTIVE_BIT;
			return render_pass_attachment;
		};

		std::ranges::transform(render_target.color_attachments, render_pass_key.color_attachments, get_render_pass_attachment);
		std::ranges::transform(render_target.resolve_attachments, render_pass_key.resolve_attachments, get_render_pass_attachment);

		if (render_target.depth_stencil_attachment.out_node_id.is_valid()) {
			const DepthStencilAttachment& attachment = render_target.depth_stencil_attachment;
			render_pass_key.depth_attachment = get_render_pass_attachment(attachment);
		}

		/* for (int i = 0; i < render_target.inputAttachments.size(); i++) {
			const InputAttachment& attachment = render_target.inputAttachments[i];
			uint32 textureInfoIdx = get_texture_info_index(attachment.nodeID);
			const TextureExecInfo& textureInfo = textureInfos[textureInfoIdx];
			const Texture& texture = *gpuSystem->get_texture_ptr(textureInfo.textureID);

			Attachment& attachmentKey = render_pass_key.inputAttachments[i];
			attachmentKey.format = texture.format;
			if (textureInfo.firstPass.id == pass_index) attachmentKey.flags |= ATTACHMENT_FIRST_PASS_BIT;
			if (textureInfo.lastPass.id == pass_index) attachmentKey.flags |= ATTACHMENT_LAST_PASS_BIT;
			if (is_external(textureInfo)) attachmentKey.flags |= ATTACHMENT_EXTERNAL_BIT;
			attachmentKey.flags |= ATTACHMENT_ACTIVE_BIT;
		}*/

		return gpu_system_->request_render_pass(render_pass_key);
	}

	VkFramebuffer RenderGraphExecution::create_framebuffer(uint32 pass_index, VkRenderPass render_pass) {

		SOUL_ASSERT_MAIN_THREAD();

		const auto graphic_node = soul::downcast<const GraphicBaseNode*>(render_graph_->get_pass_nodes()[pass_index]);

		VkImageView image_views[2 * MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		uint32 image_view_count = 0;
		const RGRenderTarget& render_target = graphic_node->get_render_target();

		for (const ColorAttachment& attachment : render_target.color_attachments) {
			const auto texture_id = get_texture_id(attachment.out_node_id);
			image_views[image_view_count++] = gpu_system_->get_texture_view(texture_id, attachment.desc.view).vk_handle;
		}

		for (const ResolveAttachment& attachment : render_target.resolve_attachments)
		{
			const auto texture_id = get_texture_id(attachment.out_node_id);
			image_views[image_view_count++] = gpu_system_->get_texture_view(texture_id, attachment.desc.view).vk_handle;
		}

		/* for (const InputAttachment& attachment : graphic_node->inputAttachments) {
			const Texture* texture = get_texture_ptr(attachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		} */

		if (render_target.depth_stencil_attachment.out_node_id.is_valid()) {
			const auto info_idx = get_texture_info_index(render_target.depth_stencil_attachment.out_node_id);
			const TextureExecInfo& texture_info = texture_infos_[info_idx];
			const auto depth_attachment_desc = render_target.depth_stencil_attachment.desc;

			image_views[image_view_count++] = gpu_system_->get_texture_view(texture_info.texture_id, depth_attachment_desc.view).vk_handle;
		}

		const VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = render_pass,
			.attachmentCount = image_view_count,
			.pAttachments = image_views,
			.width = render_target.dimension.x,
			.height = render_target.dimension.y,
			.layers = 1
		};

		return gpu_system_->create_framebuffer(framebuffer_info);
	}

	void RenderGraphExecution::sync_external() {

		for (VkEvent& event : external_events_) {
			event = VK_NULL_HANDLE;
		}

		for (auto& buffer_info : external_buffer_infos_)
		{
			if (buffer_info.passes.empty()) continue;
			const auto first_pass_type = render_graph_->get_pass_nodes()[buffer_info.passes[0].id]->get_type();
			const auto owner = gpu_system_->get_buffer(buffer_info.buffer_id).owner;
			const auto external_pass_type = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
			const auto external_queue_type = RESOURCE_OWNER_TO_QUEUE_TYPE[owner];
			SOUL_ASSERT(0, owner != ResourceOwner::PRESENTATION_ENGINE, "");
			if (external_pass_type == first_pass_type) {
				VkEvent& external_event = external_events_[first_pass_type];
				if (external_event == VK_NULL_HANDLE && external_pass_type != PassType::TRANSFER) {
					external_event = gpu_system_->create_event();
				}
				buffer_info.pending_event = external_event;
				buffer_info.pending_semaphore = TimelineSemaphore::null();
				buffer_info.unsync_write_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				buffer_info.unsync_write_access = VK_ACCESS_MEMORY_WRITE_BIT;
			}
			else if (owner != ResourceOwner::NONE) {
				buffer_info.pending_event = VK_NULL_HANDLE;
				buffer_info.pending_semaphore = command_queues_[external_queue_type].get_timeline_semaphore();
				buffer_info.unsync_write_stage = 0;
				buffer_info.unsync_write_access = 0;
			}
		}

		for (auto& texture_info : external_texture_infos_)
		{
			const ResourceOwner owner = gpu_system_->get_texture_ptr(texture_info.texture_id)->owner;
			const PassType external_pass_type = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
			const auto external_queue_type = RESOURCE_OWNER_TO_QUEUE_TYPE[owner];

			std::for_each(texture_info.view, texture_info.view + texture_info.get_view_count(),
			[this, owner, external_pass_type, external_queue_type](TextureViewExecInfo& view_info)
			{
				if (!view_info.passes.empty())
				{
					const PassType first_pass_type = render_graph_->get_pass_nodes()[view_info.passes[0].id]->get_type();
					if (first_pass_type == PassType::NONE) {
						view_info.pending_event = VK_NULL_HANDLE;
						view_info.pending_semaphore = TimelineSemaphore::null();
					}
					else if (owner == ResourceOwner::PRESENTATION_ENGINE) {
						view_info.pending_event = VK_NULL_HANDLE;
						view_info.pending_semaphore = gpu_system_->get_frame_context().image_available_semaphore;
						view_info.unsync_write_stage = 0;
						view_info.unsync_write_access = 0;
					}
					else if (external_pass_type == first_pass_type) {
						VkEvent& external_event = external_events_[first_pass_type];
						if (external_event == VK_NULL_HANDLE && external_pass_type != PassType::TRANSFER) {
							external_event = gpu_system_->create_event();
						}
						view_info.pending_event = external_event;
						view_info.pending_semaphore = TimelineSemaphore::null();
						view_info.unsync_write_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
						view_info.unsync_write_access = VK_ACCESS_MEMORY_WRITE_BIT;
					}
					else if (owner != ResourceOwner::NONE) {
						view_info.pending_event = VK_NULL_HANDLE;
						view_info.pending_semaphore = command_queues_[external_queue_type].get_timeline_semaphore();
						view_info.unsync_write_stage = 0;
						view_info.unsync_write_access = 0;
					}
				}
			});
		}

		for (auto& texture_info : internal_texture_infos_)
		{
			auto ex_texture_info = gpu_system_->get_texture(texture_info.texture_id);
			if (ex_texture_info.owner == ResourceOwner::NONE) continue;
			const auto owner = ex_texture_info.owner;
			const auto external_pass_type = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
			const auto external_queue_type = RESOURCE_OWNER_TO_QUEUE_TYPE[owner];
			std::for_each(texture_info.view, texture_info.view + texture_info.get_view_count(), 
			[this, external_pass_type, external_queue_type](TextureViewExecInfo& view_info)
			{
				if (view_info.passes.empty()) return;
				const auto first_pass_type = render_graph_->get_pass_nodes()[view_info.passes[0].id]->get_type();
				if (first_pass_type == external_pass_type)
				{
					auto& external_event = external_events_[external_pass_type];
					if (external_event == VK_NULL_HANDLE) {
						external_event = gpu_system_->create_event();
					}
					view_info.pending_event = external_event;
					view_info.unsync_write_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
					view_info.unsync_write_access = VK_ACCESS_MEMORY_WRITE_BIT;
				}
				else
				{
					view_info.pending_event = VK_NULL_HANDLE;
					view_info.pending_semaphore = command_queues_[external_queue_type].get_timeline_semaphore();
					view_info.unsync_write_stage = 0;
					view_info.unsync_write_access = 0;
				}
			});
		}

		std::ranges::for_each(texture_infos_, [this](const TextureExecInfo& texture_info)
		{
			const Texture& texture = gpu_system_->get_texture(texture_info.texture_id);
			std::for_each(texture_info.view, texture_info.view + texture_info.get_view_count(), [layout = texture.layout](TextureViewExecInfo& view_info)
			{
				view_info.layout = layout;
			});
		});

		// Sync events
		for(const auto pass_type : FlagIter<PassType>()) {
			if (external_events_[pass_type] != VK_NULL_HANDLE && pass_type != PassType::NONE) {
				const auto queue_type = PASS_TYPE_TO_QUEUE_TYPE_MAP[pass_type];
			    
				const auto sync_event_command_buffer = command_pools_.request_command_buffer(queue_type);
				vkCmdSetEvent(sync_event_command_buffer.get_vk_handle(), external_events_[pass_type], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				command_queues_[queue_type].submit(sync_event_command_buffer);
				
			}
		}

	}

	void RenderGraphExecution::execute_pass(const uint32 pass_index, PrimaryCommandBuffer command_buffer) {
		SOUL_PROFILE_ZONE();
		PassNode* pass_node = render_graph_->get_pass_nodes()[pass_index];
		// Run render pass here
		switch(pass_node->get_type()) {
		case PassType::NONE:
			SOUL_PANIC("Pass Type is unknown!");
			break;
		case PassType::TRANSFER: {

			auto copy_node = soul::downcast<TransferBaseNode*>(pass_node);
			RenderCompiler render_compiler(*gpu_system_, command_buffer.get_vk_handle());
			TransferCommandList command_list(render_compiler);
			RenderGraphRegistry registry(gpu_system_, this);
			copy_node->execute_pass(registry, command_list);

			break;
		}
		case PassType::COMPUTE: {
			auto compute_node = soul::downcast<ComputeBaseNode*>(pass_node);
			RenderCompiler render_compiler(*gpu_system_, command_buffer.get_vk_handle());
			ComputeCommandList command_list(
				command_buffer, 
				render_compiler, *gpu_system_);

            RenderGraphRegistry registry(gpu_system_, this);
            compute_node->execute_pass(registry, command_list);

			break;
		}
		case PassType::GRAPHIC: {

			auto graphic_node = soul::downcast<GraphicBaseNode*>(pass_node);
			SOUL_ASSERT(0, graphic_node != nullptr, "");
			VkRenderPass render_pass = create_render_pass(pass_index);
			VkFramebuffer framebuffer = create_framebuffer(pass_index, render_pass);

			const RGRenderTarget& render_target = graphic_node->get_render_target();
			VkClearValue clear_values[2 * MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
			uint32 clear_count = 0;
			
			for (const ColorAttachment& attachment : render_target.color_attachments) {
				ClearValue clear_value = attachment.desc.clear_value;
				memcpy(&clear_values[clear_count++], &clear_value, sizeof(VkClearValue));
			}

			for (const ResolveAttachment& attachment : render_target.resolve_attachments)
			{
				ClearValue clear_value = attachment.desc.clear_value;
				memcpy(&clear_values[clear_count++], &clear_value, sizeof(VkClearValue));
			}

			if (render_target.depth_stencil_attachment.out_node_id.is_valid()) {
				const DepthStencilAttachmentDesc& desc = render_target.depth_stencil_attachment.desc;
				ClearValue clear_value = desc.clear_value;
				clear_values[clear_count++].depthStencil = { clear_value.depth_stencil.depth, clear_value.depth_stencil.stencil };
			}

			const VkRenderPassBeginInfo render_pass_begin_info = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = render_pass,
				.framebuffer = framebuffer,
				.renderArea = {
					.extent = {
						render_target.dimension.x,
						render_target.dimension.y
					}
				},
				.clearValueCount = clear_count,
				.pClearValues = clear_values
			};
			
			RenderGraphRegistry registry(gpu_system_, this, render_pass, render_target.sample_count);
			GraphicCommandList command_list(
				command_buffer,
				render_pass_begin_info,
				command_pools_,
				*gpu_system_
			);
			graphic_node->execute_pass(registry, command_list);

			gpu_system_->destroy_framebuffer(framebuffer);

			break;
		}
		case PassType::COUNT:
			SOUL_NOT_IMPLEMENTED();
			break;
		}
	}

	void RenderGraphExecution::run() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();

		sync_external();

		Vector<VkEvent> garbage_events;

		Vector<VkBufferMemoryBarrier> pipeline_buffer_barriers;
		Vector<VkImageMemoryBarrier> pipeline_image_barriers;
		Vector<VkBufferMemoryBarrier> event_buffer_barriers;
		Vector<VkImageMemoryBarrier> event_image_barriers;
		
		Vector<VkImageMemoryBarrier> semaphore_layout_barriers;
		Vector<VkEvent> events;

		auto need_invalidate = [](VkAccessFlags visible_access_matrix[], VkAccessFlags access_flags, 
			const VkPipelineStageFlags stage_flags) -> bool {
			auto result = false;
			util::for_each_one_bit_pos(stage_flags, [&result, access_flags, visible_access_matrix](uint32 bit) {
				if (access_flags & ~visible_access_matrix[bit]) {
					result = true;
				}
			});
			return result;
		};


		for (soul_size i = 0; i < render_graph_->get_pass_nodes().size(); i++) {
			runtime::ScopeAllocator passNodeScopeAllocator("Pass Node Scope Allocator", runtime::get_temp_allocator());
			PassNode* pass_node = render_graph_->get_pass_nodes()[i];
			auto queue_type = PASS_TYPE_TO_QUEUE_TYPE_MAP[pass_node->get_type()];
			auto& command_queue = command_queues_[queue_type];
			auto& pass_info = pass_infos_[i];

			const auto cmd_buffer = command_pools_.request_command_buffer(queue_type);

				vec4f color = { (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, 1.0f };
			const VkDebugUtilsLabelEXT passLabel = {
				VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
				nullptr,                                 // pNext
				pass_node->get_name(),                    // pLabelName
				{color.x, color.y, color.z, color.w}, // color
			};
			vkCmdBeginDebugUtilsLabelEXT(cmd_buffer.get_vk_handle(), &passLabel);

			pipeline_buffer_barriers.clear();
			pipeline_image_barriers.clear();
			event_buffer_barriers.clear();
			event_image_barriers.clear();
			semaphore_layout_barriers.clear();
			events.clear();

			VkPipelineStageFlags pipeline_src_stage_flags = 0;
			VkPipelineStageFlags pipeline_dst_stage_flags = 0;
			VkPipelineStageFlags event_src_stage_flags = 0;
			VkPipelineStageFlags event_dst_stage_flags = 0;
			VkPipelineStageFlags semaphore_dst_stage_flags = 0;

			for (const BufferBarrier& barrier: pass_info.buffer_invalidates) {
				BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];

				if (buffer_info.unsync_write_access != 0) {
					for (auto& access_flags : buffer_info.visible_access_matrix) {
						access_flags = 0;
					}
				}

				if (is_semaphore_null(buffer_info.pending_semaphore) &&
					buffer_info.unsync_write_access == 0 &&
					!need_invalidate(buffer_info.visible_access_matrix, barrier.access_flags, barrier.stage_flags)) {
					continue;
				}

				if (is_semaphore_valid(buffer_info.pending_semaphore)) {
				    command_queues_[queue_type].wait(buffer_info.pending_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					buffer_info.pending_semaphore = TimelineSemaphore::null();
				} else {
					if (buffer_info.unsync_write_stage == 0 || buffer_info.unsync_write_stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) 
						SOUL_ASSERT(0, buffer_info.unsync_write_access ==0, "");
					VkBufferMemoryBarrier mem_barrier = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = buffer_info.unsync_write_access,
						.dstAccessMask = barrier.access_flags,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = gpu_system_->get_buffer_ptr(buffer_info.buffer_id)->vk_handle,
						.offset = 0,
						.size = VK_WHOLE_SIZE
					};

					const auto dst_access_flags = barrier.access_flags;

					if (buffer_info.pending_event != VK_NULL_HANDLE)
					{
						event_buffer_barriers.push_back(mem_barrier);
						events.push_back(buffer_info.pending_event);
						event_src_stage_flags |= buffer_info.unsync_write_stage;
						event_dst_stage_flags |= barrier.stage_flags;
						buffer_info.pending_event = VK_NULL_HANDLE;
					}
					else
					{
						pipeline_buffer_barriers.push_back(mem_barrier);
						pipeline_src_stage_flags |= buffer_info.unsync_write_stage;
						pipeline_dst_stage_flags |= barrier.stage_flags;
					}
					
					util::for_each_one_bit_pos(barrier.stage_flags, [&buffer_info, dst_access_flags](const uint32 bit) {
						buffer_info.visible_access_matrix[bit] |= dst_access_flags;
					});

					
				}
			}

			for (const TextureBarrier& barrier: pass_info.texture_invalidates) {
				TextureExecInfo& texture_info = texture_infos_[barrier.texture_info_idx];
				Texture* texture = gpu_system_->get_texture_ptr(texture_info.texture_id);
				TextureViewExecInfo& view_info = *texture_info.get_view(barrier.view);

				const auto layout_change = view_info.layout != barrier.layout;

				if (view_info.unsync_write_access != 0 || layout_change) {
					for (auto& access_flags : view_info.visible_access_matrix) {
						access_flags = 0;
					}
				}

				if (is_semaphore_null(view_info.pending_semaphore) &&
					!layout_change &&
					view_info.unsync_write_access == 0 &&
					!need_invalidate(view_info.visible_access_matrix, barrier.access_flags, barrier.stage_flags)) {
					continue;
				}

				VkImageMemoryBarrier mem_barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.oldLayout = view_info.layout,
					.newLayout = barrier.layout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = texture->vk_handle,
					.subresourceRange = {
						.aspectMask = vk_cast_format_to_aspect_flags(texture->desc.format),
						.baseMipLevel = barrier.view.get_level(),
						.levelCount = 1,
						.baseArrayLayer = barrier.view.get_layer(),
						.layerCount = 1
					}
				};

				if (is_semaphore_valid(view_info.pending_semaphore)) {
					semaphore_dst_stage_flags |= barrier.stage_flags;
					if (layout_change) {

						mem_barrier.srcAccessMask = 0;
						mem_barrier.dstAccessMask = barrier.access_flags;
						semaphore_layout_barriers.push_back(mem_barrier);

					}
					command_queue.wait(view_info.pending_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					view_info.pending_semaphore = TimelineSemaphore::null();
				} else {
					const auto dst_access_flags = barrier.access_flags;

					mem_barrier.srcAccessMask = view_info.unsync_write_access;
					mem_barrier.dstAccessMask = dst_access_flags;

					if (view_info.pending_event != VK_NULL_HANDLE)
					{
						event_image_barriers.push_back(mem_barrier);
						events.push_back(view_info.pending_event);
						event_src_stage_flags |= view_info.unsync_write_stage;
						event_dst_stage_flags |= barrier.stage_flags;
						view_info.pending_event = VK_NULL_HANDLE;
					}
					else
					{
						pipeline_image_barriers.push_back(mem_barrier);
						pipeline_src_stage_flags |= view_info.unsync_write_stage;
						pipeline_dst_stage_flags |= barrier.stage_flags;
					}
					
					util::for_each_one_bit_pos(barrier.stage_flags, [&view_info, dst_access_flags](uint32 bit) {
						view_info.visible_access_matrix[bit] |= dst_access_flags;
					});
					
				}

				view_info.layout = barrier.layout;
			}

			if (!pipeline_buffer_barriers.empty() || !pipeline_image_barriers.empty())
			{
				if (pipeline_src_stage_flags == 0) {
				    pipeline_src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				}
				SOUL_ASSERT(0, pipeline_dst_stage_flags != 0, "");
				vkCmdPipelineBarrier(
				    cmd_buffer.get_vk_handle(),
				    pipeline_src_stage_flags, pipeline_dst_stage_flags,
				    0,
				    0, nullptr,
				    soul::cast<uint32>(pipeline_buffer_barriers.size()), pipeline_buffer_barriers.data(),
				    soul::cast<uint32>(pipeline_image_barriers.size()), pipeline_image_barriers.data()
				);
			}

			if (!events.empty()) {
				vkCmdWaitEvents(cmd_buffer.get_vk_handle(),
                    soul::cast<uint32>(events.size()), events.data(),
                    event_src_stage_flags, event_dst_stage_flags,
                    0, nullptr,
                    soul::cast<uint32>(event_buffer_barriers.size()), event_buffer_barriers.data(),
                    soul::cast<uint32>(event_image_barriers.size()), event_image_barriers.data()
				);
			}

			if (!semaphore_layout_barriers.empty()) {
			    vkCmdPipelineBarrier(cmd_buffer.get_vk_handle(),
                    semaphore_dst_stage_flags, semaphore_dst_stage_flags,
                    0,
                    0, nullptr,
                    0, nullptr,
                    soul::cast<uint32>(semaphore_layout_barriers.size()), semaphore_layout_barriers.data());
			}

			execute_pass(i, cmd_buffer);

			FlagMap<PassType, bool> is_pass_type_dependent(false);
			for (const BufferBarrier& barrier : pass_info.buffer_flushes) {
				const BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];
				if (buffer_info.pass_counter != buffer_info.passes.size() - 1) {
					uint32 next_pass_idx = buffer_info.passes[buffer_info.pass_counter + 1].id;
					PassType pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					is_pass_type_dependent[pass_type] = true;
				}
			}

			for (const TextureBarrier& barrier: pass_info.texture_flushes) {
				const TextureExecInfo& texture_info = texture_infos_[barrier.texture_info_idx];
				if (texture_info.first_pass.is_null()) continue;
				const TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
				if (texture_view_info.pass_counter != texture_view_info.passes.size() - 1)
				{
					uint32 next_pass_idx = texture_view_info.passes[texture_view_info.pass_counter + 1].id;
					PassType pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					is_pass_type_dependent[pass_type] = true;
				}
			}

			FlagMap<PassType, TimelineSemaphore> semaphores_map(TimelineSemaphore::null());
			VkEvent event = VK_NULL_HANDLE;
			VkPipelineStageFlags unsync_write_stage_flags = 0;

			for (PassType pass_type : FlagIter<PassType>()) {
				if (is_pass_type_dependent[pass_type] && pass_type == pass_node->get_type() && pass_type != PassType::TRANSFER) {
					event = gpu_system_->create_event();
					garbage_events.push_back(event);
				}
			}

			SBOVector<Semaphore*> pending_semaphores;

			for (const BufferBarrier& barrier : pass_info.buffer_flushes) {
				BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];
				if (buffer_info.pass_counter != buffer_info.passes.size() - 1) {
					const auto next_pass_idx = buffer_info.passes[buffer_info.pass_counter + 1].id;
					PassType next_pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();

					if (pass_node->get_type() != next_pass_type) {
						pending_semaphores.push_back(&buffer_info.pending_semaphore);
					}
					else {
						buffer_info.pending_event = event;
						unsync_write_stage_flags |= barrier.stage_flags;
					}
				}
			}

			for (const TextureBarrier& barrier : pass_info.texture_flushes) {
				TextureExecInfo& texture_info = texture_infos_[barrier.texture_info_idx];
				TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
				if (texture_view_info.pass_counter != texture_view_info.passes.size() - 1) {
					uint32 next_pass_idx = texture_view_info.passes[texture_view_info.pass_counter + 1].id;
					PassType next_pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					if (pass_node->get_type() != next_pass_type) {
						pending_semaphores.push_back(&texture_view_info.pending_semaphore);
					}
					else {
						texture_view_info.pending_event = event;
						unsync_write_stage_flags |= barrier.stage_flags;
					}
				}
				texture_view_info.layout = barrier.layout;
			}

			if (event != VK_NULL_HANDLE) {
				vkCmdSetEvent(cmd_buffer.get_vk_handle(), event, unsync_write_stage_flags);
			}

			vkCmdEndDebugUtilsLabelEXT(cmd_buffer.get_vk_handle());
			command_queue.submit(cmd_buffer);

			for(Semaphore* pending_semaphore : pending_semaphores)
				*pending_semaphore = command_queue.get_timeline_semaphore();

			auto update_resource_unsync_status = [this](const PassType pass_type,
				VkAccessFlags barrier_access_flags,
				VkPipelineStageFlags event_stage_flags,
				auto& resource_info) {
					const bool is_resource_in_last_pass = resource_info.pass_counter != resource_info.passes.size() - 1;
					if (is_resource_in_last_pass) {
						const uint32 next_pass_idx = resource_info.passes[resource_info.pass_counter + 1].id;
						const PassType next_pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
						if (pass_type != next_pass_type) {
							resource_info.unsync_write_access = 0;
							resource_info.unsync_write_stage = 0;
						}
						else {
							resource_info.unsync_write_access = barrier_access_flags;
							resource_info.unsync_write_stage = event_stage_flags;
						}
					}
			};

			// Update unsync stage
			for (const BufferBarrier& barrier : pass_info.buffer_flushes) {
				BufferExecInfo& buffer_info = buffer_infos_[barrier.buffer_info_idx];
				update_resource_unsync_status(pass_node->get_type(), barrier.access_flags, unsync_write_stage_flags, buffer_info);
				buffer_info.pass_counter += 1;
			}

			for (const TextureBarrier& barrier : pass_info.texture_flushes) {
				TextureExecInfo& texture_info = texture_infos_[barrier.texture_info_idx];
				TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
				update_resource_unsync_status(pass_node->get_type(), barrier.access_flags, unsync_write_stage_flags, texture_view_info);
				texture_view_info.pass_counter += 1;
			}
		}

		// Update resource owner for external resource
		static constexpr auto PASS_TYPE_TO_RESOURCE_OWNER = FlagMap<PassType, ResourceOwner>::build_from_list({
			ResourceOwner::NONE,
			ResourceOwner::GRAPHIC_QUEUE,
			ResourceOwner::COMPUTE_QUEUE,
			ResourceOwner::TRANSFER_QUEUE
		});

		for (const TextureExecInfo& texture_info : external_texture_infos_) {
			Texture* texture = gpu_system_->get_texture_ptr(texture_info.texture_id);

			VkImageLayout layout = texture_info.view->layout;
			SOUL_ASSERT(0, std::all_of(texture_info.view, texture_info.view + texture_info.get_view_count(),
				[layout](const TextureViewExecInfo& view_info)
				{
					return view_info.layout == layout;
				}), "");

			uint64 last_pass_idx = texture_info.view->passes.back().id;
			SOUL_ASSERT(0, std::all_of(texture_info.view, texture_info.view + texture_info.get_view_count(),
				[last_pass_idx](const TextureViewExecInfo& view_info)
				{
					return view_info.passes.back().id == last_pass_idx;
				}), "");

			PassType last_pass_type = render_graph_->get_pass_nodes()[last_pass_idx]->get_type();
			texture->owner = PASS_TYPE_TO_RESOURCE_OWNER[last_pass_type];
			texture->layout = layout;
		}

		for (const BufferExecInfo& buffer_info : buffer_infos_) {
			if (buffer_info.passes.empty()) continue;
			Buffer* buffer = gpu_system_->get_buffer_ptr(buffer_info.buffer_id);
			uint64 last_pass_idx = buffer_info.passes.back().id;
			PassType last_pass_type = render_graph_->get_pass_nodes()[last_pass_idx]->get_type();
			buffer->owner = PASS_TYPE_TO_RESOURCE_OWNER[last_pass_type];
		}

		for (VkEvent event : garbage_events) {
			gpu_system_->destroy_event(event);
		}

	}

	bool RenderGraphExecution::is_external(const BufferExecInfo& info) const {
		return soul::cast<uint32>(&info - buffer_infos_.data()) >= render_graph_->get_internal_buffers().size();
	}

	bool RenderGraphExecution::is_external(const TextureExecInfo& info) const {
		return soul::cast<uint32>(&info - texture_infos_.data()) >= render_graph_->get_internal_textures().size();
	}

	BufferID RenderGraphExecution::get_buffer_id(const BufferNodeID node_id) const {
		const auto info_idx = get_buffer_info_index(node_id);
		return buffer_infos_[info_idx].buffer_id;
	}

	TextureID RenderGraphExecution::get_texture_id(const TextureNodeID node_id) const {
		const auto info_idx = get_texture_info_index(node_id);
		return texture_infos_[info_idx].texture_id;
	}

	Buffer* RenderGraphExecution::get_buffer(const BufferNodeID node_id) const {
		return gpu_system_->get_buffer_ptr(get_buffer_id(node_id));
	}

	Texture* RenderGraphExecution::get_texture(const TextureNodeID node_id) const {
		return gpu_system_->get_texture_ptr(get_texture_id(node_id));
	}

	uint32 RenderGraphExecution::get_buffer_info_index(const BufferNodeID node_id) const {
		const auto& node = render_graph_->get_buffer_node(node_id);
		if (node.resource_id.is_external()) {
			return soul::cast<uint32>(render_graph_->get_internal_buffers().size()) + node.resource_id.get_index();
		}
		return node.resource_id.get_index();
	}

	uint32 RenderGraphExecution::get_texture_info_index(TextureNodeID nodeID) const {
		const auto& node = render_graph_->get_texture_node(nodeID);
		if (node.resource_id.is_external()) {
			return soul::cast<uint32>(render_graph_->get_internal_textures().size()) + node.resource_id.get_index();
		}
		return node.resource_id.get_index();
	}

	void RenderGraphExecution::cleanup() {

		for (const auto event : external_events_) {
			if (event != VK_NULL_HANDLE) gpu_system_->destroy_event(event);
		}

		for (const auto& texture_info : internal_texture_infos_) {
			gpu_system_->destroy_texture(texture_info.texture_id);
		}

	}

	void RenderGraphExecution::init_shader_buffers(const Vector<ShaderBufferReadAccess>& access_list, const soul_size index, const QueueType queue_type) {
		PassExecInfo& pass_info = pass_infos_[index];
		for (const ShaderBufferReadAccess& shader_access : access_list) {
			SOUL_ASSERT(0, shader_access.node_id.is_valid(), "");

			const auto buffer_info_id = get_buffer_info_index(shader_access.node_id);

			const auto stage_flags = vk_cast_shader_stage_to_pipeline_stage_flags(shader_access.stage_flags);

			BufferBarrier invalidate_barrier;
			invalidate_barrier.stage_flags = stage_flags;
			invalidate_barrier.access_flags = VK_ACCESS_SHADER_READ_BIT;
			invalidate_barrier.buffer_info_idx = buffer_info_id;
			pass_info.buffer_invalidates.push_back(invalidate_barrier);

			BufferBarrier flush_barrier;
			flush_barrier.stage_flags = stage_flags;
			flush_barrier.access_flags = 0;
			flush_barrier.buffer_info_idx = buffer_info_id;
			pass_info.buffer_flushes.push_back(flush_barrier);

			update_buffer_info(queue_type, impl::get_buffer_usage_flags(shader_access.usage), PassNodeID(index), &buffer_infos_[buffer_info_id]);
		}

	}

	void RenderGraphExecution::init_shader_buffers(const Vector<ShaderBufferWriteAccess>& access_list, const soul_size index, const QueueType queue_type) {
		PassExecInfo& passInfo = pass_infos_[index];
		for (const auto& shader_access : access_list) {
			SOUL_ASSERT(0, shader_access.output_node_id.is_valid(), "");

            const auto buffer_info_id = get_buffer_info_index(shader_access.output_node_id);

            const auto stage_flags = vk_cast_shader_stage_to_pipeline_stage_flags(shader_access.stage_flags);

			BufferBarrier invalidate_barrier;
			invalidate_barrier.stage_flags = stage_flags;
			invalidate_barrier.access_flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			invalidate_barrier.buffer_info_idx = buffer_info_id;
			passInfo.buffer_invalidates.push_back(invalidate_barrier);

			BufferBarrier flush_barrier;
			flush_barrier.stage_flags = stage_flags;
			flush_barrier.access_flags = VK_ACCESS_SHADER_WRITE_BIT;
			flush_barrier.buffer_info_idx = buffer_info_id;
			passInfo.buffer_flushes.push_back(flush_barrier);

			update_buffer_info(queue_type, get_buffer_usage_flags(shader_access.usage), PassNodeID(index), &buffer_infos_[buffer_info_id]);
		}

	}

	void RenderGraphExecution::init_shader_textures(const Vector<ShaderTextureReadAccess>& access_list, const soul_size index, const QueueType queue_type) {
		PassExecInfo& pass_info = pass_infos_[index];
		for (const auto& shader_access : access_list) {
			SOUL_ASSERT(0, shader_access.node_id.is_valid(), "");

			const auto texture_info_id = get_texture_info_index(shader_access.node_id);
			VkPipelineStageFlags stage_flags = vk_cast_shader_stage_to_pipeline_stage_flags(shader_access.stage_flags);

			update_texture_info(queue_type, get_texture_usage_flags(shader_access.usage),PassNodeID(index), shader_access.view_range, &texture_infos_[texture_info_id]);

			auto is_writable = [](const ShaderTextureReadUsage usage) -> bool {
				auto mapping = FlagMap<ShaderTextureReadUsage, bool>::build_from_list({
					false,
					true
				});
				return mapping[usage];
			};

			auto image_layout = is_writable(shader_access.usage) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::ranges::transform(shader_access.view_range, std::back_inserter(pass_info.texture_invalidates),
		[stage_flags, image_layout, texture_info_id](SubresourceIndex view_index) -> TextureBarrier
			{
				return {
					.stage_flags = stage_flags,
					.access_flags = VK_ACCESS_SHADER_READ_BIT,
					.layout = image_layout,
					.texture_info_idx = texture_info_id,
					.view = view_index
				};
			});

			std::ranges::transform(shader_access.view_range, std::back_inserter(pass_info.texture_flushes),
        [stage_flags, image_layout, texture_info_id](SubresourceIndex view_index) -> TextureBarrier 
            {
                return {
                    .stage_flags = stage_flags,
                    .access_flags = 0,
                    .layout = image_layout,
                    .texture_info_idx = texture_info_id,
                    .view = view_index
                };
            });
		}
	}

	void RenderGraphExecution::init_shader_textures(const Vector<ShaderTextureWriteAccess>& access_list, const soul_size index, const QueueType queue_type) {
		PassExecInfo& pass_info = pass_infos_[index];
		for (const auto& shader_access : access_list) {
			SOUL_ASSERT(0, shader_access.output_node_id.is_valid(), "");

			const auto texture_info_id = get_texture_info_index(shader_access.output_node_id);
			const auto stage_flags = vk_cast_shader_stage_to_pipeline_stage_flags(shader_access.stage_flags);

			update_texture_info(queue_type, get_texture_usage_flags(shader_access.usage), PassNodeID(index), shader_access.view_range, &texture_infos_[texture_info_id]);

			std::ranges::transform(shader_access.view_range, std::back_inserter(pass_info.texture_invalidates),
        [stage_flags, texture_info_id](const SubresourceIndex& view_index) -> TextureBarrier
            {
                return {
                    .stage_flags = stage_flags,
                    .access_flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                    .layout = VK_IMAGE_LAYOUT_GENERAL,
                    .texture_info_idx = texture_info_id,
                    .view = view_index
                };
            });

			std::ranges::transform(shader_access.view_range, std::back_inserter(pass_info.texture_flushes),
		[stage_flags, texture_info_id](SubresourceIndex view_index) -> TextureBarrier
            {
                return {
                    .stage_flags = stage_flags,
                    .access_flags =VK_ACCESS_SHADER_WRITE_BIT,
                    .layout = VK_IMAGE_LAYOUT_GENERAL,
                    .texture_info_idx = texture_info_id,
                    .view = view_index
                };
            });

		}
	}
	
}
