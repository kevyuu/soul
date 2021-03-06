#pragma once

#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"
#include "core/enum_array.h"
#include "core/slice.h"
#include "core/pool_array.h"
#include "core/hash_map.h"
#include "core/architecture.h"

// TODO: Figure out how to do it without single header library
#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>

namespace Soul {
	namespace GPU {

		struct System;
		struct RenderGraph;

		// ID
		struct _Texture;
		using TextureID = ID<_Texture, uint32>;
		static constexpr TextureID TEXTURE_ID_NULL = ID<_Texture, uint32>(0);

		struct _Buffer;
		using BufferID = ID<_Buffer, uint32>;
		static constexpr BufferID BUFFER_ID_NULL = BufferID(0);

		struct _Sampler {};
		using SamplerID = ID<_Sampler, VkSampler>;
		static constexpr SamplerID SAMPLER_ID_NULL = SamplerID(VK_NULL_HANDLE);

		struct _ShaderArgSet;
		using ShaderArgSetID = ID<_ShaderArgSet, uint32>;
		static constexpr ShaderArgSetID  SHADER_ARG_SET_ID_NULL = ShaderArgSetID(0);

		struct _Shader;
		using ShaderID = ID<_Shader, uint16>;
		static constexpr ShaderID SHADER_ID_NULL = ShaderID(0);

		struct _Program;
		using ProgramID = ID<_Program, uint16>;
		static constexpr ProgramID PROGRAM_ID_NULL = ProgramID(0);

		struct _Semaphore;
		using SemaphoreID = ID<_Semaphore, uint32>;
		static constexpr SemaphoreID SEMAPHORE_ID_NULL = SemaphoreID(0);

		static constexpr uint32 MAX_SET_PER_SHADER_PROGRAM = 4;
		static constexpr unsigned int SET_COUNT = 8;
		static constexpr uint32 MAX_BINDING_PER_SET = 4;
		static constexpr uint32 MAX_INPUT_PER_SHADER = 8;
		static constexpr uint32 MAX_COLOR_ATTACHMENT_PER_SHADER = 8;
		static constexpr uint32 MAX_DYNAMIC_BUFFER_PER_SET = 4;
		static constexpr uint32 MAX_SIGNAL_SEMAPHORE = 4;

		enum class ResourceOwner : uint8 {
			NONE,
			GRAPHIC_QUEUE,
			COMPUTE_QUEUE,
			TRANSFER_QUEUE,
			PRESENTATION_ENGINE,
			COUNT
		};

		enum class QueueType : uint8 {
			NONE,
			GRAPHIC,
			COMPUTE,
			TRANSFER,
			COUNT
		};

		using QueueFlagBits = enum {
			QUEUE_GRAPHIC_BIT = 0x1,
			QUEUE_COMPUTE_BIT = 0x2,
			QUEUE_TRANSFER_BIT = 0x4,
			QUEUE_DEFAULT = QUEUE_GRAPHIC_BIT | QUEUE_COMPUTE_BIT | QUEUE_TRANSFER_BIT,
			QUEUE_ENUM_END_BIT
		};
		using QueueFlags = uint8;
		static_assert(QUEUE_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(QueueFlags), "");

		using BufferUsageFlagBits =  enum {
			BUFFER_USAGE_INDEX_BIT = 0x1,
			BUFFER_USAGE_VERTEX_BIT = 0x2,
			BUFFER_USAGE_UNIFORM_BIT = 0x4,
			BUFFER_USAGE_STORAGE_BIT = 0x8,
			BUFFER_USAGE_TRANSFER_SRC_BIT = 0x16,
			BUFFER_USAGE_TRANSFER_DST_BIT = 0x32,
			BUFFER_USAGE_ENUM_END_BIT
		};
		using BufferUsageFlags = uint8;
		static_assert(BUFFER_USAGE_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(BufferUsageFlags), "");

		using TextureUsageFlagBits = enum {
			TEXTURE_USAGE_SAMPLED_BIT = 0x1,
			TEXTURE_USAGE_COLOR_ATTACHMENT_BIT = 0x2,
			TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x4,
			TEXTURE_USAGE_TRANSFER_DST_BIT = 0x8,
			TEXTURE_USAGE_ENUM_END_BIT
		};
		using TextureUsageFlags = uint8;
		static_assert(TEXTURE_USAGE_ENUM_END_BIT - 1 < SOUL_UTYPE_MAX(TextureUsageFlags), "");

		enum class TextureType : uint8 {
			D1,
			D2,
			D3,
			COUNT
		};

		enum class TextureFormat : uint16 {
			
			RGB8,
			DEPTH24,

			RGBA8,
			BGRA8,
			DEPTH24_STENCIL8UI,
			DEPTH32F,

			RGB16,
			RGB16F,
			RGB16UI,
			RGB16I,

			COUNT
		};

		enum class TextureFilter : uint8 {
			NEAREST,
			LINEAR,
			COUNT
		};

		enum class TextureWrap : uint8 {
			REPEAT,
			MIRRORED_REPEAT,
			CLAMP_TO_EDGE,
			CLAMP_TO_BORDER,
			MIRROR_CLAMP_TO_EDGE,
			COUNT
		};

