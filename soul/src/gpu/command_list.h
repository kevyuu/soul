#pragma once

#include "gpu/system.h"

namespace soul::gpu
{
	class GraphicCommandList
	{
	private:
		impl::PrimaryCommandBuffer primary_command_buffer_;
		const VkRenderPassBeginInfo& render_pass_begin_info_;
		impl::CommandPools& command_pools_;
		System& gpu_system_;

		static constexpr uint32 SECONDARY_COMMAND_BUFFER_THRESHOLD = 128;

	public:
		constexpr GraphicCommandList(impl::PrimaryCommandBuffer primary_command_buffer,
			const VkRenderPassBeginInfo& render_pass_begin_info, impl::CommandPools& command_pools, System& gpu_system) :
			primary_command_buffer_(primary_command_buffer), render_pass_begin_info_(render_pass_begin_info), command_pools_(command_pools),
			gpu_system_(gpu_system) {}

		template<
			graphic_render_command RenderCommandType,
			command_generator<RenderCommandType> CommandGenerator>
			void push(const soul_size count, CommandGenerator&& generator)
		{
			if (count > SECONDARY_COMMAND_BUFFER_THRESHOLD)
			{
				primary_command_buffer_.begin_render_pass(render_pass_begin_info_, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
				const uint32 thread_count = runtime::get_thread_count();

				Vector<impl::SecondaryCommandBuffer> secondary_command_buffers;
				secondary_command_buffers.resize(thread_count);

				struct TaskData
				{
					Vector<impl::SecondaryCommandBuffer>& cmdBuffers;
					soul_size commandCount;
					VkRenderPass renderPass;
					VkFramebuffer framebuffer;
					impl::CommandPools& commandPools;
					gpu::System& gpuSystem;
				};
				const TaskData task_data = {
					secondary_command_buffers,
					count,
					render_pass_begin_info_.renderPass,
					render_pass_begin_info_.framebuffer,
					command_pools_,
					gpu_system_
				};

				runtime::TaskID task_id = runtime::parallel_for_task_create(
					runtime::TaskID::ROOT(), thread_count, 1,
					[&task_data, &generator](int index)
					{
						auto&& command_buffers = task_data.cmdBuffers;
						const auto command_count = task_data.commandCount;
						impl::SecondaryCommandBuffer command_buffer = task_data.commandPools.request_secondary_command_buffer(task_data.renderPass, 0, task_data.framebuffer);
						const uint32 div = command_count / command_buffers.size();
						const uint32 mod = command_count % command_buffers.size();

						VkPipelineLayout pipeline_layout = task_data.gpuSystem.get_bindless_pipeline_layout();
						impl::BindlessDescriptorSets bindless_descriptor_sets = task_data.gpuSystem.get_bindless_descriptor_sets();
						vkCmdBindDescriptorSets(command_buffer.get_vk_handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, BINDLESS_SET_COUNT, bindless_descriptor_sets.vkHandles, 0, 0);

						impl::RenderCompiler render_compiler(task_data.gpuSystem, command_buffer.get_vk_handle());
						if (soul::cast<uint32>(index) < mod)
						{
							soul_size start = index * (div + 1);

							for (soul_size i = 0; i < div + 1; i++)
							{
								render_compiler.compile_command(generator(start + i));
							}
						}
						else
						{
							soul_size start = mod * (div + 1) + (index - mod) * div;
							for (soul_size i = 0; i < div; i++)
							{
								render_compiler.compile_command(generator(start + i));
							}
						}
						command_buffer.end();
						command_buffers[index] = command_buffer;

					});
				runtime::run_task(task_id);
				runtime::wait_task(task_id);
				primary_command_buffer_.execute_secondary_command_buffers(soul::cast<uint32>(secondary_command_buffers.size()), secondary_command_buffers.data());
				primary_command_buffer_.end_render_pass();
			}
			else
			{
				primary_command_buffer_.begin_render_pass(render_pass_begin_info_, VK_SUBPASS_CONTENTS_INLINE);
				VkPipelineLayout pipeline_layout = gpu_system_.get_bindless_pipeline_layout();
				impl::BindlessDescriptorSets bindless_descriptor_sets = gpu_system_.get_bindless_descriptor_sets();
				vkCmdBindDescriptorSets(primary_command_buffer_.get_vk_handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, BINDLESS_SET_COUNT, bindless_descriptor_sets.vkHandles, 0, 0);

				impl::RenderCompiler render_compiler(gpu_system_, primary_command_buffer_.get_vk_handle());
				for (soul_size command_idx = 0; command_idx < count; command_idx++)
				{
					render_compiler.compile_command(generator(command_idx));
				}
				primary_command_buffer_.end_render_pass();
			}

		}

		template <graphic_render_command RenderCommandType>
		void push(soul_size count, const RenderCommandType* render_commands)
		{
			push<RenderCommandType>(count, [render_commands](soul_size index)
				{
					return *(render_commands + index);
				});
		}

		template<graphic_render_command RenderCommandType>
		void push(const RenderCommandType& render_command)
		{
			push(1, &render_command);
		}
	};

	class ComputeCommandList
	{
	private:
		impl::RenderCompiler& render_compiler_;
	public:
		explicit ComputeCommandList(impl::RenderCompiler& render_compiler) : render_compiler_(render_compiler) {}
	};

	class CopyCommandList
	{
		impl::RenderCompiler& render_compiler_;
	public:
		explicit CopyCommandList(impl::RenderCompiler& render_compiler) : render_compiler_(render_compiler) {}
		void push(const RenderCommandCopyTexture& command)
		{
			render_compiler_.compile_command(command);
		}
	};
}
