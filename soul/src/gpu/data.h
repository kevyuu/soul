#pragma once

#include "core/type.h"
#include "core/debug.h"
#include "core/array.h"
#include "core/pool_array.h"
#include "core/hash_map.h"
#include "core/architecture.h"

// TODO: Figure out how to do it without single header library
#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>

namespace Soul {
	namespace GPU {

		using TextureID = uint32;
		using BufferID = uint32;
		using SamplerID = uint32;
		using ShaderArgumentID = uint32;

		enum class TextureFormat : uint16 {
			
			RGB8,
			DEPTH24,

			RGBA8,
			DEPTH24_STENCIL8UI,
			DEPTH32F,

			RGB16,
			RGB16F,
			RGB16UI,
			RGB16I,

			COUNT
		};

		struct CommandBuffer {
			VkDevice device;
			VkCommandBuffer vkHandle;
		};

		struct VertexBufferDesc {
			uint32 size; // in byte
			void* data;
		};

		struct IndexBufferDesc {
			uint32 size;
			void* data;
		};

		struct UniformBufferDesc {
			uint32 size;
		};

		struct SamplerTextureDesc {
			TextureFormat format;
			uint16 width, height;
			uint16 mipLevels;
			uint32 dataSize;
			unsigned char* data;
		};

		struct Buffer {
			VkBuffer vkHandle;
			VmaAllocation allocation;
		};

		struct Texture {
			VkImage image;
			VkImageView imageView;
			VmaAllocation allocation;
		};

		struct alignas(SOUL_CACHELINE_SIZE) ThreadContext {
			VkCommandPool graphicsCommandPool;
			VkCommandPool transferCommandPool;
			VkCommandPool computeCommandPool;

			// Command buffer for cpu to gpu data transfer work. The operation on this 
			// command buffer have to finish before the first draw command of the current frame.
			VkCommandBuffer stagingCommandBuffer;

			Array<CommandBuffer> commandBuffers;
			Array<Buffer> stagingBuffers;
			
			Array<VkRenderPass> newRenderPasses;
			Array<VkPipeline> newPipelines;
			Array<VkDescriptorSet> newDescriptorSets;
		};

		struct FrameContext {	
			
			Array<ThreadContext> threadContexts;
			Array<VkSemaphore> recycledSemaphore;

			Buffer uniformRingBuffer;		
		};

		struct Database {

			VkInstance instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debugMessenger;

			VkDevice device = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkPhysicalDeviceProperties physicalDeviceProperties;
			VkPhysicalDeviceFeatures physicalDeviceFeatures;

			uint32_t graphicsQueueFamilyIndex;
			uint32_t presentQueueFamilyIndex;
			uint32_t computeQueueFamilyIndex;
			uint32_t transferQueueFamilyIndex;

			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkQueue presentQueue = VK_NULL_HANDLE;
			VkQueue computeQueue = VK_NULL_HANDLE;
			VkQueue transferQueue = VK_NULL_HANDLE;

			VkSurfaceKHR surface;
			VkSurfaceCapabilitiesKHR surfaceCaps;

			struct Swapchain {
				VkSwapchainKHR vkID = VK_NULL_HANDLE;
				VkSurfaceFormatKHR format;
				VkExtent2D extent;
				Array<VkImage> images;
				Array<VkImageView> imageViews;
			} swapchain;

			Array<FrameContext> frameContexts;
			uint32 currentFrame;

			VmaAllocator allocator;
			
			PoolArray<Buffer> buffers;
			PoolArray<Texture> texturePool;
			PoolArray<VkSemaphore> semaphorePool;

			HashMap<VkRenderPass> renderPassMap;
			HashMap<VkPipeline> pipelineMap;
			HashMap<VkDescriptorSet> descriptorSetMap;
		};

		struct RenderGraphExecution {
			struct BufferInfo {
				uint16 firstPass;
				uint16 lastPass;
				VkBufferUsageFlags usageFlags = 0u;
			};
			Array<BufferInfo> bufferInfos;
			
			struct TextureInfo {
				uint16 firstPass;
				uint16 lastPass;
				VkImageUsageFlags usageFlags = 0u;
			};
			Array<TextureInfo> textureInfos;
		};
	}
}