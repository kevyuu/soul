#pragma once

#include "gpu/id.h"
#include "gpu/constant.h"

namespace soul::gpu
{
	class System;
	struct RenderCommand;
	struct RenderCommandDraw;
	struct RenderCommandDrawPrimitiveBindless;
	struct RenderCommandCopyTexture;
}

namespace soul::gpu::impl
{
	class RenderCompiler {
	public:
		constexpr RenderCompiler(System& gpu_system, VkCommandBuffer commandBuffer) :
			gpu_system_(gpu_system), commandBuffer(commandBuffer),
			currentPipeline(VK_NULL_HANDLE) {}

		System& gpu_system_;
		VkCommandBuffer commandBuffer;
		VkPipeline currentPipeline;

		void compile_command(const RenderCommand& command);
		void compile_command(const RenderCommandDraw& command);
		void compile_command(const RenderCommandDrawPrimitiveBindless& command);
		void compile_command(const RenderCommandCopyTexture& command);

	private:
		void apply_pipeline_state(PipelineStateID pipeline_state_id);
		void apply_push_constant(void* push_constant_data, uint32 push_constant_size);
	};
}
