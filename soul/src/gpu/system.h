#pragma once
#include "core/util.h"
#include "memory/memory.h"

#include "gpu/data.h"
#include "gpu/render_graph.h"

#define SOUL_VK_CHECK(result, message, ...) SOUL_ASSERT(0, result == VK_SUCCESS, "result = %d | " message, result, ##__VA_ARGS__)

namespace Soul { namespace GPU {

	struct GraphicBaseNode;
	struct RenderGraph;

	struct System {

		explicit System(Memory::Allocator* allocator) : _db(allocator) {}

		struct Config {
			void* windowHandle = nullptr;
			uint32 swapchainWidth = 0;
			uint32 swapchainHeight = 0;
			uint16 maxFrameInFlight = 0;
			uint16 threadCount = 0;
		};

		void init(const Config& config);
		void _frameContextInit(const System::Config& config);

		void shutdown();

		BufferID bufferCreate(const BufferDesc& desc);

		template <SOUL_TEMPLATE_ARG_LAMBDA(DataGenFunc, void(int, byte*))>
		BufferID bufferCreate(const BufferDesc& desc, DataGenFunc dataGenFunc) {
			SOUL_PROFILE_ZONE();
			BufferDesc desc2 = desc;
			desc2.usageFlags |= BUFFER_USAGE_TRANSFER_DST_BIT;
			desc2.queueFlags |= QUEUE_TRANSFER_BIT;
			BufferID bufferID = bufferCreate(desc2);

			_Buffer &buffer = *_bufferPtr(bufferID);

			_Buffer stagingBuffer = {};
			size_t size = buffer.unitCount * buffer.unitSize;
			VkBufferCreateInfo stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
			stagingBufferInfo.size = size;
			stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VmaAllocationCreateInfo stagingAllocInfo = {};
			stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
			SOUL_VK_CHECK(vmaCreateBuffer(_db.gpuAllocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.vkHandle,
										  &stagingBuffer.allocation, nullptr), "");

			_frameContext().stagingBuffers.add(stagingBuffer);

			void *mappedData;
			vmaMapMemory(_db.gpuAllocator, stagingBuffer.allocation, &mappedData);
			for (int i = 0; i < buffer.unitCount; i++) {
				dataGenFunc(i, (byte *) mappedData + i * buffer.unitSize);
			}
			vmaUnmapMemory(_db.gpuAllocator, stagingBuffer.allocation);

			_transferBufferToBuffer(stagingBuffer.vkHandle, buffer.vkHandle, size);

			buffer.owner = ResourceOwner::TRANSFER_QUEUE;

			return bufferID;
		}

		void bufferDestroy(BufferID bufferID);
		_Buffer* _bufferPtr(BufferID bufferID);

		TextureID textureCreate(const TextureDesc& desc);
		TextureID textureCreate(const TextureDesc& desc, const byte* data, uint32 dataSize);
		void textureDestroy(TextureID textureID);
		_Texture* _texturePtr(TextureID textureID);

		ShaderID shaderCreate(const ShaderDesc& desc, ShaderStage stage);
		void shaderDestroy(ShaderID shaderID);
		_Shader* _shaderPtr(ShaderID shaderID);

		ProgramID programRequest(const GraphicBaseNode& node);
		_Program* _programPtr(ProgramID programID);

		VkPipeline _pipelineCreate(const GraphicBaseNode& node, ProgramID programID, VkRenderPass renderPass);
		void _pipelineDestroy(VkPipeline pipeline);

		SamplerID samplerRequest(const SamplerDesc& desc);

		ShaderArgSetID _shaderArgSetRequest(const ShaderArgSetDesc& desc, ProgramID programID, uint32 set);

		SemaphoreID _semaphoreCreate();
		void _semaphoreReset(SemaphoreID ID);
		void _semaphoreDestroy(SemaphoreID id);
		_Semaphore* _semaphorePtr(SemaphoreID id);

		VkEvent _eventCreate();
		void _eventDestroy(VkEvent event);

		void renderGraphExecute(const RenderGraph& renderGraph);

		void _queueFlush(QueueType queueType,
						 uint32 semaphoreCount, SemaphoreID* semaphoreID,
						 VkFence fence);
		void _queueWaitSemaphore(QueueType queueType, SemaphoreID ID, VkPipelineStageFlags waitStages);
		void _queueSubmitCommandBuffer(QueueType queueType, VkCommandBuffer commandBuffer,
									   uint32 semaphoreCount, SemaphoreID *semaphoreID,
									   VkFence = VK_NULL_HANDLE);
		VkCommandBuffer _queueRequestCommandBuffer(QueueType queueType);

		void frameFlush();
		void _frameBegin();
		void _frameEnd();

		Vec2ui32 getSwapchainExtent();
		TextureID getSwapchainTexture();

		void _stagingFrameBegin();
		void _stagingFrameEnd();
		void _stagingSetup();
		void _stagingFlush();
		void _stagingTransferToBuffer(const byte *data, uint32 size, BufferID bufferID);
		void _stagingTransferToTexture(uint32 size, const byte *data, const TextureDesc& desc, TextureID textureID);
		void _transferBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size);


		void _surfaceCreate(void* windowHandle, VkSurfaceKHR* surface);

		_Buffer _stagingBufferRequest(const byte *data, uint32 size);

		_FrameContext& _frameContext();
		_ThreadContext& _threadContext();

		VkRenderPass _renderPassCreate(const VkRenderPassCreateInfo& info);
		void _renderPassDestroy(VkRenderPass renderPass);

		VkFramebuffer _framebufferCreate(const VkFramebufferCreateInfo& info);
		void _framebufferDestroy(VkFramebuffer framebuffer);

		_QueueData _getQueueDataFromQueueFlags(QueueFlags flags);

		_Database _db;
	};

}}