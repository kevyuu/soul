#include "core/dev_util.h"

#include "gpu/data.h"
#include "gpu/render_graph.h"

#include <volk/volk.h>

namespace Soul { namespace GPU{ namespace Command {

	void DrawVertex::_submit(_Database* db, ProgramID programID, VkCommandBuffer cmdBuffer) {
		_Buffer& buffer = db->buffers[vertexBufferID.id];
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &buffer.vkHandle, offsets);
		vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
	}

	void DrawIndex::_submit(_Database* db, ProgramID programID, VkCommandBuffer cmdBuffer) {
		_Buffer& vertexBuffer = db->buffers[vertexBufferID.id];
		SOUL_ASSERT(0, vertexBuffer.usageFlags & BUFFER_USAGE_VERTEX_BIT, "");

		_Buffer& indexBuffer = db->buffers[indexBufferID.id];
		SOUL_ASSERT(0, indexBuffer.usageFlags & BUFFER_USAGE_INDEX_BIT, "");

		VkDeviceSize offsets[] = {0};
		_Program& program = db->programs[programID.id];
		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			if (shaderArgSets[i] == SHADER_ARG_SET_ID_NULL) continue;
			const _ShaderArgSet& argSet = db->shaderArgSets[shaderArgSets[i].id];
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program.pipelineLayout, i, 1, &argSet.vkHandle, argSet.offsetCount, argSet.offset);
		}
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.vkHandle, offsets);
		// TODO: How to configure index type
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.vkHandle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
	}

	void DrawIndexRegion::_submit(_Database *db, ProgramID programID, VkCommandBuffer cmdBuffer) {

		VkRect2D scissorRect;
		scissorRect.offset = {scissor.offsetX, scissor.offsetY};
		scissorRect.extent = {scissor.width, scissor.height};
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);

		_Buffer& vertexBuffer = db->buffers[vertexBufferID.id];
		SOUL_ASSERT(0, vertexBuffer.usageFlags & BUFFER_USAGE_VERTEX_BIT, "");

		_Buffer& indexBuffer = db->buffers[indexBufferID.id];
		SOUL_ASSERT(0, indexBuffer.usageFlags & BUFFER_USAGE_INDEX_BIT, "");

		VkDeviceSize offsets[] = {0};
		_Program& program = db->programs[programID.id];
		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			if (shaderArgSets[i] == SHADER_ARG_SET_ID_NULL) continue;
			const _ShaderArgSet& argSet = db->shaderArgSets[shaderArgSets[i].id];
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, program.pipelineLayout, i, 1, &argSet.vkHandle, argSet.offsetCount, argSet.offset);
		}
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.vkHandle, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.vkHandle, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(cmdBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
	}

}}}