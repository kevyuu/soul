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
	auto SHADER_BUFFER_WRITE_USAGE_MAP = ShaderBufferWriteMap::build_from_list(
{
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

		buffer_info->usageFlags |= usage_flags;
		buffer_info->queueFlags |= { queue_type };
		if (buffer_info->firstPass.is_null()) {
			buffer_info->firstPass = pass_id;
		}
		buffer_info->lastPass = pass_id;
		buffer_info->passes.push_back(pass_id);
	};
	

	void update_texture_info(const QueueType queue_type, const TextureUsageFlags usage_flags,
	                         const PassNodeID pass_id, const SubresourceIndexRange view_index_range, TextureExecInfo* texture_info)
	{

		texture_info->usageFlags |= usage_flags;
		texture_info->queueFlags |= { queue_type };
		if (texture_info->firstPass.is_null()) { 
			texture_info->firstPass = pass_id;
		}
		texture_info->lastPass = pass_id;

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
		auto fold_external_texture_view_count = [gpu_system = this->gpu_system_](soul_size count, const RGExternalTexture& external_texture)
		{
			const gpu::TextureDesc& desc = gpu_system->get_texture(external_texture.textureID).desc;
			return count + desc.get_view_count();
		};

		const soul_size internal_view_count = std::accumulate(internal_textures.begin(), internal_textures.end(), 0u, fold_internal_texture_view_count);
		const soul_size external_view_count = std::accumulate(external_textures.begin(), external_textures.end(), 0u, fold_external_texture_view_count);
		texture_view_infos_.resize(internal_view_count + external_view_count);

		for (soul_size texture_info_idx = 0, view_offset = 0; texture_info_idx < internal_texture_infos_.size(); texture_info_idx++)
		{
			TextureExecInfo& texture_info = internal_texture_infos_[texture_info_idx];
			texture_info.view = texture_view_infos_.data() + view_offset;
			texture_info.mipLevels = internal_textures[texture_info_idx].mipLevels;
			texture_info.layers = internal_textures[texture_info_idx].layerCount;
			view_offset += texture_info.get_view_count();
		}

		for (soul_size texture_info_idx = 0, view_offset = internal_view_count; texture_info_idx < external_texture_infos_.size(); texture_info_idx++)
		{
			TextureExecInfo& texture_info = external_texture_infos_[texture_info_idx];
			texture_info.view = texture_view_infos_.data() + view_offset;
			const TextureDesc& desc = gpu_system_->get_texture(external_textures[texture_info_idx].textureID).desc;
			texture_info.mipLevels = desc.mipLevels;
			texture_info.layers = desc.layerCount;
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
					auto compute_node = static_cast<const ComputeBaseNode*>(&pass_node);
                    init_shader_buffers(compute_node->get_buffer_read_accesses(), i, QueueType::COMPUTE);
                    init_shader_buffers(compute_node->get_buffer_write_accesses(), i, QueueType::COMPUTE);
                    init_shader_textures(compute_node->get_texture_read_accesses(), i, QueueType::COMPUTE);
                    init_shader_textures(compute_node->get_texture_write_accesses(), i, QueueType::COMPUTE);
				break;
			}
			case PassType::GRAPHIC: {
				auto graphic_node = static_cast<const GraphicBaseNode*>(&pass_node);
				init_shader_buffers(graphic_node->get_buffer_read_accesses(), i, QueueType::GRAPHIC);
				init_shader_buffers(graphic_node->get_buffer_write_accesses(), i, QueueType::GRAPHIC);
				init_shader_textures(graphic_node->get_texture_read_accesses(), i, QueueType::GRAPHIC);
				init_shader_textures(graphic_node->get_texture_write_accesses(), i, QueueType::GRAPHIC);

				for (const BufferNodeID node_id : graphic_node->get_vertex_buffers()) {
					SOUL_ASSERT(0, node_id.is_valid(), "");

					uint32 buffer_info_id = get_buffer_info_index(node_id);

					pass_info.bufferInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
						.accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						.bufferInfoIdx = buffer_info_id
					});

					pass_info.bufferInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
						.accessFlags = 0,
						.bufferInfoIdx = buffer_info_id
					});

					update_buffer_info(QueueType::GRAPHIC, { BufferUsage::VERTEX }, pass_node_id, &buffer_infos_[buffer_info_id]);
				}

				for (const BufferNodeID node_id : graphic_node->get_index_buffers()) {
					SOUL_ASSERT(0, node_id.is_valid(), "");

					uint32 buffer_info_id = get_buffer_info_index(node_id);

					pass_info.bufferInvalidates.add(BufferBarrier());
					BufferBarrier& invalidate_barrier = pass_info.bufferInvalidates.back();
					invalidate_barrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					invalidate_barrier.accessFlags = VK_ACCESS_INDEX_READ_BIT;
					invalidate_barrier.bufferInfoIdx = buffer_info_id;

					pass_info.bufferFlushes.add(BufferBarrier());
					BufferBarrier& flush_barrier = pass_info.bufferFlushes.back();
					flush_barrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					flush_barrier.accessFlags = 0;
					flush_barrier.bufferInfoIdx = buffer_info_id;

					update_buffer_info(QueueType::GRAPHIC, { BufferUsage::INDEX }, pass_node_id, &buffer_infos_[buffer_info_id]);
				}

				const auto& [dimension, sampleCount, colorAttachments, 
					resolveAttachments, depthStencilAttachment] = graphic_node->get_render_target();

				for (const ColorAttachment& color_attachment : colorAttachments) {
					SOUL_ASSERT(0, color_attachment.outNodeID.is_valid(), "");

					uint32 texture_info_id = get_texture_info_index(color_attachment.outNodeID);
					update_texture_info(QueueType::GRAPHIC, { TextureUsage::COLOR_ATTACHMENT },
						pass_node_id, { color_attachment.desc.view, 1, 1 }, & texture_infos_[texture_info_id]);

					pass_info.textureInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.textureInfoIdx = texture_info_id,
						.view =color_attachment.desc.view
					});

					pass_info.textureFlushes.push_back({
						.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.textureInfoIdx = texture_info_id,
						.view = color_attachment.desc.view
					});

				}

				for (const ResolveAttachment& resolve_attachment : resolveAttachments)
				{
					uint32 texture_info_id = get_texture_info_index(resolve_attachment.outNodeID);
					update_texture_info(QueueType::GRAPHIC, { TextureUsage::COLOR_ATTACHMENT },
						pass_node_id, { resolve_attachment.desc.view, 1, 1 }, & texture_infos_[texture_info_id]);

					pass_info.textureInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.textureInfoIdx = texture_info_id,
						.view = resolve_attachment.desc.view
					});

					pass_info.textureFlushes.push_back({
						.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						.textureInfoIdx = texture_info_id,
						.view = resolve_attachment.desc.view
					});

				}

				/* for (const InputAttachment& inputAttachment : graphic_node->inputAttachments) {

					uint32 textureInfoID = get_texture_info_index(inputAttachment.nodeID);
					updateTextureInfo(&textureInfos[textureInfoID], QueueType::GRAPHIC,
						{ TextureUsage::INPUT_ATTACHMENT }, pass_node_id);

					TextureBarrier invalidateBarrier;
					invalidateBarrier.stageFlags = vkCastShaderStageToPipelineStageFlags(inputAttachment.stageFlags);
					invalidateBarrier.accessFlags = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
					invalidateBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					invalidateBarrier.textureInfoIdx = textureInfoID;
					pass_info.textureInvalidates.add(invalidateBarrier);

					TextureBarrier flushBarrier;
					flushBarrier.stageFlags = vkCastShaderStageToPipelineStageFlags(inputAttachment.stageFlags);
					flushBarrier.accessFlags = 0;
					flushBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					flushBarrier.textureInfoIdx = textureInfoID;
					pass_info.textureFlushes.add(flushBarrier);
				}*/

				if (depthStencilAttachment.outNodeID.is_valid()) {
					uint32 resource_info_index = get_texture_info_index(depthStencilAttachment.outNodeID);

					uint32 texture_info_id = get_texture_info_index(depthStencilAttachment.outNodeID);
					update_texture_info(QueueType::GRAPHIC, { TextureUsage::DEPTH_STENCIL_ATTACHMENT },
						pass_node_id, { depthStencilAttachment.desc.view, 1, 1 }, & texture_infos_[texture_info_id]);

					pass_info.textureInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.textureInfoIdx = resource_info_index,
						.view = depthStencilAttachment.desc.view
					});

					pass_info.textureFlushes.push_back({
						.stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.textureInfoIdx = resource_info_index,
						.view = depthStencilAttachment.desc.view
					});
				}

				break;
			}
			case PassType::TRANSFER:
			{
				const auto copy_node = static_cast<const CopyBaseNode*>(&pass_node);
				for (const auto& source_buffer : copy_node->get_source_buffers())
				{
					const uint32 resource_info_index = get_buffer_info_index(source_buffer.nodeID);
					update_buffer_info(QueueType::TRANSFER, { BufferUsage::TRANSFER_SRC }, pass_node_id, &buffer_infos_[resource_info_index]);

					pass_info.bufferInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.accessFlags = VK_ACCESS_TRANSFER_READ_BIT,
						.bufferInfoIdx = resource_info_index
					});

					pass_info.bufferFlushes.push_back({
						.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.accessFlags = 0,
						.bufferInfoIdx = resource_info_index
					});
				}

				for (const auto& dst_buffer : copy_node->get_destination_buffers())
				{
					const uint32 resource_info_index = get_buffer_info_index(dst_buffer.outputNodeID);
					update_buffer_info(QueueType::TRANSFER, { BufferUsage::TRANSFER_DST }, pass_node_id, &buffer_infos_[resource_info_index]);

					pass_info.bufferInvalidates.push_back({
						.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT,
						.bufferInfoIdx = resource_info_index
					});

					pass_info.bufferFlushes.push_back({
						.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
						.accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT,
						.bufferInfoIdx = resource_info_index
					});
				}

				for(const auto& src_texture : copy_node->get_source_textures())
				{
					const uint32 resource_info_index = get_texture_info_index(src_texture.nodeID);
					update_texture_info(QueueType::TRANSFER, { TextureUsage::TRANSFER_SRC }, pass_node_id, src_texture.viewRange, &texture_infos_[resource_info_index]);

					auto generate_invalidate_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.accessFlags = VK_ACCESS_TRANSFER_READ_BIT,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							.textureInfoIdx = resource_info_index,
							.view = view_index
						};
					};
					std::ranges::transform(src_texture.viewRange, std::back_inserter(pass_info.textureInvalidates), generate_invalidate_barrier);

					auto generate_flush_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.accessFlags = 0,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							.textureInfoIdx = resource_info_index,
							.view = view_index
						};
					};
					std::ranges::transform(src_texture.viewRange, std::back_inserter(pass_info.textureFlushes), generate_flush_barrier);

				}

				for (const auto& dst_texture : copy_node->get_destination_textures())
				{
					const uint32 resource_info_index = get_texture_info_index(dst_texture.outputNodeID);
					update_texture_info(QueueType::TRANSFER, { TextureUsage::TRANSFER_DST }, pass_node_id, dst_texture.viewRange, &texture_infos_[resource_info_index]);

					auto generate_invalidate_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							.textureInfoIdx = resource_info_index
						};
					};
					std::ranges::transform(dst_texture.viewRange, std::back_inserter(pass_info.textureInvalidates), generate_invalidate_barrier);

					auto generate_flush_barrier = [resource_info_index](SubresourceIndex view_index) -> TextureBarrier
					{
						return {
							.stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
							.accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT,
							.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							.textureInfoIdx = resource_info_index
						};
					};
					std::ranges::transform(dst_texture.viewRange, std::back_inserter(pass_info.textureFlushes), generate_flush_barrier);
				}
				break;
			}
			case PassType::COUNT:
			default:
				SOUL_NOT_IMPLEMENTED();
				break;
			}
		}

		for (VkEvent& event : external_events_) {
			event = VK_NULL_HANDLE;
		}

		for (auto src_pass_type : FlagIter<PassType>()) {
			for (auto dst_pass_type : FlagIter<PassType>()) {
				external_semaphores_[src_pass_type][dst_pass_type] = SemaphoreID::Null();
			}
		}

		for (soul_size i = 0; i < render_graph_->get_internal_buffers().size(); i++) {
			const RGInternalBuffer& rg_buffer = render_graph_->get_internal_buffers()[i];
			BufferExecInfo& buffer_info = buffer_infos_[i];

			buffer_info.bufferID = gpu_system_->create_buffer({
				.count = rg_buffer.count,
				.typeSize = rg_buffer.typeSize,
				.typeAlignment = rg_buffer.typeAlignment,
				.usageFlags = buffer_info.usageFlags,
				.queueFlags = buffer_info.queueFlags
			});
		}

		for (soul_size i = 0; i < external_buffer_infos_.size(); i++) {
			BufferExecInfo& buffer_info = external_buffer_infos_[i];
			if (buffer_info.passes.empty()) continue;

			buffer_info.bufferID = render_graph_->get_external_buffers()[i].bufferID;

			PassType first_pass_type = render_graph_->get_pass_nodes()[buffer_info.passes[0].id]->get_type();
			ResourceOwner owner = gpu_system_->get_buffer_ptr(buffer_info.bufferID)->owner;
			PassType external_pass_type = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
			SOUL_ASSERT(0, owner != ResourceOwner::PRESENTATION_ENGINE, "");
			if (external_pass_type == first_pass_type) {
				VkEvent& external_event = external_events_[first_pass_type];
				if (external_event == VK_NULL_HANDLE) {
					external_event = gpu_system_->create_event();
				}
				buffer_info.pendingEvent = external_event;
				buffer_info.pendingSemaphore = SemaphoreID::Null();
				buffer_info.unsyncWriteStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				buffer_info.unsyncWriteAccess = VK_ACCESS_MEMORY_WRITE_BIT;
			} else {
				SemaphoreID& externalSemaphore = external_semaphores_[external_pass_type][first_pass_type];
				if (externalSemaphore == SemaphoreID::Null()) {
					externalSemaphore = gpu_system_->create_semaphore();
				}

				buffer_info.pendingEvent = VK_NULL_HANDLE;
				buffer_info.pendingSemaphore = externalSemaphore;
				buffer_info.unsyncWriteStage = 0;
				buffer_info.unsyncWriteAccess = 0;
			}

		}

		for (soul_size i = 0; i < render_graph_->get_internal_textures().size(); i++) {
			const RGInternalTexture& rg_texture = render_graph_->get_internal_textures()[i];
			TextureExecInfo& texture_info = texture_infos_[i];

			if (texture_info.queueFlags.none()) continue;

			TextureDesc desc = {
				.type = rg_texture.type,
				.format = rg_texture.format,
				.extent = rg_texture.extent,
				.mipLevels = rg_texture.mipLevels,
				.sampleCount = rg_texture.sampleCount,
				.usageFlags = texture_info.usageFlags,
				.queueFlags = texture_info.queueFlags,
				.name = rg_texture.name
			};
			if (!rg_texture.clear) {
				texture_info.textureID = gpu_system_->create_texture(desc);
			} else {
				desc.usageFlags |= { TextureUsage::SAMPLED };
				texture_info.textureID = gpu_system_->create_texture(desc, rg_texture.clearValue);
				
				std::for_each(texture_info.view, texture_info.view + texture_info.get_view_count(), [this](TextureViewExecInfo& view_info)
				{
					if (view_info.passes.empty()) return;
					PassType firstPassType = render_graph_->get_pass_nodes()[view_info.passes[0].id]->get_type();
					if (firstPassType == PassType::GRAPHIC)
					{
						VkEvent& externalEvent = external_events_[PassType::GRAPHIC];
						if (externalEvent == VK_NULL_HANDLE) {
							externalEvent = gpu_system_->create_event();
						}
						view_info.pendingEvent = externalEvent;
						view_info.pendingSemaphore = SemaphoreID::Null();
						view_info.unsyncWriteStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
						view_info.unsyncWriteAccess = VK_ACCESS_MEMORY_WRITE_BIT;
					}
					else
					{
						SemaphoreID& externalSemaphoreID = external_semaphores_[PassType::GRAPHIC][firstPassType];
						if (externalSemaphoreID == SemaphoreID::Null()) {
							externalSemaphoreID = gpu_system_->create_semaphore();
						}
						view_info.pendingEvent = VK_NULL_HANDLE;
						view_info.pendingSemaphore = externalSemaphoreID;
						view_info.unsyncWriteStage = 0;
						view_info.unsyncWriteAccess = 0;
					}
				});
			}
		}

		for (soul_size i = 0; i < external_texture_infos_.size(); i++) {
			TextureExecInfo& texture_info = external_texture_infos_[i];
			texture_info.textureID = render_graph_->get_external_textures()[i].textureID;

			const ResourceOwner owner = gpu_system_->get_texture_ptr(texture_info.textureID)->owner;
			const PassType external_pass_type = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];

			std::for_each(texture_info.view, texture_info.view + texture_info.get_view_count(), [this, owner, external_pass_type](TextureViewExecInfo& view_info)
			{
				if (!view_info.passes.empty())
				{
					const PassType first_pass_type = render_graph_->get_pass_nodes()[view_info.passes[0].id]->get_type();
					if (first_pass_type == PassType::NONE) {
						view_info.pendingEvent = VK_NULL_HANDLE;
						view_info.pendingSemaphore = SemaphoreID::Null();
					}
					else if (owner == ResourceOwner::PRESENTATION_ENGINE) {
						view_info.pendingEvent = VK_NULL_HANDLE;
						view_info.pendingSemaphore = gpu_system_->get_frame_context().imageAvailableSemaphore;
						view_info.unsyncWriteStage = 0;
						view_info.unsyncWriteAccess = 0;
					}
					else if (external_pass_type == first_pass_type) {
						VkEvent& externalEvent = external_events_[first_pass_type];
						if (externalEvent == VK_NULL_HANDLE) {
							externalEvent = gpu_system_->create_event();
						}
						view_info.pendingEvent = externalEvent;
						view_info.pendingSemaphore = SemaphoreID::Null();
						view_info.unsyncWriteStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
						view_info.unsyncWriteAccess = VK_ACCESS_MEMORY_WRITE_BIT;
					}
					else if (owner != ResourceOwner::NONE) {
						SemaphoreID& external_semaphore_id = external_semaphores_[external_pass_type][first_pass_type];
						if (external_semaphore_id == SemaphoreID::Null()) {
							external_semaphore_id = gpu_system_->create_semaphore();
						}
						view_info.pendingEvent = VK_NULL_HANDLE;
						view_info.pendingSemaphore = external_semaphore_id;
						view_info.unsyncWriteStage = 0;
						view_info.unsyncWriteAccess = 0;
					}
				}
			});

		}

		std::ranges::for_each(texture_infos_, [this](TextureExecInfo& texture_info)
		{
			const Texture& texture = gpu_system_->get_texture(texture_info.textureID);
			std::for_each(texture_info.view, texture_info.view + texture_info.get_view_count(), [layout = texture.layout](TextureViewExecInfo& view_info)
			{
				view_info.layout = layout;
			});
		});
	}

	VkRenderPass RenderGraphExecution::create_render_pass(const uint32 pass_index) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		auto graphic_node = static_cast<const GraphicBaseNode*>(render_graph_->get_pass_nodes()[pass_index]);
		RenderPassKey render_pass_key = {};
		const RGRenderTarget& render_target = graphic_node->get_render_target();

		auto get_render_pass_attachment = [this, pass_index](const auto& attachment) -> Attachment
		{
            const uint32 texture_info_idx = get_texture_info_index(attachment.outNodeID);
			const TextureExecInfo& texture_info = texture_infos_[texture_info_idx];
			const Texture& texture = *gpu_system_->get_texture_ptr(texture_info.textureID);

			Attachment render_pass_attachment = {};
			render_pass_attachment.format = texture.desc.format;
			render_pass_attachment.sampleCount = texture.desc.sampleCount;
			if (texture_info.firstPass.id == pass_index) render_pass_attachment.flags |= ATTACHMENT_FIRST_PASS_BIT;
			if (texture_info.lastPass.id == pass_index) render_pass_attachment.flags |= ATTACHMENT_LAST_PASS_BIT;
			if (attachment.desc.clear) render_pass_attachment.flags |= ATTACHMENT_CLEAR_BIT;
			if (is_external(texture_info)) render_pass_attachment.flags |= ATTACHMENT_EXTERNAL_BIT;
			render_pass_attachment.flags |= ATTACHMENT_ACTIVE_BIT;
			return render_pass_attachment;
		};

		std::ranges::transform(render_target.colorAttachments, render_pass_key.colorAttachments, get_render_pass_attachment);
		std::ranges::transform(render_target.resolveAttachments, render_pass_key.resolveAttachments, get_render_pass_attachment);

		if (render_target.depthStencilAttachment.outNodeID.is_valid()) {
			const DepthStencilAttachment& attachment = render_target.depthStencilAttachment;
			render_pass_key.depthAttachment = get_render_pass_attachment(attachment);
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

		auto graphic_node = static_cast<const GraphicBaseNode*>(render_graph_->get_pass_nodes()[pass_index]);

		VkImageView image_views[2 * MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		uint32 image_view_count = 0;
		const RGRenderTarget& render_target = graphic_node->get_render_target();

		for (const ColorAttachment& attachment : render_target.colorAttachments) {
			const auto texture_id = get_texture_id(attachment.outNodeID);
			image_views[image_view_count++] = gpu_system_->get_texture_view(texture_id, attachment.desc.view).vkHandle;
		}

		for (const ResolveAttachment& attachment : render_target.resolveAttachments)
		{
			const auto texture_id = get_texture_id(attachment.outNodeID);
			image_views[image_view_count++] = gpu_system_->get_texture_view(texture_id, attachment.desc.view).vkHandle;
		}

		/* for (const InputAttachment& attachment : graphic_node->inputAttachments) {
			const Texture* texture = get_texture_ptr(attachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		} */

		if (render_target.depthStencilAttachment.outNodeID.is_valid()) {
			uint32 info_idx = get_texture_info_index(render_target.depthStencilAttachment.outNodeID);
			const TextureExecInfo& texture_info = texture_infos_[info_idx];
			auto depth_attachment_desc = render_target.depthStencilAttachment.desc;

			image_views[image_view_count++] = gpu_system_->get_texture_view(texture_info.textureID, depth_attachment_desc.view).vkHandle;
		}

		VkFramebufferCreateInfo framebuffer_info = {
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

	void RenderGraphExecution::submit_external_sync_primitive() {

		// Sync semaphores
		for(const auto src_pass_type : FlagIter<PassType>()) {
			runtime::ScopeAllocator scopeAllocator("Sync semaphore allocator", runtime::get_temp_allocator());
			Vector<Semaphore*> semaphores(&scopeAllocator);
			semaphores.reserve(to_underlying(PassType::COUNT));

			for (auto semaphore_id : external_semaphores_[src_pass_type])
			{
				if (semaphore_id.is_null()) continue;
				semaphores.add(gpu_system_->get_semaphore_ptr(semaphore_id));
			}

			if (!semaphores.empty()) {
				const QueueType src_queue_type = PASS_TYPE_TO_QUEUE_TYPE_MAP[src_pass_type];
				VkCommandBuffer sync_semaphore_cmd_buffer = command_pools_.requestCommandBuffer(src_queue_type);
				command_queues_[src_queue_type].submit(sync_semaphore_cmd_buffer, semaphores);
			}
		}

		// Sync events
		for(PassType pass_type : FlagIter<PassType>()) {
			if (external_events_[pass_type] != VK_NULL_HANDLE && pass_type != PassType::NONE) {
				QueueType queue_type = PASS_TYPE_TO_QUEUE_TYPE_MAP[pass_type];
				VkCommandBuffer syncEventCmdBuffer = command_pools_.requestCommandBuffer(queue_type);
				vkCmdSetEvent(syncEventCmdBuffer, external_events_[pass_type], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				command_queues_[queue_type].submit(syncEventCmdBuffer);
			}
		}

	}

	void RenderGraphExecution::execute_pass(const uint32 pass_index, VkCommandBuffer command_buffer) {
		SOUL_PROFILE_ZONE();
		PassNode* passNode = render_graph_->get_pass_nodes()[pass_index];
		// Run render pass here
		switch(passNode->get_type()) {
		case PassType::NONE:
			SOUL_PANIC(0, "Pass Type is unknown!");
			break;
		case PassType::TRANSFER: {

			auto copy_node = static_cast<CopyBaseNode*>(passNode);
			RenderCompiler render_compiler(*gpu_system_, command_buffer);
			CopyCommandList command_list(render_compiler);
			RenderGraphRegistry registry(gpu_system_, this);
			copy_node->execute_pass(registry, command_list);

			break;
		}
		case PassType::COMPUTE: {
			auto compute_node = static_cast<ComputeBaseNode*>(passNode);
			RenderCompiler render_compiler(*gpu_system_, command_buffer);
			ComputeCommandList command_list(render_compiler);

            RenderGraphRegistry registry(gpu_system_, this);
            compute_node->execute_pass(registry, command_list);

			break;
		}
		case PassType::GRAPHIC: {

			auto graphic_node = static_cast<GraphicBaseNode*>(passNode);
			SOUL_ASSERT(0, graphic_node != nullptr, "");
			VkRenderPass render_pass = create_render_pass(pass_index);
			VkFramebuffer framebuffer = create_framebuffer(pass_index, render_pass);

			const RGRenderTarget& render_target = graphic_node->get_render_target();
			VkClearValue clear_values[2 * MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
			uint32 clear_count = 0;
			
			for (const ColorAttachment& attachment : render_target.colorAttachments) {
				ClearValue clear_value = attachment.desc.clearValue;
				memcpy(&clear_values[clear_count++], &clear_value, sizeof(VkClearValue));
			}

			for (const ResolveAttachment& attachment : render_target.resolveAttachments)
			{
				ClearValue clearValue = attachment.desc.clearValue;
				memcpy(&clear_values[clear_count++], &clearValue, sizeof(VkClearValue));
			}

			/* for ([[maybe_unused]] InputAttachment attachment : graphic_node->inputAttachments) {
				clear_values[clear_count++] = {};
			} */

			if (render_target.depthStencilAttachment.outNodeID.is_valid()) {
				const DepthStencilAttachmentDesc& desc = render_target.depthStencilAttachment.desc;
				ClearValue clearValue = desc.clearValue;
				clear_values[clear_count++].depthStencil = { clearValue.depthStencil.depth, clearValue.depthStencil.stencil };
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
			
			RenderGraphRegistry registry(gpu_system_, this, render_pass, render_target.sampleCount);
			GraphicCommandList command_list(
				impl::PrimaryCommandBuffer(command_buffer),
				render_pass_begin_info,
				command_pools_,
				*gpu_system_
			);
			graphic_node->execute_pass(registry, command_list);

			gpu_system_->destroy_framebuffer(framebuffer);

			break;
		}
		case PassType::COUNT:
		default: {
			SOUL_NOT_IMPLEMENTED();
			break;
		}
		}
	}

	void RenderGraphExecution::run() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();

		submit_external_sync_primitive();

		Vector<VkEvent> garbage_events;
		Vector<SemaphoreID> garbage_semaphores;

		Vector<VkBufferMemoryBarrier> event_buffer_barriers;
		Vector<VkImageMemoryBarrier> event_image_barriers;

		Vector<VkImageMemoryBarrier> init_layout_barriers;
		Vector<VkImageMemoryBarrier> semaphore_layout_barriers;
		Vector<VkEvent> events;

		auto need_invalidate = [](VkAccessFlags visible_access_matrix[], VkAccessFlags access_flags, 
			VkPipelineStageFlags stage_flags) -> bool {
			bool result = false;
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
			QueueType queue_type = PASS_TYPE_TO_QUEUE_TYPE_MAP[pass_node->get_type()];
			PassExecInfo& pass_info = pass_infos_[i];

			VkCommandBuffer cmd_buffer = command_pools_.requestCommandBuffer(queue_type);

				Vec4f color = { (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, 1.0f };
			const VkDebugUtilsLabelEXT passLabel = {
				VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
				nullptr,                                 // pNext
				pass_node->get_name(),                    // pLabelName
				{color.x, color.y, color.z, color.w}, // color
			};
			vkCmdBeginDebugUtilsLabelEXT(cmd_buffer, &passLabel);
			
			event_buffer_barriers.clear();
			event_image_barriers.clear();
			init_layout_barriers.clear();
			semaphore_layout_barriers.clear();
			events.clear();

			VkPipelineStageFlags event_src_stage_flags = 0;
			VkPipelineStageFlags event_dst_stage_flags = 0;
			VkPipelineStageFlags semaphore_dst_stage_flags = 0;
			VkPipelineStageFlags init_layout_dst_stage_flags = 0;

			for (const BufferBarrier& barrier: pass_info.bufferInvalidates) {
				BufferExecInfo& buffer_info = buffer_infos_[barrier.bufferInfoIdx];

				if (buffer_info.unsyncWriteAccess != 0) {
					for (VkAccessFlags& access_flags : buffer_info.visibleAccessMatrix) {
						access_flags = 0;
					}
				}

				if (buffer_info.pendingSemaphore.is_null() &&
					buffer_info.unsyncWriteAccess == 0 &&
					!need_invalidate(buffer_info.visibleAccessMatrix, barrier.accessFlags, barrier.stageFlags)) {
					continue;
				}

				if (buffer_info.pendingSemaphore.is_valid()) {
					Semaphore& semaphore = *gpu_system_->get_semaphore_ptr(buffer_info.pendingSemaphore);

					if (!semaphore.isPending()) {
						command_queues_[queue_type].wait(gpu_system_->get_semaphore_ptr(buffer_info.pendingSemaphore), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					}

					buffer_info.pendingSemaphore = SemaphoreID::Null();
				} else {

					VkBufferMemoryBarrier mem_barrier = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = buffer_info.unsyncWriteAccess,
						.dstAccessMask = barrier.accessFlags,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = gpu_system_->get_buffer_ptr(buffer_info.bufferID)->vkHandle,
						.offset = 0,
						.size = VK_WHOLE_SIZE
					};

					VkAccessFlags dst_access_flags = barrier.accessFlags;

					event_buffer_barriers.push_back(mem_barrier);
					events.push_back(buffer_info.pendingEvent);
					event_src_stage_flags |= buffer_info.unsyncWriteStage;
					event_dst_stage_flags |= barrier.stageFlags;

					util::for_each_one_bit_pos(barrier.stageFlags, [&buffer_info, dst_access_flags](const uint32 bit) {
						buffer_info.visibleAccessMatrix[bit] |= dst_access_flags;
					});

					buffer_info.pendingEvent = VK_NULL_HANDLE;

				}
			}

			for (const TextureBarrier& barrier: pass_info.textureInvalidates) {
				TextureExecInfo& texture_info = texture_infos_[barrier.textureInfoIdx];
				Texture* texture = gpu_system_->get_texture_ptr(texture_info.textureID);
				TextureViewExecInfo& view_info = *texture_info.get_view(barrier.view);

				bool layout_change = view_info.layout != barrier.layout;

				if (view_info.unsyncWriteAccess != 0 || layout_change) {
					for (VkAccessFlags& accessFlags : view_info.visibleAccessMatrix) {
						accessFlags = 0;
					}
				}

				if (view_info.pendingSemaphore.is_null() &&
					!layout_change &&
					view_info.unsyncWriteAccess == 0 &&
					!need_invalidate(view_info.visibleAccessMatrix, barrier.accessFlags, barrier.stageFlags)) {
					continue;
				}

				VkImageMemoryBarrier mem_barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.oldLayout = view_info.layout,
					.newLayout = barrier.layout,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = texture->vkHandle,
					.subresourceRange = {
						.aspectMask = vkCastFormatToAspectFlags(texture->desc.format),
						.baseMipLevel = barrier.view.get_level(),
						.levelCount = 1,
						.baseArrayLayer = barrier.view.get_layer(),
						.layerCount = 1
					}
				};

				if (view_info.pendingSemaphore != SemaphoreID::Null()) {
					semaphore_dst_stage_flags |= barrier.stageFlags;
					if (layout_change) {

						mem_barrier.srcAccessMask = 0;
						mem_barrier.dstAccessMask = barrier.accessFlags;
						semaphore_layout_barriers.push_back(mem_barrier);

					}
					Semaphore& semaphore = *gpu_system_->get_semaphore_ptr(view_info.pendingSemaphore);
					if (!semaphore.isPending()) {
						command_queues_[queue_type].wait(gpu_system_->get_semaphore_ptr(view_info.pendingSemaphore), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					}
					view_info.pendingSemaphore = SemaphoreID::Null();
				} else if (view_info.pendingEvent != VK_NULL_HANDLE){
					VkAccessFlags dstAccessFlags = barrier.accessFlags;

					mem_barrier.srcAccessMask = view_info.unsyncWriteAccess;
					mem_barrier.dstAccessMask = dstAccessFlags;

					event_image_barriers.push_back(mem_barrier);
					events.push_back(view_info.pendingEvent);
					event_src_stage_flags |= view_info.unsyncWriteStage;
					event_dst_stage_flags |= barrier.stageFlags;

					util::for_each_one_bit_pos(barrier.stageFlags, [&view_info, dstAccessFlags](uint32 bit) {
						view_info.visibleAccessMatrix[bit] |= dstAccessFlags;
					});

					view_info.pendingEvent = VK_NULL_HANDLE;

				} else {
					SOUL_ASSERT(0, layout_change, "");
					SOUL_ASSERT(0, view_info.layout == VK_IMAGE_LAYOUT_UNDEFINED, "");

					mem_barrier.srcAccessMask = 0;
					mem_barrier.dstAccessMask = barrier.accessFlags;
					init_layout_barriers.push_back(mem_barrier);
					init_layout_dst_stage_flags |= barrier.stageFlags;
				}

				view_info.layout = barrier.layout;
			}

			if (!events.empty()) {
				vkCmdWaitEvents(cmd_buffer,
				                soul::cast<uint32>(events.size()), events.data(),
				                event_src_stage_flags, event_dst_stage_flags,
				                0, nullptr,
				                soul::cast<uint32>(event_buffer_barriers.size()), event_buffer_barriers.data(),
				                soul::cast<uint32>(event_image_barriers.size()), event_image_barriers.data()
				);
			}

			if (!init_layout_barriers.empty()) {
				vkCmdPipelineBarrier(cmd_buffer,
				                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, init_layout_dst_stage_flags,
				                     0,
				                     0, nullptr,
				                     0, nullptr,
				                     soul::cast<uint32>(init_layout_barriers.size()), init_layout_barriers.data());
			}

			if (!semaphore_layout_barriers.empty()) {
				vkCmdPipelineBarrier(cmd_buffer,
				                     semaphore_dst_stage_flags, semaphore_dst_stage_flags,
				                     0,
				                     0, nullptr,
				                     0, nullptr,
				                     soul::cast<uint32>(semaphore_layout_barriers.size()), semaphore_layout_barriers.data());
			}

			execute_pass(i, cmd_buffer);

			FlagMap<PassType, bool> is_pass_type_dependent(false);
			for (const BufferBarrier& barrier : pass_info.bufferFlushes) {
				const BufferExecInfo& buffer_info = buffer_infos_[barrier.bufferInfoIdx];
				if (buffer_info.passCounter != buffer_info.passes.size() - 1) {
					uint32 next_pass_idx = buffer_info.passes[buffer_info.passCounter + 1].id;
					PassType pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					is_pass_type_dependent[pass_type] = true;
				}
			}

			for (const TextureBarrier& barrier: pass_info.textureFlushes) {
				const TextureExecInfo& texture_info = texture_infos_[barrier.textureInfoIdx];
				if (texture_info.firstPass.is_null()) continue;
				const TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
				if (texture_view_info.passCounter != texture_view_info.passes.size() - 1)
				{
					uint32 next_pass_idx = texture_view_info.passes[texture_view_info.passCounter + 1].id;
					PassType pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					is_pass_type_dependent[pass_type] = true;
				}
			}

			FlagMap<PassType, SemaphoreID> semaphoresMap(SemaphoreID::Null());
			VkEvent event = VK_NULL_HANDLE;
			VkPipelineStageFlags eventStageFlags = 0;

			for (PassType passType : FlagIter<PassType>()) {
				if (is_pass_type_dependent[passType] && passType == pass_node->get_type()) {
					event = gpu_system_->create_event();
					garbage_events.push_back(event);
				} else if (is_pass_type_dependent[passType] && passType != pass_node->get_type()) {
					semaphoresMap[passType] = gpu_system_->create_semaphore();
					garbage_semaphores.push_back(semaphoresMap[passType]);
				}
			}

			for (const BufferBarrier& barrier : pass_info.bufferFlushes) {
				BufferExecInfo& bufferInfo = buffer_infos_[barrier.bufferInfoIdx];
				if (bufferInfo.passCounter != bufferInfo.passes.size() - 1) {
					uint32 nextPassIdx = bufferInfo.passes[bufferInfo.passCounter + 1].id;
					PassType nextPassType = render_graph_->get_pass_nodes()[nextPassIdx]->get_type();

					if (pass_node->get_type() != nextPassType) {
						SemaphoreID semaphoreID = semaphoresMap[nextPassType];
						SOUL_ASSERT(0, semaphoreID != SemaphoreID::Null(), "");
						bufferInfo.pendingSemaphore = semaphoreID;
					} else {
						SOUL_ASSERT(0, event != VK_NULL_HANDLE, "");
						bufferInfo.pendingEvent = event;
						eventStageFlags |= barrier.stageFlags;
						
					}
				}
			}

			for (const TextureBarrier& barrier: pass_info.textureFlushes) {
				TextureExecInfo& texture_info = texture_infos_[barrier.textureInfoIdx];
				TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
				if (texture_view_info.passCounter != texture_view_info.passes.size() - 1) {
					uint32 next_pass_idx = texture_view_info.passes[texture_view_info.passCounter + 1].id;
					PassType nextPassType = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					if (pass_node->get_type() != nextPassType) {
						SemaphoreID semaphoreID = semaphoresMap[nextPassType];
						SOUL_ASSERT(0, semaphoreID != SemaphoreID::Null(), "");
						texture_view_info.pendingSemaphore = semaphoreID;
					} else {
						SOUL_ASSERT(0, event != VK_NULL_HANDLE, "");
						texture_view_info.pendingEvent = event;
						eventStageFlags |= barrier.stageFlags;
					}
				}
				texture_view_info.layout = barrier.layout;
			}

			if (event != VK_NULL_HANDLE) {
				vkCmdSetEvent(cmd_buffer, event, eventStageFlags);
			}

			Vector<Semaphore*> semaphores(&passNodeScopeAllocator);
			semaphores.reserve(to_underlying(PassType::COUNT));
			for (SemaphoreID semaphore_id : semaphoresMap) {
				if (semaphore_id.is_valid()) {
					semaphores.add(gpu_system_->get_semaphore_ptr(semaphore_id));
				}
			}

			auto update_resource_unsync_status = [this](const PassType pass_type,
				VkAccessFlags barrier_access_flags,
				VkPipelineStageFlags event_stage_flags, 
				auto& resource_info) {
				bool is_resource_in_last_pass = resource_info.passCounter != resource_info.passes.size() - 1;
				if (is_resource_in_last_pass) {
					const uint32 next_pass_idx = resource_info.passes[resource_info.passCounter].id;
					const PassType next_pass_type = render_graph_->get_pass_nodes()[next_pass_idx]->get_type();
					if (pass_type != next_pass_type) {
						resource_info.unsyncWriteAccess = 0;
						resource_info.unsyncWriteStage = 0;
					}
					else {
						resource_info.unsyncWriteAccess = barrier_access_flags;
						resource_info.unsyncWriteStage = event_stage_flags;
					}
				}
			};

			// Update unsync stage
			for (const BufferBarrier& barrier : pass_info.bufferFlushes) {
				BufferExecInfo& buffer_info = buffer_infos_[barrier.bufferInfoIdx];
				update_resource_unsync_status(pass_node->get_type(), barrier.accessFlags, eventStageFlags, buffer_info);
				buffer_info.passCounter += 1;
			}

			for (const TextureBarrier& barrier: pass_info.textureFlushes) {
				TextureExecInfo& texture_info = texture_infos_[barrier.textureInfoIdx];
				TextureViewExecInfo& texture_view_info = *texture_info.get_view(barrier.view);
				update_resource_unsync_status(pass_node->get_type(), barrier.accessFlags, eventStageFlags, texture_view_info);
				texture_view_info.passCounter += 1;
			}

			vkCmdEndDebugUtilsLabelEXT(cmd_buffer);
			command_queues_[queue_type].submit(cmd_buffer, semaphores);
		}

		// Update resource owner for external resource
		static auto PASS_TYPE_TO_RESOURCE_OWNER = FlagMap<PassType, ResourceOwner>::build_from_list({
			ResourceOwner::NONE,
			ResourceOwner::GRAPHIC_QUEUE,
			ResourceOwner::COMPUTE_QUEUE,
			ResourceOwner::TRANSFER_QUEUE
		});

		for (const TextureExecInfo& texture_info : external_texture_infos_) {
			Texture* texture = gpu_system_->get_texture_ptr(texture_info.textureID);

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
			Buffer* buffer = gpu_system_->get_buffer_ptr(buffer_info.bufferID);
			uint64 last_pass_idx = buffer_info.passes.back().id;
			PassType last_pass_type = render_graph_->get_pass_nodes()[last_pass_idx]->get_type();
			buffer->owner = PASS_TYPE_TO_RESOURCE_OWNER[last_pass_type];
		}

		for (VkEvent event : garbage_events) {
			gpu_system_->destroy_event(event);
		}

		for (SemaphoreID semaphore_id : garbage_semaphores) {
			gpu_system_->destroy_semaphore(semaphore_id);
		}
	}

	bool RenderGraphExecution::is_external(const BufferExecInfo& info) const {
		return soul::cast<uint32>(&info - buffer_infos_.data()) >= render_graph_->get_internal_buffers().size();
	}

	bool RenderGraphExecution::is_external(const TextureExecInfo& info) const {
		return soul::cast<uint32>(&info - texture_infos_.data()) >= render_graph_->get_internal_textures().size();
	}

	BufferID RenderGraphExecution::get_buffer_id(const BufferNodeID node_id) const {
		const uint32 info_idx = get_buffer_info_index(node_id);
		return buffer_infos_[info_idx].bufferID;
	}

	TextureID RenderGraphExecution::get_texture_id(const TextureNodeID node_id) const {
		const uint32 info_idx = get_texture_info_index(node_id);
		return texture_infos_[info_idx].textureID;
	}

	Buffer* RenderGraphExecution::get_buffer(const BufferNodeID node_id) const {
		return gpu_system_->get_buffer_ptr(get_buffer_id(node_id));
	}

	Texture* RenderGraphExecution::get_texture(const TextureNodeID node_id) const {
		return gpu_system_->get_texture_ptr(get_texture_id(node_id));
	}

	uint32 RenderGraphExecution::get_buffer_info_index(const BufferNodeID node_id) const {
		const BufferNode& node = render_graph_->get_buffer_node(node_id);
		if (node.resourceID.is_external()) {
			return soul::cast<uint32>(render_graph_->get_internal_buffers().size()) + node.resourceID.get_index();
		}
		return node.resourceID.get_index();
	}

	uint32 RenderGraphExecution::get_texture_info_index(TextureNodeID nodeID) const {
		const TextureNode& node = render_graph_->get_texture_node(nodeID);
		if (node.resourceID.is_external()) {
			return soul::cast<uint32>(render_graph_->get_internal_textures().size()) + node.resourceID.get_index();
		}
		return node.resourceID.get_index();
	}

	void RenderGraphExecution::cleanup() {

		for (auto event : external_events_) {
			if (event != VK_NULL_HANDLE) gpu_system_->destroy_event(event);
		}

		for (PassType src_pass_type : FlagIter<PassType>()) {
			for (auto semaphore_id : external_semaphores_[src_pass_type]) {
				if (semaphore_id.is_valid()) gpu_system_->destroy_semaphore(semaphore_id);
			}
		}

		for (BufferExecInfo& buffer_info : internal_buffer_infos_) {
			gpu_system_->destroy_buffer(buffer_info.bufferID);
		}

		for (TextureExecInfo& texture_info : internal_texture_infos_) {
			gpu_system_->destroy_texture(texture_info.textureID);
		}

	}

	void RenderGraphExecution::init_shader_buffers(const Vector<ShaderBufferReadAccess>& shaderAccessList, soul_size index, QueueType queue_type) {
		PassExecInfo& passInfo = pass_infos_[index];
		for (const ShaderBufferReadAccess& shader_access : shaderAccessList) {
			SOUL_ASSERT(0, shader_access.nodeID.is_valid(), "");

			uint32 buffer_info_id = get_buffer_info_index(shader_access.nodeID);

			VkPipelineStageFlags stage_flags = vkCastShaderStageToPipelineStageFlags(shader_access.stageFlags);

			BufferBarrier invalidate_barrier;
			invalidate_barrier.stageFlags = stage_flags;
			invalidate_barrier.accessFlags = VK_ACCESS_SHADER_READ_BIT;
			invalidate_barrier.bufferInfoIdx = buffer_info_id;
			passInfo.bufferInvalidates.push_back(invalidate_barrier);

			BufferBarrier flush_barrier;
			flush_barrier.stageFlags = stage_flags;
			flush_barrier.accessFlags = 0;
			flush_barrier.bufferInfoIdx = buffer_info_id;
			passInfo.bufferFlushes.push_back(flush_barrier);

			update_buffer_info(queue_type, impl::get_buffer_usage_flags(shader_access.usage), PassNodeID(index), &buffer_infos_[buffer_info_id]);
		}

	}

	void RenderGraphExecution::init_shader_buffers(const Vector<ShaderBufferWriteAccess>& shaderAccessList, soul_size index, QueueType queue_type) {
		PassExecInfo& passInfo = pass_infos_[index];
		for (const auto& shader_access : shaderAccessList) {
			SOUL_ASSERT(0, shader_access.outputNodeID.is_valid(), "");

            const uint32 buffer_info_id = get_buffer_info_index(shader_access.outputNodeID);

			VkPipelineStageFlags stage_flags = vkCastShaderStageToPipelineStageFlags(shader_access.stageFlags);

			BufferBarrier invalidate_barrier;
			invalidate_barrier.stageFlags = stage_flags;
			invalidate_barrier.accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			invalidate_barrier.bufferInfoIdx = buffer_info_id;
			passInfo.bufferInvalidates.push_back(invalidate_barrier);

			BufferBarrier flush_barrier;
			flush_barrier.stageFlags = stage_flags;
			flush_barrier.accessFlags = VK_ACCESS_SHADER_WRITE_BIT;
			flush_barrier.bufferInfoIdx = buffer_info_id;
			passInfo.bufferFlushes.push_back(flush_barrier);

			update_buffer_info(queue_type, get_buffer_usage_flags(shader_access.usage), PassNodeID(index), &buffer_infos_[buffer_info_id]);
		}

	}

	void RenderGraphExecution::init_shader_textures(const Vector<ShaderTextureReadAccess>& access_list, soul_size index, QueueType queue_type) {
		PassExecInfo& pass_info = pass_infos_[index];
		for (const auto& shader_access : access_list) {
			SOUL_ASSERT(0, shader_access.nodeID.is_valid(), "");

			uint32 texture_info_id = get_texture_info_index(shader_access.nodeID);
			VkPipelineStageFlags stage_flags = vkCastShaderStageToPipelineStageFlags(shader_access.stageFlags);

			update_texture_info(queue_type, get_texture_usage_flags(shader_access.usage),PassNodeID(index), shader_access.viewRange, &texture_infos_[texture_info_id]);

			auto is_writable = [](const ShaderTextureReadUsage usage) -> bool {
				auto mapping = FlagMap<ShaderTextureReadUsage, bool>::build_from_list({
					false,
					true
				});
				return mapping[usage];
			};

			VkImageLayout image_layout = is_writable(shader_access.usage) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::transform(shader_access.viewRange.begin(), shader_access.viewRange.end(), std::back_inserter(pass_info.textureInvalidates),
		[stage_flags, image_layout, texture_info_id](SubresourceIndex view_index) -> TextureBarrier
			{
				return {
					.stageFlags = stage_flags,
					.accessFlags = VK_ACCESS_SHADER_READ_BIT,
					.layout = image_layout,
					.textureInfoIdx = texture_info_id,
					.view = view_index
				};
			});

			std::transform(shader_access.viewRange.begin(), shader_access.viewRange.end(), std::back_inserter(pass_info.textureFlushes),
		[stage_flags, image_layout, texture_info_id](SubresourceIndex view_index) -> TextureBarrier 
			{
				return {
					.stageFlags = stage_flags,
					.accessFlags = 0,
					.layout = image_layout,
					.textureInfoIdx = texture_info_id,
					.view = view_index
				};
			});
		}
	}

	void RenderGraphExecution::init_shader_textures(const Vector<ShaderTextureWriteAccess>& access_list, soul_size index, QueueType queue_type) {
		PassExecInfo& pass_info = pass_infos_[index];
		for (const auto& shader_access : access_list) {
			SOUL_ASSERT(0, shader_access.outputNodeID.is_valid(), "");

			const uint32 texture_info_id = get_texture_info_index(shader_access.outputNodeID);
			const VkPipelineStageFlags stage_flags = vkCastShaderStageToPipelineStageFlags(shader_access.stageFlags);

			update_texture_info(queue_type, get_texture_usage_flags(shader_access.usage), PassNodeID(index), shader_access.viewRange, &texture_infos_[texture_info_id]);

			std::ranges::transform(shader_access.viewRange, std::back_inserter(pass_info.textureInvalidates),
        [stage_flags, texture_info_id](const SubresourceIndex& view_index) -> TextureBarrier
            {
                return {
                    .stageFlags = stage_flags,
                    .accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                    .layout = VK_IMAGE_LAYOUT_GENERAL,
                    .textureInfoIdx = texture_info_id,
                    .view = view_index
                };
            });

			std::ranges::transform(shader_access.viewRange, std::back_inserter(pass_info.textureFlushes),
		[stage_flags, texture_info_id](SubresourceIndex view_index) -> TextureBarrier
            {
                return {
                    .stageFlags = stage_flags,
                    .accessFlags =VK_ACCESS_SHADER_WRITE_BIT,
                    .layout = VK_IMAGE_LAYOUT_GENERAL,
                    .textureInfoIdx = texture_info_id,
                    .view = view_index
                };
            });

		}
	}
	
}
