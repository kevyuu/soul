#pragma once

#include "core/type_traits.h"
#include "gpu/data.h"
#include "gpu/render_graph.h"

#if defined(SOUL_ASSERT_ENABLE)
#define SOUL_VK_CHECK(result, message, ...) SOUL_ASSERT(0, result == VK_SUCCESS, "result = %d | " message, result, ##__VA_ARGS__)
#else
#define SOUL_VK_CHECK(result, message, ...) result
#endif

namespace Soul::GPU
{

	class GraphicBaseNode;

	class System {
	public:
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
		BufferID bufferCreate(const BufferDesc& desc, void* data) {
			SOUL_PROFILE_ZONE();
			BufferDesc desc2 = desc;
			desc2.usageFlags |= BUFFER_USAGE_TRANSFER_DST_BIT;
			desc2.queueFlags |= QUEUE_TRANSFER_BIT;
			BufferID bufferID = bufferCreate(desc2);

			bufferLoad(bufferID, data);
			return bufferID;
		}

		template<
			typename Func,
			SOUL_REQUIRE(is_lambda_v<Func, void(int, void*)>)
		>
		BufferID bufferCreate(const BufferDesc& desc, Func dataGenFunc) {
			SOUL_PROFILE_ZONE();
			BufferDesc desc2 = desc;
			desc2.usageFlags |= BUFFER_USAGE_TRANSFER_DST_BIT;
			desc2.queueFlags |= QUEUE_TRANSFER_BIT;
			BufferID bufferID = bufferCreate(desc2);

			bufferLoad(bufferID, dataGenFunc);
			return bufferID;
		}

		void bufferDestroy(BufferID bufferID);

		void bufferLoad(BufferID bufferID, void* data) {
			impl::Buffer& buffer = *_bufferPtr(bufferID);

			SOUL_ASSERT(0, buffer.usageFlags & BUFFER_USAGE_TRANSFER_DST_BIT, "");
			impl::Buffer stagingBuffer = {};
			size_t size = buffer.unitCount * buffer.unitSize;
			VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			stagingBufferInfo.size = size;
			stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VmaAllocationCreateInfo stagingAllocInfo = {};
			stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
			SOUL_VK_CHECK(vmaCreateBuffer(_db.gpuAllocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.vkHandle,
				              &stagingBuffer.allocation, nullptr), "");

			_frameContext().stagingBuffers.add(stagingBuffer);

			void* mappedData;
			vmaMapMemory(_db.gpuAllocator, stagingBuffer.allocation, &mappedData);
			memcpy(mappedData, data, size);
			vmaUnmapMemory(_db.gpuAllocator, stagingBuffer.allocation);

			_transferBufferToBuffer(stagingBuffer.vkHandle, buffer.vkHandle, size);

			buffer.owner = ResourceOwner::TRANSFER_QUEUE;
		}

		template<
			typename Func,
			SOUL_REQUIRE(is_lambda_v<Func, void(int, void*)>)
		>
		void bufferLoad(BufferID bufferID, Func dataGenFunc) {
			impl::Buffer& buffer = *_bufferPtr(bufferID);

			SOUL_ASSERT(0, buffer.usageFlags & BUFFER_USAGE_TRANSFER_DST_BIT, "");
			impl::Buffer stagingBuffer = {};
			size_t size = buffer.unitCount * buffer.unitSize;
			VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			stagingBufferInfo.size = size;
			stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VmaAllocationCreateInfo stagingAllocInfo = {};
			stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
			SOUL_VK_CHECK(vmaCreateBuffer(_db.gpuAllocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.vkHandle,
				              &stagingBuffer.allocation, nullptr), "");

			_frameContext().stagingBuffers.add(stagingBuffer);

			void* mappedData;
			vmaMapMemory(_db.gpuAllocator, stagingBuffer.allocation, &mappedData);
			for (int i = 0; i < buffer.unitCount; i++) {
				auto unitData = (void*)((byte*)mappedData + i * buffer.unitSize);
				dataGenFunc(i, unitData);
			}
			vmaUnmapMemory(_db.gpuAllocator, stagingBuffer.allocation);

			_transferBufferToBuffer(stagingBuffer.vkHandle, buffer.vkHandle, size);

			buffer.owner = ResourceOwner::TRANSFER_QUEUE;
		}

		impl::Buffer* _bufferPtr(BufferID bufferID);
		const impl::Buffer& _bufferRef(BufferID bufferID);

		TextureID textureCreate(const TextureDesc& desc);
		TextureID textureCreate(const TextureDesc& desc, const byte* data, uint32 dataSize);
		TextureID textureCreate(const TextureDesc& desc, ClearValue clearValue);
		void _generateMipmaps(TextureID textureID);

		TextureID _textureExportCreate(TextureID srcTextureID);
		void textureDestroy(TextureID textureID);
		impl::Texture* _texturePtr(TextureID textureID);
		const impl::Texture& _textureRef(TextureID textureID);
		VkImageView _textureGetMipView(TextureID textureID, uint32 level);

		ShaderID shaderCreate(const ShaderDesc& desc, ShaderStage stage);
		void shaderDestroy(ShaderID shaderID);
		impl::Shader* _shaderPtr(ShaderID shaderID);

		VkDescriptorSetLayout _descriptorSetLayoutRequest(const impl::DescriptorSetLayoutKey& key);

		ProgramID programRequest(const ProgramDesc& key);
		impl::Program* _programPtr(ProgramID programID);
		const impl::Program& _programRef(ProgramID programID);

		VkPipeline _pipelineCreate(const ComputeBaseNode& node, ProgramID programID);
		VkPipeline _pipelineCreate(const GraphicBaseNode& node, ProgramID programID, VkRenderPass renderPass);
		void _pipelineDestroy(VkPipeline pipeline);

		PipelineStateID _pipelineStateRequest(const PipelineStateDesc& key, VkRenderPass renderPass);
		impl::PipelineState* _pipelineStatePtr(PipelineStateID pipelineStateID);
		const impl::PipelineState& _pipelineStateRef(PipelineStateID pipelineStateID);

		SamplerID samplerRequest(const SamplerDesc& desc);

		ShaderArgSetID _shaderArgSetRequest(const ShaderArgSetDesc& desc);
		const impl::ShaderArgSet& _shaderArgSetRef(ShaderArgSetID argSetID);

		SemaphoreID _semaphoreCreate();
		void _semaphoreReset(SemaphoreID ID);
		void _semaphoreDestroy(SemaphoreID id);
		impl::Semaphore* _semaphorePtr(SemaphoreID id);

		VkEvent _eventCreate();
		void _eventDestroy(VkEvent event);

		void renderGraphExecute(const RenderGraph& renderGraph);

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

		impl::Buffer _stagingBufferRequest(const byte *data, uint32 size);

		impl::_FrameContext& _frameContext();

		VkRenderPass _renderPassRequest(const impl::RenderPassKey& key);
		VkRenderPass _renderPassCreate(const VkRenderPassCreateInfo& info);
		void _renderPassDestroy(VkRenderPass renderPass);

		VkFramebuffer _framebufferCreate(const VkFramebufferCreateInfo& info);
		void _framebufferDestroy(VkFramebuffer framebuffer);

		impl::QueueData _getQueueDataFromQueueFlags(QueueFlags flags);

		impl::Database _db;
	};

}
