#pragma once

#include "gpu/data.h"

namespace Soul { namespace GPU {
	class System;

	class RenderCompiler {
	public:
		explicit RenderCompiler(System* gpuSystem, VkCommandBuffer commandBuffer);

		System* gpuSystem;
		VkCommandBuffer commandBuffer;
		VkPipeline currentPipeline;
		VkPipelineLayout currentPipelineLayout;
		VkPipelineBindPoint currentBindPoint;
		ShaderArgSetID currentShaderArgumentSets[MAX_SET_PER_SHADER_PROGRAM];

		void compileCommand(const RenderCommand& command);
		void compileCommand(const RenderCommandDrawIndex& command);
		void compileCommand(const RenderCommandDrawVertex& command);
		void compileCommand(const RenderCommandDrawPrimitive& command);

	private:
		void _applyPipelineState(PipelineStateID pipelineStateID);
		void _applyShaderArguments(const ShaderArgSetID* shaderArgumentIDs);
	};
}}