		enum class ShaderStage : uint8 {
			NONE,
			VERTEX,
			GEOMETRY,
			FRAGMENT,
			COMPUTE,
			COUNT
		};

		enum class Topology : uint8 {
			POINT_LIST,
			LINE_LIST,
			LINE_STRIP,
			TRIANGLE_LIST,
			TRIANGLE_STRIP,
			TRIANGLE_FAN,
			COUNT
		};

		enum class PolygonMode : uint8 {
			FILL,
			LINE,
			POINT,
			COUNT
		};

		enum class CullMode : uint8 {
			NONE,
			FRONT,
			BACK,
			FRONT_AND_BACK,
			COUNT
		};

		enum class FrontFace : uint8 {
			CLOCKWISE,
			COUNTER_CLOCKWISE,
			COUNT
		};

		enum class CompareOp : uint8 {
			NEVER,
			LESS,
			EQUAL,
			LESS_OR_EQUAL,
			GREATER,
			NOT_EQUAL,
			GREATER_OR_EQUAL,
			ALWAYS,
			COUNT
		};

		enum class BlendFactor : uint8 {
			ZERO,
			ONE,
			SRC_COLOR,
			ONE_MINUS_SRC_COLOR,
			DST_COLOR,
			ONE_MINUS_DST_COLOR,
			SRC_ALPHA,
			ONE_MINUS_SRC_ALPHA,
			DST_ALPHA,
			ONE_MINUS_DST_ALPHA,
			CONSTANT_COLOR,
			ONE_MINUS_CONSTANT_COLOR,
			CONSTANT_ALPHA,
			ONE_MINUS_CONSTANT_ALPHA,
			SRC_ALPHA_SATURATE,
			SRC1_COLOR,
			ONE_MINUS_SRC1_COLOR,
			SRC1_ALPHA,
			ONE_MINUS_SRC1_ALPHA,
			COUNT
		};

		enum class BlendOp : uint8 {
			ADD,
			SUBTRACT,
			REVERSE_SUBTRACT,
			MIN,
			MAX,
			COUNT
		};

		enum class DescriptorType : uint8 {
			UNIFORM_BUFFER,
			SAMPLED_IMAGE,
			COUNT
		};
		struct DescriptorTypeUtil {
			static bool IsBuffer(DescriptorType type) {
				return type == DescriptorType::UNIFORM_BUFFER;
			}

			static bool IsWriteableBuffer(DescriptorType type) {
				return false;
			}

			static bool IsTexture(DescriptorType type) {
				return type == DescriptorType::SAMPLED_IMAGE;
			}

			static bool IsWriteableTexture(DescriptorType type) {
				return false;
			}
		};

		enum class TextureLayout : uint8 {
			DONT_CARE,
			UNDEFINED,
			GENERAL,
			COLOR_ATTACHMENT_OPTIMAL,
			DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			SHADER_READ_ONLY_OPTIMAL,
			TRANSFER_SRC_OPTIMAL,
			TRANSFER_DST_OPTIMAL,
			PRESENT_SRC,
			COUNT
		};



		struct ClearValue {
			Vec4f color;
			Vec2f depthStencil;
		};

		struct UniformDescriptor {
			BufferID bufferID;
			uint32 unitIndex;
		};

		struct SampledImageDescriptor {
			TextureID textureID;
			SamplerID samplerID;
		};

		struct Descriptor {
			DescriptorType type;
			union {
				UniformDescriptor uniformInfo;
				SampledImageDescriptor sampledImageInfo;
			};
		};

		struct ShaderArgSetDesc {
			uint32 bindingCount;
			Descriptor* bindingDescriptions;
		};

		struct BufferDesc {
			uint16 count = 0;
			uint16 typeSize = 0;
			uint16 typeAlignment = 0;
			BufferUsageFlags usageFlags = 0;
			QueueFlags  queueFlags = QUEUE_DEFAULT;
		};

		struct TextureDesc {
			TextureType type;
			TextureFormat format;
			uint16 width, height, depth;
			uint16 mipLevels;
			TextureUsageFlags usageFlags;
			QueueFlags queueFlags;
		};

		struct SamplerDesc {
			TextureFilter minFilter;
			TextureFilter magFilter;
			TextureFilter mipmapFilter;
			TextureWrap wrapU;
			TextureWrap wrapV;
			TextureWrap wrapW;
			bool anisotropyEnable;
			float maxAnisotropy;
		};

		struct ShaderDesc {
			const char* name = nullptr;
			const char* source = nullptr;
			uint32 sourceSize = 0;
		};

		struct _Database;

		struct _Buffer {
			VkBuffer vkHandle = VK_NULL_HANDLE;
			VmaAllocation allocation;
			uint16 unitCount = 0;
			uint16 unitSize = 0;
			BufferUsageFlags usageFlags = 0;
			QueueFlags queueFlags = 0;
			ResourceOwner owner = ResourceOwner::NONE;
		};

