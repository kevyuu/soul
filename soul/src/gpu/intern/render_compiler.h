#pragma once

#include "gpu/id.h"
#include "gpu/constant.h"

namespace soul::gpu
{
	class System;
	struct RenderCommand;
	struct RenderCommandDraw;
	struct RenderCommandDrawIndex;
	struct RenderCommandUpdateTexture;
	struct RenderCommandCopyTexture;
	struct RenderCommandUpdateBuffer;
	struct RenderCommandCopyBuffer;
	struct RenderCommandDispatch;
}

namespace soul::gpu::impl
{
	class RenderCompiler {
	public:
		constexpr RenderCompiler(System& gpu_system, VkCommandBuffer commandBuffer) :
			gpu_system_(gpu_system), command_buffer(commandBuffer),
			current_pipeline(VK_NULL_HANDLE) {}

		System& gpu_system_;
		VkCommandBuffer command_buffer;
		VkPipeline current_pipeline;

		void compile_command(const RenderCommand& command);
		void compile_command(const RenderCommandDraw& command);
		void compile_command(const RenderCommandDrawIndex& command);
		void compile_command(const RenderCommandUpdateTexture& command);
		void compile_command(const RenderCommandCopyTexture& command);
		void compile_command(const RenderCommandUpdateBuffer& command);
		void compile_command(const RenderCommandCopyBuffer& command);
		void compile_command(const RenderCommandDispatch& command);

	private:
		void apply_pipeline_state(PipelineStateID pipeline_state_id);
		void apply_push_constant(void* push_constant_data, uint32 push_constant_size);

	};
}
