//
// Created by Kevin Yudi Utama on 26/12/19.
//

#pragma once
#include "gpu/data.h"
#include "memory/allocator.h"

namespace Soul::GPU::impl
{

	struct BufferBarrier {
		VkPipelineStageFlags stageFlags = 0;
		VkAccessFlags accessFlags = 0;
		uint16 bufferInfoIdx = 0;
	};

	struct TextureBarrier {
		VkPipelineStageFlags stageFlags = 0;
		VkAccessFlags accessFlags = 0;

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint16 textureInfoIdx = 0;
	};

	struct BufferExecInfo {
		PassNodeID firstPass = PASS_NODE_ID_NULL;
		PassNodeID lastPass = PASS_NODE_ID_NULL;
		BufferUsageFlags usageFlags = 0u;
		QueueFlags queueFlags = 0;
		BufferID bufferID;

		VkEvent pendingEvent = VK_NULL_HANDLE;
		SemaphoreID pendingSemaphore = SEMAPHORE_ID_NULL;
		VkPipelineStageFlags unsyncWriteStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsyncWriteAccess = 0;

		VkAccessFlags visibleAccessMatrix[32] = {};

		Array<PassNodeID> passes;
		uint32 passCounter = 0;
	};

	struct TextureExecInfo {
		PassNodeID firstPass = PASS_NODE_ID_NULL;
		PassNodeID lastPass = PASS_NODE_ID_NULL;
		TextureUsageFlags usageFlags = 0u;
		QueueFlags queueFlags = 0;
		TextureID textureID;

		VkEvent pendingEvent = VK_NULL_HANDLE;
		SemaphoreID pendingSemaphore = SEMAPHORE_ID_NULL;
		VkPipelineStageFlags unsyncWriteStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsyncWriteAccess = 0;

		VkAccessFlags visibleAccessMatrix[32] = {};

		Array<PassNodeID> passes;
		uint32 passCounter = 0;

	};

	struct PassExecInfo {
		Array<BufferBarrier> bufferFlushes;
		Array<BufferBarrier> bufferInvalidates;
		Array<TextureBarrier> textureFlushes;
		Array<TextureBarrier> textureInvalidates;

	};

	class RenderGraphExecution {
	public:
		RenderGraphExecution(const RenderGraph* renderGraph, System* system, Memory::Allocator* allocator, 
			CommandQueues& commandQueues, CommandPools& commandPools):
			_renderGraph(renderGraph), _gpuSystem(system),
			bufferInfos(allocator), textureInfos(allocator), passInfos(allocator), commandQueues(commandQueues), commandPools(commandPools)
		{}

		RenderGraphExecution(const RenderGraphExecution& other) = delete;
		RenderGraphExecution& operator=(const RenderGraphExecution& other) = delete;

		RenderGraphExecution(RenderGraphExecution&& other) = delete;
		RenderGraphExecution& operator=(RenderGraphExecution&& other) = delete;

		~RenderGraphExecution() = default;

		void init();
		void run();
		void cleanup();

		Array<BufferExecInfo> bufferInfos;
		Slice<BufferExecInfo> internalBufferInfos;
		Slice<BufferExecInfo> externalBufferInfos;

		Array<TextureExecInfo> textureInfos;
		Slice<TextureExecInfo> internalTextureInfos;
		Slice<TextureExecInfo> externalTextureInfos;

		Array<PassExecInfo> passInfos;

		explicit RenderGraphExecution() = default;

		bool isExternal(const BufferExecInfo& info) const;
		bool isExternal(const TextureExecInfo& info) const;
		BufferID getBufferID(BufferNodeID nodeID) const;
		TextureID getTextureID(TextureNodeID nodeID) const;
		Buffer* getBuffer(BufferNodeID nodeID) const;
		Texture* getTexture(TextureNodeID nodeID) const;
		uint32 getBufferInfoIndex(BufferNodeID nodeID) const;
		uint32 getTextureInfoIndex(TextureNodeID nodeID) const;

	private:
		const RenderGraph* _renderGraph;
		System* _gpuSystem;

		EnumArray<PassType, VkEvent> _externalEvents;
		EnumArray<PassType, EnumArray<PassType, SemaphoreID>> _externalSemaphores;
		CommandQueues& commandQueues;
		CommandPools& commandPools;

		VkRenderPass _renderPassCreate(uint32 passIndex);
		VkFramebuffer _framebufferCreate(uint32 passIndex, VkRenderPass renderPass);
		void _submitExternalSyncPrimitive();
		void _executePass(uint32 passIndex, VkCommandBuffer commandBuffer);

		void _initInShaderBuffers(const Array<ShaderBufferReadAccess>& accessList, int index, QueueFlagBits queueFlags);
		void _initOutShaderBuffers(const Array<ShaderBufferWriteAccess>& accessList, int index, QueueFlagBits queueFlags);
		void _initShaderTextures(const Array<ShaderTextureReadAccess>& shaderAccessList, int index, QueueFlagBits queueFlags);
		void _initShaderTextures(const Array<ShaderTextureWriteAccess>& shaderAccessList, int index, QueueFlagBits queueFlags);
	};
}
