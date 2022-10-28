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
	struct RenderCommandBuildBlas;
	struct RenderCommandBatchBuildBlas;
	struct RenderCommandBuildTlas;
	struct RenderCommandRayTrace;

	namespace impl
	{
		class SecondaryCommandBuffer;
	}
}

namespace soul::gpu::impl
{
	class RenderCompiler {
	public:
		constexpr RenderCompiler(System& gpu_system, VkCommandBuffer command_buffer) :
			gpu_system_(gpu_system), command_buffer_(command_buffer),
			current_pipeline_(VK_NULL_HANDLE) {}

		void bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point);
		void begin_render_pass(const VkRenderPassBeginInfo& render_pass_begin_info, VkSubpassContents subpass_contents);
		void end_render_pass();
		void execute_secondary_command_buffers(uint32 count, const SecondaryCommandBuffer* secondary_command_buffers);

		void compile_command(const RenderCommand& command);
		void compile_command(const RenderCommandDraw& command);
		void compile_command(const RenderCommandDrawIndex& command);
		void compile_command(const RenderCommandUpdateTexture& command);
		void compile_command(const RenderCommandCopyTexture& command);
		void compile_command(const RenderCommandUpdateBuffer& command);
		void compile_command(const RenderCommandCopyBuffer& command);
		void compile_command(const RenderCommandDispatch& command);
		void compile_command(const RenderCommandBuildBlas& command);
		void compile_command(const RenderCommandBatchBuildBlas& command);
		void compile_command(const RenderCommandBuildTlas& command);
		void compile_command(const RenderCommandRayTrace& command);

	private:
		void apply_pipeline_state(PipelineStateID pipeline_state_id);
		void apply_pipeline_state(VkPipeline pipeline, VkPipelineBindPoint pipeline_bind_point);
		void apply_push_constant(void* push_constant_data, uint32 push_constant_size);

		System& gpu_system_;
		VkCommandBuffer command_buffer_;
		VkPipeline current_pipeline_;
	};
}
