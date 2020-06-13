//
// Created by Kevin Yudi Utama on 26/12/19.
//

#pragma once
#include "gpu/data.h"
#include "memory/allocator.h"

namespace Soul { namespace GPU {

	struct _BufferBarrier {
		VkPipelineStageFlags stageFlags = 0;
		VkAccessFlags accessFlags = 0;
		uint16 bufferInfoIdx = 0;
	};

	struct _TextureBarrier {
		VkPipelineStageFlags stageFlags = 0;
		VkAccessFlags accessFlags = 0;

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint16 textureInfoIdx = 0;
	};

	struct _RGBufferExecInfo {
		PassNodeID firstPass = PASS_NODE_ID_NULL;
		PassNodeID lastPass = PASS_NODE_ID_NULL;
		BufferUsageFlags usageFlags = 0u;
		QueueFlags queueFlags = 0;
		BufferID bufferID = BUFFER_ID_NULL;

		VkEvent pendingEvent = VK_NULL_HANDLE;
		SemaphoreID pendingSemaphore = SEMAPHORE_ID_NULL;
		VkPipelineStageFlags unsyncWriteStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsyncWriteAccess = 0;

		VkAccessFlags visibleAccessMatrix[32] = {};

		Array<PassNodeID> passes;
		uint32 passCounter = 0;
	};

	struct _RGTextureExecInfo {
		PassNodeID firstPass = PASS_NODE_ID_NULL;
		PassNodeID lastPass = PASS_NODE_ID_NULL;
		VkImageUsageFlags usageFlags = 0u;
		QueueFlags queueFlags = 0;
		TextureID textureID = TEXTURE_ID_NULL;

		VkEvent pendingEvent = VK_NULL_HANDLE;
		SemaphoreID pendingSemaphore = SEMAPHORE_ID_NULL;
		VkPipelineStageFlags unsyncWriteStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkAccessFlags unsyncWriteAccess = 0;

		VkAccessFlags visibleAccessMatrix[32] = {};

		Array<PassNodeID> passes;
		uint32 passCounter = 0;

	};

	struct _RGExecPassInfo {
		ProgramID programID;

		Array<_BufferBarrier> bufferFlushes;
		Array<_BufferBarrier> bufferInvalidates;
		Array<_TextureBarrier> textureFlushes;
		Array<_TextureBarrier> textureInvalidates;

	};

	class _RenderGraphExecution {
	public:
		_RenderGraphExecution(const RenderGraph* renderGraph, System* system, Memory::Allocator* allocator):
				_renderGraph(renderGraph), _gpuSystem(system),
				bufferInfos(allocator), textureInfos(allocator), passInfos(allocator) {}

		_RenderGraphExecution(const _RenderGraphExecution& other) = delete;
		_RenderGraphExecution& operator=(const _RenderGraphExecution& other) = delete;

		_RenderGraphExecution(_RenderGraphExecution&& other) = delete;
		_RenderGraphExecution& operator=(_RenderGraphExecution&& other) = delete;

		~_RenderGraphExecution() = default;

		void init();
		void run();
		void cleanup();

		Array<_RGBufferExecInfo> bufferInfos;
		Slice<_RGBufferExecInfo> internalBufferInfos;
		Slice<_RGBufferExecInfo> externalBufferInfos;

		Array<_RGTextureExecInfo> textureInfos;
		Slice<_RGTextureExecInfo> internalTextureInfos;
		Slice<_RGTextureExecInfo> externalTextureInfos;

		Array<_RGExecPassInfo> passInfos;

		explicit _RenderGraphExecution() = default;

		bool isExternal(const _RGBufferExecInfo& info) const;
		bool isExternal(const _RGTextureExecInfo& info) const;
		BufferID getBufferID(BufferNodeID nodeID) const;
		TextureID getTextureID(TextureNodeID nodeID) const;
		_Buffer* getBuffer(BufferNodeID nodeID) const;
		_Texture* getTexture(TextureNodeID nodeID) const;
		uint32 getBufferInfoIndex(BufferNodeID nodeID) const;
		uint32 getTextureInfoIndex(TextureNodeID nodeID) const;

	private:
		const RenderGraph* _renderGraph;
		System* _gpuSystem;

		EnumArray<PassType, VkEvent> _externalEvents;
		EnumArray<PassType, EnumArray<PassType, SemaphoreID>> _externalSemaphores;

		VkRenderPass _renderPassCreate(uint32 passIndex);
		VkFramebuffer _framebufferCreate(uint32 passIndex, VkRenderPass renderPass);
		void _submitExternalSyncPrimitive();
		void _executePass(uint32 passIndex, VkCommandBuffer commandBuffer);

		void _initInShaderBuffers(const Array<ShaderBuffer>& shaderBuffers, int index, QueueFlagBits queueFlags);
	    void _initOutShaderBuffers(const Array<ShaderBuffer>& shaderBuffers, int index, QueueFlagBits queueFlags);
        void _initInShaderTextures(const Array<ShaderTexture>& shaderTextures, int index, QueueFlagBits queueFlags);
        void _initOutShaderTextures(const Array<ShaderTexture>& shaderTextures, int index, QueueFlagBits queueFlags);
	};
}}
