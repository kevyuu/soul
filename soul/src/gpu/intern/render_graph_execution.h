//
// Created by Kevin Yudi Utama on 26/12/19.
//

#pragma once
#include "gpu/type.h"
#include "memory/allocator.h"

namespace soul::gpu::impl
{

	struct BufferBarrier {
		VkPipelineStageFlags stageFlags = 0;
		VkAccessFlags accessFlags = 0;
		uint32 bufferInfoIdx = 0;
	};

	struct TextureBarrier {
		VkPipelineStageFlags stageFlags = 0;
		VkAccessFlags accessFlags = 0;

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint32 textureInfoIdx = 0;
		SubresourceIndex view;
	};

	struct BufferExecInfo {
		PassNodeID firstPass = PASS_NODE_ID_NULL;
		PassNodeID lastPass = PASS_NODE_ID_NULL;
		BufferUsageFlags usageFlags;
		QueueFlags queueFlags;
		BufferID bufferID;

		VkEvent pendingEvent = VK_NULL_HANDLE;
		SemaphoreID pendingSemaphore = SemaphoreID::Null();
		VkPipelineStageFlags unsyncWriteStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsyncWriteAccess = 0;

		VkAccessFlags visibleAccessMatrix[32] = {};

		Array<PassNodeID> passes;
		uint32 passCounter = 0;
	};

	struct TextureViewExecInfo {
		VkEvent pendingEvent = VK_NULL_HANDLE;
		SemaphoreID pendingSemaphore = SemaphoreID::Null();
		VkPipelineStageFlags unsyncWriteStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsyncWriteAccess = 0;
		VkAccessFlags visibleAccessMatrix[32] = {};
		Array<PassNodeID> passes;
		uint32 passCounter = 0;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct TextureExecInfo {
		PassNodeID firstPass;
		PassNodeID lastPass;
		TextureUsageFlags usageFlags;
		QueueFlags queueFlags;
		TextureID textureID;
		TextureViewExecInfo* view = nullptr;
		uint32 mipLevels = 0;
		uint32 layers = 0;

		[[nodiscard]] soul_size get_view_count() const
		{
			return mipLevels * layers;
		}

		[[nodiscard]] soul_size get_view_index(const SubresourceIndex index) const
		{
			return index.get_layer() * mipLevels + index.get_level();
		}

		[[nodiscard]] TextureViewExecInfo* get_view(const SubresourceIndex index) const
		{
			return view + get_view_index(index);
		}
	};

	class PassAdjacencyList
	{

	private:
		Array<Array<PassNodeID>> dependencies_;
		Array<Array<PassNodeID>> dependents_;
		Array<soul_size> dependency_levels_;
	};

	struct PassExecInfo {
		PassNode* passNode = nullptr;
		Array<BufferBarrier> bufferFlushes;
		Array<BufferBarrier> bufferInvalidates;
		Array<TextureBarrier> textureFlushes;
		Array<TextureBarrier> textureInvalidates;
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

		EnumArray<PassType, VkEvent> external_events_;
		EnumArray<PassType, EnumArray<PassType, SemaphoreID>> external_semaphores_;
		CommandQueues& command_queues_;
		CommandPools& command_pools_;

		Array<BufferExecInfo> buffer_infos_;
		Slice<BufferExecInfo> internal_buffer_infos_;
		Slice<BufferExecInfo> external_buffer_infos_;

		Array<TextureExecInfo> texture_infos_;
		Slice<TextureExecInfo> internal_texture_infos_;
		Slice<TextureExecInfo> external_texture_infos_;
		Array<TextureViewExecInfo> texture_view_infos_;

		Array<PassExecInfo> pass_infos_;
		
		

		[[nodiscard]] VkRenderPass create_render_pass(uint32 pass_index);
		[[nodiscard]] VkFramebuffer create_framebuffer(uint32 pass_index, VkRenderPass render_pass);
		void submit_external_sync_primitive();
		void execute_pass(const uint32 pass_index, VkCommandBuffer command_buffer);

		void init_shader_buffers(const Array<ShaderBufferReadAccess>& access_list, soul_size index, QueueType queue_type);
		void init_shader_buffers(const Array<ShaderBufferWriteAccess>& access_list, soul_size index, QueueType queue_type);
		void init_shader_textures(const Array<ShaderTextureReadAccess>& access_list, soul_size index, QueueType queue_type);
		void init_shader_textures(const Array<ShaderTextureWriteAccess>& access_list, soul_size index, QueueType queue_type);

	};
}
