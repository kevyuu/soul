//
// Created by Kevin Yudi Utama on 26/12/19.
//

#pragma once
#include "gpu/type.h"
#include "memory/allocator.h"

namespace soul::gpu::impl
{

	struct BufferBarrier {
		VkPipelineStageFlags stage_flags = 0;
		VkAccessFlags access_flags = 0;
		uint32 buffer_info_idx = 0;
	};

	struct TextureBarrier {
		VkPipelineStageFlags stage_flags = 0;
		VkAccessFlags access_flags = 0;

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint32 texture_info_idx = 0;
		SubresourceIndex view;
	};

	struct BufferExecInfo {
		PassNodeID first_pass;
		PassNodeID last_pass;
		BufferUsageFlags usage_flags;
		QueueFlags queue_flags;
		BufferID buffer_id;

		VkEvent pending_event = VK_NULL_HANDLE;
		Semaphore pending_semaphore = TimelineSemaphore::null();
		VkPipelineStageFlags unsync_write_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsync_write_access = 0;

		VkAccessFlags visible_access_matrix[32] = {};

		Vector<PassNodeID> passes;
		uint32 pass_counter = 0;
	};

	struct TextureViewExecInfo {
		VkEvent pending_event = VK_NULL_HANDLE;
		Semaphore pending_semaphore = TimelineSemaphore::null();
		VkPipelineStageFlags unsync_write_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsync_write_access = 0;
		VkAccessFlags visible_access_matrix[32] = {};
		Vector<PassNodeID> passes;
		uint32 pass_counter = 0;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct TextureExecInfo {
		PassNodeID first_pass;
		PassNodeID last_pass;
		TextureUsageFlags usage_flags;
		QueueFlags queue_flags;
		TextureID texture_id;
		TextureViewExecInfo* view = nullptr;
		uint32 mip_levels = 0;
		uint32 layers = 0;

		[[nodiscard]] soul_size get_view_count() const
		{
			return mip_levels * layers;
		}

		[[nodiscard]] soul_size get_view_index(const SubresourceIndex index) const
		{
			return index.get_layer() * mip_levels + index.get_level();
		}

		[[nodiscard]] TextureViewExecInfo* get_view(const SubresourceIndex index) const
		{
			return view + get_view_index(index);
		}
	};

	class PassAdjacencyList
	{

	private:
		Vector<Vector<PassNodeID>> dependencies_;
		Vector<Vector<PassNodeID>> dependents_;
		Vector<soul_size> dependency_levels_;
	};

	struct PassExecInfo {
		PassBaseNode* pass_node = nullptr;
		Vector<BufferBarrier> buffer_flushes;
		Vector<BufferBarrier> buffer_invalidates;
		Vector<TextureBarrier> texture_flushes;
		Vector<TextureBarrier> texture_invalidates;
	};

	class RenderGraphExecution {
	public:
		RenderGraphExecution(const RenderGraph* renderGraph, System* system, memory::Allocator* allocator, 
			CommandQueues& commandQueues, CommandPools& commandPools):
			render_graph_(renderGraph), gpu_system_(system),
			buffer_infos_(allocator), texture_infos_(allocator), pass_infos_(allocator), command_queues_(commandQueues), command_pools_(commandPools)
		{}

		RenderGraphExecution() = delete;
		RenderGraphExecution(const RenderGraphExecution& other) = delete;
		RenderGraphExecution& operator=(const RenderGraphExecution& other) = delete;

		RenderGraphExecution(RenderGraphExecution&& other) = delete;
		RenderGraphExecution& operator=(RenderGraphExecution&& other) = delete;

		~RenderGraphExecution() = default;

		void init();
		void run();
		void cleanup();

		[[nodiscard]] bool is_external(const BufferExecInfo& info) const;
		[[nodiscard]] bool is_external(const TextureExecInfo& info) const;
		[[nodiscard]] BufferID get_buffer_id(BufferNodeID node_id) const;
		[[nodiscard]] TextureID get_texture_id(TextureNodeID node_id) const;
		[[nodiscard]] Buffer* get_buffer(BufferNodeID node_id) const;
		[[nodiscard]] Texture* get_texture(TextureNodeID node_id) const;
		[[nodiscard]] uint32 get_buffer_info_index(BufferNodeID node_id) const;
		[[nodiscard]] uint32 get_texture_info_index(TextureNodeID nodeID) const;

	private:
		const RenderGraph* render_graph_;
		System* gpu_system_;

		FlagMap<QueueType, VkEvent> external_events_;
		CommandQueues& command_queues_;
		CommandPools& command_pools_;

		Vector<BufferExecInfo> buffer_infos_;
		Slice<BufferExecInfo> internal_buffer_infos_;
		Slice<BufferExecInfo> external_buffer_infos_;

		Vector<TextureExecInfo> texture_infos_;
		Slice<TextureExecInfo> internal_texture_infos_;
		Slice<TextureExecInfo> external_texture_infos_;
		Vector<TextureViewExecInfo> texture_view_infos_;

		Vector<PassExecInfo> pass_infos_;
		
		

		[[nodiscard]] VkRenderPass create_render_pass(uint32 pass_index);
		[[nodiscard]] VkFramebuffer create_framebuffer(uint32 pass_index, VkRenderPass render_pass);
		void sync_external();
		void execute_pass(uint32 pass_index, PrimaryCommandBuffer command_buffer);

		void init_shader_buffers(std::span<const ShaderBufferReadAccess> access_list, soul_size index, QueueType queue_type);
		void init_shader_buffers(std::span<const ShaderBufferWriteAccess> access_list, soul_size index, QueueType queue_type);
		void init_shader_textures(std::span<const ShaderTextureReadAccess> access_list, soul_size index, QueueType queue_type);
		void init_shader_textures(std::span<const ShaderTextureWriteAccess> access_list, soul_size index, QueueType queue_type);

	};
}
