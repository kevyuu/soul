#include "memory/util.h"

#include "runtime/scope_allocator.h"

#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_compiler.h"
#include "gpu/system.h"

#include <volk.h>

namespace soul::gpu::impl
{
    void RenderCompiler::bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point)
    {
		const auto pipeline_layout = gpu_system_.get_bindless_pipeline_layout();
		const auto bindless_descriptor_sets = gpu_system_.get_bindless_descriptor_sets();
		vkCmdBindDescriptorSets(command_buffer_, pipeline_bind_point, pipeline_layout, 0, BINDLESS_SET_COUNT, bindless_descriptor_sets.vk_handles, 0, nullptr);
    }

    void RenderCompiler::begin_render_pass(const VkRenderPassBeginInfo& render_pass_begin_info,
        const VkSubpassContents subpass_contents)
    {
		vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, subpass_contents);
    }

    void RenderCompiler::end_render_pass()
    {
		vkCmdEndRenderPass(command_buffer_);
    }

	void RenderCompiler::execute_secondary_command_buffers(uint32_t count, const SecondaryCommandBuffer* secondary_command_buffers)
	{
		static_assert(sizeof(SecondaryCommandBuffer) == sizeof(VkCommandBuffer));
		const auto* command_buffers = reinterpret_cast<const VkCommandBuffer*>(secondary_command_buffers);
		vkCmdExecuteCommands(command_buffer_, count, command_buffers);
	}

    void RenderCompiler::compile_command(const RenderCommand& command) {
		SOUL_PROFILE_ZONE();
		#define COMPILE_PACKET(TYPE_STRUCT) \
			case TYPE_STRUCT::TYPE: \
				compile_command(*static_cast<const TYPE_STRUCT*>(&command)); \
				break

		switch (command.type) {
		COMPILE_PACKET(RenderCommandDraw);
		COMPILE_PACKET(RenderCommandDrawIndex);
		COMPILE_PACKET(RenderCommandUpdateTexture);
		COMPILE_PACKET(RenderCommandCopyTexture);
		COMPILE_PACKET(RenderCommandUpdateBuffer);
		COMPILE_PACKET(RenderCommandCopyBuffer);
		COMPILE_PACKET(RenderCommandDispatch);
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
			SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX), "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(command_buffer_, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
		}
		vkCmdDraw(command_buffer_, command.vertex_count, command.instance_count, command.first_vertex, command.first_instance);
	}

	void RenderCompiler::compile_command(const RenderCommandDrawIndex& command) {
		SOUL_PROFILE_ZONE();
		apply_pipeline_state(command.pipeline_state_id);
		apply_push_constant(command.push_constant_data, command.push_constant_size);
		
		for (uint32 vert_buf_idx = 0; vert_buf_idx < MAX_VERTEX_BINDING; vert_buf_idx++) {
			BufferID vert_buf_id = command.vertex_buffer_ids[vert_buf_idx];
			if (vert_buf_id.is_null()) {
				continue;
			}
			const Buffer& vertex_buffer = gpu_system_.get_buffer(vert_buf_id);
			SOUL_ASSERT(0, vertex_buffer.desc.usage_flags.test(BufferUsage::VERTEX), "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(command_buffer_, vert_buf_idx, 1, &vertex_buffer.vk_handle, offsets);
		}

		const Buffer& index_buffer = gpu_system_.get_buffer(command.index_buffer_id);
		SOUL_ASSERT(0, index_buffer.desc.usage_flags.test(gpu::BufferUsage::INDEX), "");

		vkCmdBindIndexBuffer(command_buffer_, index_buffer.vk_handle, command.index_offset, vk_cast(command.index_type));
		vkCmdDrawIndexed(command_buffer_, command.index_count, 1, command.first_index, command.vertex_offsets[0], 1);
	}

	void RenderCompiler::compile_command(const RenderCommandUpdateTexture& command)
    {
		runtime::ScopeAllocator scope_allocator("compile_command::RenderCommandUpdateTexture");
		const Texture& dst_texture = gpu_system_.get_texture(command.dst_texture);

		const auto gpu_allocator = gpu_system_.get_gpu_allocator();
	    const BufferID staging_buffer_id = gpu_system_.create_staging_buffer(command.data_size);
		const Buffer& staging_buffer = gpu_system_.get_buffer(staging_buffer_id);
		void* mapped_data;
		vmaMapMemory(gpu_allocator, staging_buffer.allocation, &mapped_data);
		memcpy(mapped_data, command.data, command.data_size);
		vmaUnmapMemory(gpu_allocator, staging_buffer.allocation);

	    auto get_buffer_image_copy = [format = dst_texture.desc.format](const TextureRegionUpdate& region) -> VkBufferImageCopy
		{

			// SOUL_ASSERT(0, region.extent.z == 1, "3D texture is not supported yet");
			return {
				.bufferOffset = region.buffer_offset,
				.bufferRowLength = region.buffer_row_length,
				.bufferImageHeight = region.buffer_image_height,
				.imageSubresource = {
					.aspectMask = vk_cast_format_to_aspect_flags(format),
					.mipLevel = region.subresource.mip_level,
					.baseArrayLayer = region.subresource.base_array_layer,
					.layerCount = region.subresource.layer_count
				},
				.imageOffset = {
					.x = region.offset.x,
					.y = region.offset.y,
					.z = region.offset.z
				},
				.imageExtent = {
					.width = region.extent.x,
					.height = region.extent.y,
					.depth = region.extent.z
				}
			};
		};

		soul::Vector<VkBufferImageCopy> buffer_image_copies(command.region_count, scope_allocator);
		std::transform(command.regions, command.regions + command.region_count, buffer_image_copies.begin(), get_buffer_image_copy);

		vkCmdCopyBufferToImage(command_buffer_,
			staging_buffer.vk_handle,
			dst_texture.vk_handle,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			soul::cast<uint32>(buffer_image_copies.size()),
			buffer_image_copies.data());
    }

	void RenderCompiler::compile_command(const RenderCommandCopyTexture& command)
	{
		SOUL_PROFILE_ZONE();
		const auto& src_texture = gpu_system_.get_texture(command.src_texture);
		const auto& dst_texture = gpu_system_.get_texture(command.dst_texture);

		const auto src_aspect_mask = vk_cast_format_to_aspect_flags(src_texture.desc.format);
		const auto dst_aspect_mask = vk_cast_format_to_aspect_flags(dst_texture.desc.format);

		runtime::ScopeAllocator scope_allocator("compile_command copy texture");
		Vector<VkImageCopy> image_copies(&scope_allocator);
		image_copies.resize(command.region_count);

		std::transform(command.regions, command.regions + command.region_count, image_copies.begin(),
			[src_aspect_mask, dst_aspect_mask](const TextureRegionCopy& copy_region) -> VkImageCopy
			{
				return {
					.srcSubresource = get_vk_subresource_layers(copy_region.src_subresource, src_aspect_mask),
					.srcOffset = get_vk_offset_3d(copy_region.src_offset),
					.dstSubresource = get_vk_subresource_layers(copy_region.dst_subresource, dst_aspect_mask),
					.dstOffset = get_vk_offset_3d(copy_region.dst_offset),
					.extent = get_vk_extent_3d(copy_region.extent)
				};
			});

		vkCmdCopyImage(command_buffer_,
			src_texture.vk_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dst_texture.vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			command.region_count,
			image_copies.data());
	}

	void RenderCompiler::compile_command(const RenderCommandUpdateBuffer& command)
	{
		const auto gpu_allocator = gpu_system_.get_gpu_allocator();
		const auto& dst_buffer = gpu_system_.get_buffer(command.dst_buffer);
        if (dst_buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
			SOUL_ASSERT(0, dst_buffer.memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "");
			void* mapped_data;
			vmaMapMemory(gpu_allocator, dst_buffer.allocation, &mapped_data);
			for (soul_size region_idx = 0; region_idx < command.region_count; region_idx++)
			{
				const auto& region_load = command.regions[region_idx];
				memcpy(memory::util::pointer_add(mapped_data, region_load.dst_offset), memory::util::pointer_add(command.data, region_load.src_offset), region_load.size);
			}
			vmaUnmapMemory(gpu_allocator, dst_buffer.allocation);
        }
		else
		{
			for (soul_size region_idx = 0; region_idx < command.region_count; region_idx++)
			{
				const auto& region_load = command.regions[region_idx];
				const BufferID staging_buffer_id = gpu_system_.create_staging_buffer(region_load.size);
				const Buffer& staging_buffer = gpu_system_.get_buffer(staging_buffer_id);
				void* mapped_data;
				vmaMapMemory(gpu_allocator, staging_buffer.allocation, &mapped_data);
				memcpy(mapped_data, memory::util::pointer_add(command.data, region_load.src_offset), region_load.size);
				vmaUnmapMemory(gpu_allocator, staging_buffer.allocation);
				const VkBufferCopy copy_region = {
					.srcOffset = 0,
					.dstOffset = region_load.dst_offset,
					.size = region_load.size
				};
				vkCmdCopyBuffer(command_buffer_, staging_buffer.vk_handle, dst_buffer.vk_handle, 1, &copy_region);
			}
		}
	}

	void RenderCompiler::compile_command(const RenderCommandCopyBuffer& command)
	{
		runtime::ScopeAllocator<> scope_allocator("compile_command::RenderCommandCopyBuffer");
		const auto src_buffer = gpu_system_.get_buffer(command.src_buffer);
		const auto dst_buffer = gpu_system_.get_buffer(command.dst_buffer);
		soul::Vector<VkBufferCopy> region_copies(command.region_count, scope_allocator);
		std::transform(command.regions, command.regions + command.region_count, region_copies.begin(),
		[](BufferRegionCopy region_copy) -> VkBufferCopy
		{
			return {
				.srcOffset = region_copy.src_offset,
				.dstOffset = region_copy.dst_offset,
				.size = region_copy.size
			};
		});
		vkCmdCopyBuffer(command_buffer_, src_buffer.vk_handle, dst_buffer.vk_handle, command.region_count, region_copies.data());
	}

	void RenderCompiler::compile_command(const RenderCommandDispatch& command)
	{
		SOUL_PROFILE_ZONE();
		apply_push_constant(command.push_constant_data, command.push_constant_size);
		apply_pipeline_state(command.pipeline_state_id);
		vkCmdDispatch(command_buffer_, command.group_count.x, command.group_count.y, command.group_count.z);
	}

	void RenderCompiler::apply_pipeline_state(PipelineStateID pipeline_state_id) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT(0, pipeline_state_id.is_valid(), "");
		const PipelineState& pipeline_state = gpu_system_.get_pipeline_state(pipeline_state_id);
		if (pipeline_state.vk_handle != current_pipeline_) {
			vkCmdBindPipeline(command_buffer_, pipeline_state.bind_point, pipeline_state.vk_handle);
			current_pipeline_ = pipeline_state.vk_handle;
		}
	}

	void RenderCompiler::apply_push_constant(void* push_constant_data, uint32 push_constant_size)
	{
		if (push_constant_data == nullptr) return;
		SOUL_PROFILE_ZONE();
		vkCmdPushConstants(command_buffer_, gpu_system_.get_bindless_pipeline_layout(), VK_SHADER_STAGE_ALL, 0, push_constant_size, push_constant_data);
	}

}