		struct _Texture {
			VkImage vkHandle;
			VkImageView view;
			VmaAllocation allocation;
			VkImageLayout layout;
			VkExtent3D extent;
			VkSharingMode sharingMode;
			TextureFormat format;
			TextureType type;
			ResourceOwner owner;
		};

		struct _ShaderDescriptorBinding {
			DescriptorType type = {};
			uint8 count = 0;
		};

		struct _ShaderInput {
			VkFormat format = VK_FORMAT_UNDEFINED;
			uint32 offset = 0;
		};

		struct _Shader {
			VkShaderModule module = VK_NULL_HANDLE;
			_ShaderDescriptorBinding bindings[MAX_SET_PER_SHADER_PROGRAM][MAX_BINDING_PER_SET];
			_ShaderInput inputs[MAX_INPUT_PER_SHADER];
			uint32 inputStride = 0;
		};

		struct _ProgramDescriptorBinding {
			DescriptorType type;
			uint8 count = 0;
			VkShaderStageFlags shaderStageFlags = 0;
			VkPipelineStageFlags pipelineStageFlags = 0;
		};

		struct _Program {
			VkPipelineLayout pipelineLayout;
			VkDescriptorSetLayout descriptorLayouts[MAX_SET_PER_SHADER_PROGRAM];
			_ProgramDescriptorBinding bindings[MAX_SET_PER_SHADER_PROGRAM][MAX_BINDING_PER_SET];
		};

		struct _QueueData {
			int count = 0;
			uint32 indices[3] = {};
		};

		struct _Semaphore {
			VkSemaphore vkHandle = VK_NULL_HANDLE;
			VkPipelineStageFlags stageFlags = 0;
		};

		struct alignas(SOUL_CACHELINE_SIZE) _ThreadContext {

		};

		struct _FrameContext {

			Array<_ThreadContext> threadContexts;

			EnumArray<QueueType, VkCommandPool> commandPools;
			EnumArray<QueueType, Array<VkCommandBuffer> > commandBuffers;
			EnumArray<QueueType, uint16> usedCommandBuffers;

			VkFence fence = VK_NULL_HANDLE;
			SemaphoreID imageAvailableSemaphore = SEMAPHORE_ID_NULL;
			SemaphoreID renderFinishedSemaphore = SEMAPHORE_ID_NULL;

			uint32 swapchainIndex = 0;

			struct Garbages {
				Array<TextureID> textures;
				Array<BufferID> buffers;
				Array<ShaderID> shaders;
				Array<ProgramID> programs;
				Array<VkRenderPass> renderPasses;
				Array<VkFramebuffer> frameBuffers;
				Array<VkPipeline> pipelines;
				Array<VkEvent> events;
				Array<SemaphoreID> semaphores;
			} garbages;

			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

			Array<_Buffer> stagingBuffers;
			VkCommandBuffer stagingCommandBuffer = VK_NULL_HANDLE;
			bool stagingAvailable = false;
			bool stagingSynced = false;
		};

		struct _Swapchain {
			VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
			VkSurfaceFormatKHR format;
			VkExtent2D extent;
			Array<TextureID> textures;
			Array<VkImage> images;
			Array<VkImageView> imageViews;
			Array<VkFence> fences;
		};

		struct _ShaderArgSet {
			VkDescriptorSet vkHandle;
			uint32 offset[8];
			uint32 offsetCount;

			bool operator==(const _ShaderArgSet& rhs) const noexcept {
				return vkHandle == rhs.vkHandle && offset == rhs.offset;
			}
		};

		struct _Submission {
			Array<VkSemaphore> waitSemaphores;
			Array<VkPipelineStageFlags> waitStages;
			Array<VkCommandBuffer> commands;
		};

		struct _Database {

			VkInstance instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debugMessenger;

			VkDevice device = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkPhysicalDeviceProperties physicalDeviceProperties;
			VkPhysicalDeviceFeatures physicalDeviceFeatures;

			uint32 graphicsQueueFamilyIndex;
			uint32 presentQueueFamilyIndex;
			uint32 computeQueueFamilyIndex;
			uint32 transferQueueFamilyIndex;
			EnumArray<QueueType, uint32> queueFamilyIndices;

			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkQueue presentQueue = VK_NULL_HANDLE;
			VkQueue computeQueue = VK_NULL_HANDLE;
			VkQueue transferQueue = VK_NULL_HANDLE;

			EnumArray<QueueType, VkQueue> queues;

			VkSurfaceKHR surface;
			VkSurfaceCapabilitiesKHR surfaceCaps;

			_Swapchain swapchain;

			Array<_FrameContext> frameContexts;
			uint32 currentFrame;

			VmaAllocator allocator;

			PoolArray<_Buffer> buffers;
			PoolArray<_Texture> textures;
			PoolArray<_Shader> shaders;
			PoolArray<_Program> programs;
			PoolArray<_Semaphore> semaphores;

			HashMap<VkSampler> samplerMap;

			HashMap<VkDescriptorSet> descriptorSets;
			Array<_ShaderArgSet> shaderArgSets;

			EnumArray<QueueType, _Submission> submissions;

		};

	}
}


