#pragma once

#include "gpu/constant.h"
#include "gpu/id.h"

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

} // namespace soul::gpu

namespace soul::gpu::impl
{

  class RenderCompiler
  {
  public:
    constexpr RenderCompiler(System& gpu_system, VkCommandBuffer command_buffer)
        : gpu_system_(gpu_system),
          command_buffer_(command_buffer),
          current_pipeline_(VK_NULL_HANDLE)
    {
    }

    auto bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point) -> void;
    auto begin_render_pass(
      const VkRenderPassBeginInfo& render_pass_begin_info, VkSubpassContents subpass_contents)
      -> void;
    auto end_render_pass() -> void;
    auto execute_secondary_command_buffers(
      u32 count, const SecondaryCommandBuffer* secondary_command_buffers) -> void;

    auto compile_command(const RenderCommand& command) -> void;
    auto compile_command(const RenderCommandDraw& command) -> void;
    auto compile_command(const RenderCommandDrawIndex& command) -> void;
    auto compile_command(const RenderCommandUpdateTexture& command) -> void;
    auto compile_command(const RenderCommandCopyTexture& command) -> void;
    auto compile_command(const RenderCommandUpdateBuffer& command) -> void;
    auto compile_command(const RenderCommandCopyBuffer& command) -> void;
    auto compile_command(const RenderCommandDispatch& command) -> void;
    auto compile_command(const RenderCommandBuildBlas& command) -> void;
    auto compile_command(const RenderCommandBatchBuildBlas& command) -> void;
    auto compile_command(const RenderCommandBuildTlas& command) -> void;
    auto compile_command(const RenderCommandRayTrace& command) -> void;

  private:
    auto apply_pipeline_state(PipelineStateID pipeline_state_id) -> void;
    auto apply_pipeline_state(VkPipeline pipeline, VkPipelineBindPoint pipeline_bind_point) -> void;
    auto apply_push_constant(const void* push_constant_data, u32 push_constant_size) -> void;

    System& gpu_system_;
    VkCommandBuffer command_buffer_;
    VkPipeline current_pipeline_;
  };

} // namespace soul::gpu::impl
