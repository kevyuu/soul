#include "gpu/intern/render_compiler.h"
#include "gpu/system.h"
#include <volk/volk.h>

namespace Soul::GPU
{

	using namespace impl;

	RenderCompiler::RenderCompiler(System* gpuSystem, VkCommandBuffer commandBuffer) : 
		gpuSystem(gpuSystem), commandBuffer(commandBuffer), 
		currentPipeline(VK_NULL_HANDLE), currentPipelineLayout(VK_NULL_HANDLE),
		currentBindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM) {}

	void RenderCompiler::compileCommand(const RenderCommand& command) {
#define COMPILE_PACKET(TYPE_STRUCT) \
			case TYPE_STRUCT::TYPE: \
				compileCommand(*static_cast<const TYPE_STRUCT*>(&command)); \
				break

		switch (command.type) {
		COMPILE_PACKET(RenderCommandDrawIndex);
		COMPILE_PACKET(RenderCommandDrawVertex);
		COMPILE_PACKET(RenderCommandDrawPrimitive);
			break;
		default:
			SOUL_NOT_IMPLEMENTED();
		}
	}

	void RenderCompiler::compileCommand(const RenderCommandDrawIndex& command) {
		
		_applyPipelineState(command.pipelineStateID);
		_applyShaderArguments(command.shaderArgSetIDs);

		const Buffer& vertexBuffer = gpuSystem->_bufferRef(command.vertexBufferID);
		SOUL_ASSERT(0, vertexBuffer.usageFlags & BUFFER_USAGE_VERTEX_BIT, "");

		const Buffer& indexBuffer = gpuSystem->_bufferRef(command.indexBufferID);
		SOUL_ASSERT(0, indexBuffer.usageFlags & BUFFER_USAGE_INDEX_BIT, "");

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.vkHandle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.vkHandle, 0, indexBuffer.unitSize == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, command.indexCount, 1, command.indexOffset, command.vertexOffset, 0);

	}

	void RenderCompiler::compileCommand(const RenderCommandDrawVertex& command) {

		_applyPipelineState(command.pipelineStateID);
		_applyShaderArguments(command.shaderArgSetIDs);

		if (!command.vertexBufferID.isNull()) {
			const Buffer& vertexBuffer = gpuSystem->_bufferRef(command.vertexBufferID);
			SOUL_ASSERT(0, vertexBuffer.usageFlags & BUFFER_USAGE_VERTEX_BIT, "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.vkHandle, offsets);
		}
		vkCmdDraw(commandBuffer, command.vertexCount, 1, 0, 0);
	}

	void RenderCompiler::compileCommand(const RenderCommandDrawPrimitive& command) {
		_applyPipelineState(command.pipelineStateID);
		_applyShaderArguments(command.shaderArgSetIDs);

		for (uint32 vertBufIdx = 0; vertBufIdx < MAX_VERTEX_BINDING; vertBufIdx++) {
			BufferID vertBufID = command.vertexBufferIDs[vertBufIdx];
			if (vertBufID.isNull()) {
				continue;
			}
			const Buffer& vertexBuffer = gpuSystem->_bufferRef(vertBufID);
			SOUL_ASSERT(0, vertexBuffer.usageFlags & BUFFER_USAGE_VERTEX_BIT, "");
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, vertBufIdx, 1, &vertexBuffer.vkHandle, offsets);			
		}

		const Buffer& indexBuffer = gpuSystem->_bufferRef(command.indexBufferID);
		SOUL_ASSERT(0, indexBuffer.usageFlags & BUFFER_USAGE_INDEX_BIT, "");
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.vkHandle, 0, indexBuffer.unitSize == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, indexBuffer.unitCount, 1, 0, 0, 1);
	}

	void RenderCompiler::_applyPipelineState(PipelineStateID pipelineStateID) {
		SOUL_ASSERT(pipelineStateID != PIPELINE_STATE_ID_NULL, "");
		const PipelineState& pipelineState = gpuSystem->_pipelineStateRef(pipelineStateID);
		if (pipelineState.vkHandle != currentPipeline) {
			vkCmdBindPipeline(commandBuffer, pipelineState.bindPoint, pipelineState.vkHandle);
			currentPipeline = pipelineState.vkHandle;
			const Program& program = gpuSystem->_programRef(pipelineState.programID);
			currentPipelineLayout = program.pipelineLayout;
			currentBindPoint = pipelineState.bindPoint;
		}
	}

	void RenderCompiler::_applyShaderArguments(const ShaderArgSetID* shaderArgSetIDs) {
		for (int i = 0; i < MAX_SET_PER_SHADER_PROGRAM; i++) {
			if (shaderArgSetIDs[i].isNull()) continue;
			if (shaderArgSetIDs[i] != currentShaderArgumentSets[i]) {
				const ShaderArgSet& argSet = gpuSystem->_shaderArgSetRef(shaderArgSetIDs[i]);
				vkCmdBindDescriptorSets(commandBuffer, currentBindPoint, currentPipelineLayout, i, 1, &argSet.vkHandle, argSet.offsetCount, argSet.offset);
				currentShaderArgumentSets[i] = shaderArgSetIDs[i];
			}
		}
	}
}
