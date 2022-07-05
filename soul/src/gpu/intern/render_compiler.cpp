#include "runtime/scope_allocator.h"

#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_compiler.h"
#include "gpu/system.h"

#include <volk.h>

namespace soul::gpu::impl
{

	void RenderCompiler::compile_command(const RenderCommand& command) {
		SOUL_PROFILE_ZONE();
		#define COMPILE_PACKET(TYPE_STRUCT) \
			case TYPE_STRUCT::TYPE: \
				compile_command(*static_cast<const TYPE_STRUCT*>(&command)); \
				break

		switch (command.type) {
		COMPILE_PACKET(RenderCommandDraw);
		COMPILE_PACKET(RenderCommandDrawPrimitiveBindless);
		COMPILE_PACKET(RenderCommandCopyTexture);
		case RenderCommandType::COUNT:
			SOUL_NOT_IMPLEMENTED();
		}
	}

	void RenderCompiler::compile_command(const RenderCommandDraw& command)
	{
		SOUL_PROFILE_ZONE();
		apply_pipeline_state(command.pipeline_state_id);
		apply_push_constant(command.push_constant_data, command.push_constant_size);

		for (uint32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++) {
			BufferID vert_buf_id = command.vertex_buffer_i_ds[vert_buf_idx];
			if (vert_buf_id.is_null()) {
				continue;
			}
			const Buffer& vertex_buffer = gpu_system_.get_buffer(vert_buf_id);
			SOUL_ASSERT(0, vertex_buffer.desc.usageFlags.test(BufferUsage::VERTEX), "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
		}
		vkCmdDraw(commandBuffer, command.vertex_count, command.instance_count, command.first_vertex, command.first_instance);
	}

	void RenderCompiler::compile_command(const RenderCommandDrawPrimitiveBindless& command) {
		SOUL_PROFILE_ZONE();
		apply_pipeline_state(command.pipeline_state_id);
		apply_push_constant(command.push_constant_data, command.push_constant_size);
		
		for (uint32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++) {
			BufferID vert_buf_id = command.vertex_buffer_ids[vert_buf_idx];
			if (vert_buf_id.is_null()) {
				continue;
			}
			const Buffer& vertex_buffer = gpu_system_.get_buffer(vert_buf_id);
			SOUL_ASSERT(0, vertex_buffer.desc.usageFlags.test(BufferUsage::VERTEX), "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
		}

		const Buffer& index_buffer = gpu_system_.get_buffer(command.index_buffer_id);
		SOUL_ASSERT(0, index_buffer.desc.usageFlags.test(gpu::BufferUsage::INDEX), "");
		vkCmdBindIndexBuffer(commandBuffer, index_buffer.vk_handle, 0, index_buffer.unit_size == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, command.index_count, 1, command.index_offset, command.vertex_offsets[0], 1);
	}

	void RenderCompiler::compile_command(const RenderCommandCopyTexture& command)
	{
		SOUL_PROFILE_ZONE();
		const Texture* src_texture = gpu_system_.get_texture_ptr(command.src_texture);
		const Texture* dst_texture = gpu_system_.get_texture_ptr(command.dst_texture);

		const auto src_aspect_mask = vkCastFormatToAspectFlags(src_texture->desc.format);
		const auto dst_aspect_mask = vkCastFormatToAspectFlags(dst_texture->desc.format);

		runtime::ScopeAllocator scope_allocator("compile_command copy texture");
		Vector<VkImageCopy> image_copies(&scope_allocator);
		image_copies.resize(command.region_count);

		std::transform(command.regions, command.regions + command.region_count, image_copies.begin(),
			[src_aspect_mask, dst_aspect_mask](const TextureCopyRegion& copy_region) -> VkImageCopy
			{
				return {
					.srcSubresource = get_vk_subresource_layers(copy_region.srcSubresource, src_aspect_mask),
					.srcOffset = get_vk_offset_3d(copy_region.srcOffset),
					.dstSubresource = get_vk_subresource_layers(copy_region.dstSubresource, dst_aspect_mask),
					.dstOffset = get_vk_offset_3d(copy_region.dstOffset),
					.extent = get_vk_extent_3d(copy_region.extent)
				};
			});

		vkCmdCopyImage(commandBuffer,
			src_texture->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dst_texture->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			command.region_count,
			image_copies.data());
	}


	void RenderCompiler::apply_pipeline_state(PipelineStateID pipeline_state_id) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT(0, pipeline_state_id != PIPELINE_STATE_ID_NULL, "");
		const PipelineState& pipeline_state = gpu_system_.get_pipeline_state(pipeline_state_id);
		if (pipeline_state.vk_handle != currentPipeline) {
			vkCmdBindPipeline(commandBuffer, pipeline_state.bind_point, pipeline_state.vk_handle);
			currentPipeline = pipeline_state.vk_handle;
		}
	}

	void RenderCompiler::apply_push_constant(void* push_constant_data, uint32 push_constant_size)
	{
		if (push_constant_data == nullptr) return;
		SOUL_PROFILE_ZONE();
		vkCmdPushConstants(commandBuffer, gpu_system_.get_bindless_pipeline_layout(), VK_SHADER_STAGE_ALL, 0, push_constant_size, push_constant_data);
	}

}
