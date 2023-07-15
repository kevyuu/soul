#pragma once

#include "gpu/system.h"

namespace soul::gpu
{

  template <PipelineFlags pipeline_flags>
  class CommandList
  {
  public:
    constexpr CommandList(
      impl::RenderCompiler& render_compiler,
      const VkRenderPassBeginInfo* render_pass_begin_info,
      impl::CommandPools& command_pools,
      System& gpu_system)
        : render_compiler_(render_compiler),
          render_pass_begin_info_(render_pass_begin_info),
          command_pools_(command_pools),
          gpu_system_(gpu_system)
    {
    }

    template <typename RenderCommandType, command_generator<RenderCommandType> CommandGenerator>
      requires(pipeline_flags.test(PipelineType::RASTER))
    auto push(const usize count, CommandGenerator&& generator) -> void
    {
      static_assert(pipeline_flags.test(PipelineType::RASTER));
      static_assert(render_command<RenderCommandType, pipeline_flags>);
      if (count > SECONDARY_COMMAND_BUFFER_THRESHOLD) {
        render_compiler_.begin_render_pass(
          *render_pass_begin_info_, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        const auto thread_count = runtime::get_thread_count();

        Vector<impl::SecondaryCommandBuffer> secondary_command_buffers;
        secondary_command_buffers.resize(thread_count);

        struct TaskData {
          Vector<impl::SecondaryCommandBuffer>& command_buffers;
          usize command_count;
          VkRenderPass render_pass;
          VkFramebuffer framebuffer;
          impl::CommandPools& command_pools;
          System& gpu_system;
        };
        const TaskData task_data = {
          secondary_command_buffers,
          count,
          render_pass_begin_info_->renderPass,
          render_pass_begin_info_->framebuffer,
          command_pools_,
          gpu_system_};

        const auto task_id = runtime::parallel_for_task_create(
          runtime::TaskID::ROOT(), thread_count, 1, [&task_data, &generator](int index) {
            auto&& command_buffers = task_data.command_buffers;
            const auto command_count = task_data.command_count;
            impl::SecondaryCommandBuffer command_buffer =
              task_data.command_pools.request_secondary_command_buffer(
                task_data.render_pass, 0, task_data.framebuffer);
            const ui32 div = command_count / command_buffers.size();
            const ui32 mod = command_count % command_buffers.size();

            impl::RenderCompiler render_compiler(
              task_data.gpu_system, command_buffer.get_vk_handle());
            render_compiler.bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS);
            if (soul::cast<ui32>(index) < mod) {
              const usize start = index * (div + 1);

              for (usize i = 0; i < div + 1; i++) {
                render_compiler.compile_command(generator(start + i));
              }
            } else {
              const usize start = mod * (div + 1) + (index - mod) * div;
              for (usize i = 0; i < div; i++) {
                render_compiler.compile_command(generator(start + i));
              }
            }
            command_buffer.end();
            command_buffers[index] = command_buffer;
          });
        runtime::run_task(task_id);
        runtime::wait_task(task_id);
        render_compiler_.execute_secondary_command_buffers(
          soul::cast<ui32>(secondary_command_buffers.size()), secondary_command_buffers.data());
        render_compiler_.end_render_pass();
      } else {
        render_compiler_.begin_render_pass(*render_pass_begin_info_, VK_SUBPASS_CONTENTS_INLINE);
        for (usize command_idx = 0; command_idx < count; command_idx++) {
          render_compiler_.compile_command(generator(command_idx));
        }
        render_compiler_.end_render_pass();
      }
    }

    template <typename RenderCommandType>
    auto push(usize count, const RenderCommandType* render_commands) -> void
    {
      static_assert(pipeline_flags.test(PipelineType::RASTER));
      static_assert(render_command<RenderCommandType, pipeline_flags>);
      push<RenderCommandType>(
        count, [render_commands](usize index) { return *(render_commands + index); });
    }

    template <typename RenderCommandType>
    auto push(const RenderCommandType& command) -> void
    {
      static_assert(render_command<RenderCommandType, pipeline_flags>);
      if constexpr (RenderCommandType::PIPELINE_TYPE == PipelineType::RASTER) {
        push(1, &command);
      } else {
        render_compiler_.compile_command(command);
      }
    }

  private:
    impl::RenderCompiler& render_compiler_;
    impl::PrimaryCommandBuffer primary_command_buffer_;
    const VkRenderPassBeginInfo* render_pass_begin_info_;
    impl::CommandPools& command_pools_;
    System& gpu_system_;

    static constexpr ui32 SECONDARY_COMMAND_BUFFER_THRESHOLD = 128;
  };

} // namespace soul::gpu
