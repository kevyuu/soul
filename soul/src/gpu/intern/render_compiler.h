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
	struct RasterTargetDesc;

	namespace impl
	{
		class SecondaryCommandBuffer;
	}
}

namespace soul::gpu::impl
{
	class RenderCompiler {
	public:
		constexpr RenderCompiler(System& gpu_system, const VkCommandBuffer command_buffer) :
			gpu_system_(gpu_system), command_buffer_(command_buffer),
			current_pipeline_(VK_NULL_HANDLE) {}

		void bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point);
		void begin_render_pass(const VkRenderPassBeginInfo& render_pass_begin_info, VkSubpassContents subpass_contents);
		void end_render_pass();
		void begin_raster(const RasterTargetDesc& raster_target, bool use_secondary_command_buffer);
		void end_raster();
	    void execute_secondary_command_buffers(uint32 count, const SecondaryCommandBuffer* secondary_command_buffers);
		
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

		System& gpu_system_;
		VkCommandBuffer command_buffer_;
		VkPipeline current_pipeline_;
	};
}
