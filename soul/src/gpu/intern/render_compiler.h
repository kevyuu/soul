#pragma once

#include "gpu/id.h"
#include "gpu/constant.h"

namespace soul::gpu
{
	class System;
	struct RenderCommand;
	struct RenderCommandDrawIndex;
	struct RenderCommandDrawPrimitive;
	struct RenderCommandCopyTexture;
}

namespace soul::gpu::impl
{
	class RenderCompiler {
	public:
		constexpr RenderCompiler(System& gpu_system, VkCommandBuffer commandBuffer) :
			gpu_system_(gpu_system), commandBuffer(commandBuffer),
			currentPipeline(VK_NULL_HANDLE), currentPipelineLayout(VK_NULL_HANDLE),
			currentBindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM) {}

		System& gpu_system_;
		VkCommandBuffer commandBuffer;
		VkPipeline currentPipeline;
		VkPipelineLayout currentPipelineLayout;
		VkPipelineBindPoint currentBindPoint;
		ShaderArgSetID currentShaderArgumentSets[MAX_SET_PER_SHADER_PROGRAM];

		void compile_command(const RenderCommand& command);
		void compile_command(const RenderCommandDrawIndex& command);
		void compile_command(const RenderCommandDrawPrimitive& command);
		void compile_command(const RenderCommandCopyTexture& command);

	private:
		void apply_pipeline_state(PipelineStateID pipeline_state_id);
		void apply_shader_arguments(const ShaderArgSetID* shader_arg_set_ids);
	};
}
