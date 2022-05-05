#include "runtime/scope_allocator.h"

#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_compiler.h"
#include "gpu/system.h"

#include <volk/volk.h>

namespace soul::gpu::impl
{

	using namespace impl;

	void RenderCompiler::compile_command(const RenderCommand& command) {
		SOUL_PROFILE_ZONE();
		#define COMPILE_PACKET(TYPE_STRUCT) \
			case TYPE_STRUCT::TYPE: \
				compile_command(*static_cast<const TYPE_STRUCT*>(&command)); \
				break

		switch (command.type) {
		COMPILE_PACKET(RenderCommandDrawIndex);
		COMPILE_PACKET(RenderCommandDrawPrimitive);
		COMPILE_PACKET(RenderCommandCopyTexture);
			break;
		default:
			SOUL_NOT_IMPLEMENTED();
		}
	}

	void RenderCompiler::compile_command(const RenderCommandDrawIndex& command) {
		
		apply_pipeline_state(command.pipelineStateID);
		apply_shader_arguments(command.shaderArgSetIDs);

		const Buffer& vertexBuffer = gpu_system_.get_buffer(command.vertexBufferID);
		SOUL_ASSERT(0, vertexBuffer.desc.usageFlags.test(BufferUsage::VERTEX), "");

		const Buffer& indexBuffer = gpu_system_.get_buffer(command.indexBufferID);
		SOUL_ASSERT(0, indexBuffer.desc.usageFlags.test(BufferUsage::INDEX), "");

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.vkHandle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.vkHandle, 0, indexBuffer.unitSize == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, command.indexCount, 1, command.indexOffset, command.vertexOffset, 0);

	}

	void RenderCompiler::compile_command(const RenderCommandDrawPrimitive& command) {
		apply_pipeline_state(command.pipelineStateID);
		apply_shader_arguments(command.shaderArgSetIDs);

		for (uint32 vertBufIdx = 0; vertBufIdx < MAX_VERTEX_BINDING; vertBufIdx++) {
			BufferID vertBufID = command.vertexBufferIDs[vertBufIdx];
			if (vertBufID.is_null()) {
				continue;
			}
			const Buffer& vertexBuffer = gpu_system_.get_buffer(vertBufID);
			SOUL_ASSERT(0, vertexBuffer.desc.usageFlags.test(BufferUsage::VERTEX), "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, vertBufIdx, 1, &vertexBuffer.vkHandle, offsets);			
		}

		const Buffer& indexBuffer = gpu_system_.get_buffer(command.indexBufferID);
		SOUL_ASSERT(0, indexBuffer.desc.usageFlags.test(gpu::BufferUsage::INDEX), "");
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.vkHandle, 0, indexBuffer.unitSize == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, indexBuffer.desc.count, 1, 0, 0, 1);
	}

	void RenderCompiler::compile_command(const RenderCommandCopyTexture& command)
	{
		const Texture* src_texture = gpu_system_.get_texture_ptr(command.srcTexture);
		const Texture* dst_texture = gpu_system_.get_texture_ptr(command.dstTexture);

		const VkImageAspectFlags src_aspect_mask = vkCastFormatToAspectFlags(src_texture->desc.format);
		const VkImageAspectFlags dst_aspect_mask = vkCastFormatToAspectFlags(dst_texture->desc.format);

		runtime::ScopeAllocator scope_allocator("compile_command copy texture");
		Array<VkImageCopy> image_copies(&scope_allocator);
		image_copies.resize(command.regionCount);

		std::transform(command.regions, command.regions + command.regionCount, image_copies.begin(),
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
			src_texture->vkHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dst_texture->vkHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			command.regionCount,
			image_copies.data());
	}


	void RenderCompiler::apply_pipeline_state(PipelineStateID pipeline_state_id) {
		SOUL_ASSERT(pipeline_state_id != PIPELINE_STATE_ID_NULL, "");
		const PipelineState pipelineState = gpu_system_.get_pipeline_state(pipeline_state_id);
		if (pipelineState.vkHandle != currentPipeline) {
			vkCmdBindPipeline(commandBuffer, pipelineState.bindPoint, pipelineState.vkHandle);
			currentPipeline = pipelineState.vkHandle;
			const Program& program = gpu_system_.get_program(pipelineState.programID);
			currentPipelineLayout = program.pipelineLayout;
			currentBindPoint = pipelineState.bindPoint;
		}
	}

	void RenderCompiler::apply_shader_arguments(const ShaderArgSetID* shader_arg_set_ids) {
		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			if (shader_arg_set_ids[i].is_null()) continue;
			if (shader_arg_set_ids[i] != currentShaderArgumentSets[i]) {
				const ShaderArgSet arg_set = gpu_system_.get_shader_arg_set(shader_arg_set_ids[i]);
				vkCmdBindDescriptorSets(commandBuffer, currentBindPoint, currentPipelineLayout, i, 1, &arg_set.vkHandle, arg_set.offsetCount, arg_set.offset);
				currentShaderArgumentSets[i] = shader_arg_set_ids[i];
			}
		}
	}
}
