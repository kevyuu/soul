#pragma once
#include "gpu/data.h"
#include "gpu/render_graph.h"

#define SOUL_VK_CHECK(result, message, ...) SOUL_ASSERT(0, result == VK_SUCCESS, "result = %d | " message, result, ##__VA_ARGS__)

namespace Soul { namespace GPU {

	struct System {

		struct Config {
			void* windowHandle = nullptr;
			uint32 swapchainWidth = 0;
			uint32 swapchainHeight = 0;
			uint16 maxFrameInFlight = 0;
			uint16 threadCount = 0;
		};

		void init(const Config& config);
		void shutdown();

		BufferID vertexBufferCreate(const VertexBufferDesc& desc);
		BufferID indexBufferCreate(const IndexBufferDesc& desc);
		template<typename T, typename DataGenFunc>
		BufferID uniformBufferArrayCreate(int dataCount, DataGenFunc dataGenFunc);
		void bufferDestroy(BufferID bufferID);

		TextureID samplerTextureCreate(const SamplerTextureDesc& desc);
		void textureDestroy(TextureID textureID);

		void renderGraphExecute(RenderGraph* renderGraph);

		void _surfaceCreate(void* windowHandle, VkSurfaceKHR* surface);
		void _stagingBufferRequest(VmaAllocation allocation, uint32 size, void* data);

		FrameContext& _getCurrentFrameContext();
		ThreadContext& _getCurrentThreadContext();

		BufferID getBuffer(BufferNodeID bufferNodeID);
		TextureID getTexture(TextureNodeID textureNodeID);

		Database _db;
	};

	template<typename T, typename DataGenFunc>
	BufferID System::uniformBufferArrayCreate(int dataCount, DataGenFunc dataGenFunc) {

		SOUL_ASSERT(0, dataCount != 0, "");
		
		uint32 minUboAlignment = _db.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		uint32 dynamicAlignment = sizeof(T);
		if (minUboAlignment > 0) {
			dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		
		Buffer uniformBuffer = {};
		VkBufferCreateInfo bufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = dataCount * dynamicAlignment,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};

		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};

		SOUL_VK_CHECK(vmaCreateBuffer(_db.allocator, &bufferInfo, &allocInfo, &uniformBuffer.vkHandle, &uniformBuffer.allocation, nullptr), "");
		PoolID poolID = _db.buffers.add(uniformBuffer);

		Buffer stagingBuffer = {};
		VkBufferCreateInfo stagingBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = dataCount * dynamicAlignment,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};
		VmaAllocationCreateInfo stagingAllocInfo = {
			.usage = VMA_MEMORY_USAGE_CPU_ONLY
		};
		SOUL_VK_CHECK(vmaCreateBuffer(_db.allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer.vkHandle, &stagingBuffer.allocation, nullptr),"");

		ThreadContext& threadContext = _getCurrentThreadContext();
		threadContext.stagingBuffers.add(stagingBuffer);

		void* mappedData;
		vmaMapMemory(_db.allocator, stagingBuffer.allocation, &mappedData);
		for (int i = 0; i < dataCount; i++) {
			dataGenFunc(i, (T*) ((char*)mappedData + i * dynamicAlignment));
		}
		vmaUnmapMemory(_db.allocator, stagingBuffer.allocation);

		return poolID;
	}

}}