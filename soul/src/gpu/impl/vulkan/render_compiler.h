#pragma once

#include "gpu/constant.h"
#include "gpu/id.h"

#include "gpu/impl/vulkan/type.h"

namespace soul::gpu
{

  class System;
  struct RenderCommand;
  struct RenderCommandDraw;
  struct RenderCommandDrawIndex;
  struct RenderCommandDrawIndexedIndirect;
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
    [[nodiscard]]
    static auto New(NotNull<System*> gpu_system, VkCommandBuffer command_buffer)
    {
      return RenderCompiler(gpu_system, command_buffer);
    }

    auto bind_descriptor_sets(VkPipelineBindPoint pipeline_bind_point) -> void;

    auto begin_render_pass(
      const VkRenderPassBeginInfo& render_pass_begin_info, VkSubpassContents subpass_contents)
      -> void;

    auto end_render_pass() -> void;
    auto execute_secondary_command_buffers(
      u32 count, const SecondaryCommandBuffer* secondary_command_buffers) -> void;

    void compile_command(const RenderCommand& command);
    void compile_command(const RenderCommandDraw& command);
    void compile_command(const RenderCommandDrawIndex& command);
    void compile_command(const RenderCommandDrawIndexedIndirect& command);
    void compile_command(const RenderCommandUpdateTexture& command);
    void compile_command(const RenderCommandCopyTexture& command);
    void compile_command(const RenderCommandClearTexture& command);
    void compile_command(const RenderCommandUpdateBuffer& command);
    void compile_command(const RenderCommandCopyBuffer& command);
    void compile_command(const RenderCommandDispatch& command);
    void compile_command(const RenderCommandDispatchIndirect& command);
    void compile_command(const RenderCommandBuildBlas& command);
    void compile_command(const RenderCommandBatchBuildBlas& command);
    void compile_command(const RenderCommandBuildTlas& command);
    void compile_command(const RenderCommandRayTrace& command);

  private:
    auto apply_pipeline_state(PipelineStateID pipeline_state_id) -> void;
    auto apply_pipeline_state(VkPipeline pipeline, VkPipelineBindPoint pipeline_bind_point) -> void;
    auto apply_push_constant(const void* push_constant_data, u32 push_constant_size) -> void;
    void apply_push_constant(Span<const byte*> push_constant);

    NotNull<System*> gpu_system_;
    VkCommandBuffer command_buffer_;
    VkPipeline current_pipeline_ = VK_NULL_HANDLE;

    RenderCompiler(NotNull<System*> gpu_system, VkCommandBuffer command_buffer)
        : gpu_system_(gpu_system), command_buffer_(command_buffer)
    {
    }
  };

} // namespace soul::gpu::impl
