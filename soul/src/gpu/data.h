#pragma once

#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"
#include "core/enum_array.h"
#include "core/slice.h"
#include "core/pool.h"
#include "core/uint64_hash_map.h"
#include "core/hash_map.h"
#include "core/architecture.h"

#include "runtime/runtime.h"
#include "memory/allocators/proxy_allocator.h"

// TODO: Figure out how to do it without single header library
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VK_NO_PROTOTYPES
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include <mutex>

namespace Soul {
	namespace GPU {

		struct System;
		struct RenderGraph;

		// ID
		struct _Texture;
		using TextureID = ID<_Texture, uint32, 0>;

		struct _Buffer;
		using BufferID = ID<_Buffer, uint32, 0>;

		struct _Sampler {};
		using SamplerID = ID<_Sampler, VkSampler, VK_NULL_HANDLE>;
		static constexpr SamplerID SAMPLER_ID_NULL = SamplerID();

		struct _PipelineState;
		using PipelineStateID = ID<_PipelineState, PoolID, 0>;
		static constexpr PipelineStateID PIPELINE_STATE_ID_NULL = PipelineStateID();

		struct _ShaderArgSet;
		using ShaderArgSetID = ID<_ShaderArgSet, uint32, 0>;

		struct _Shader;
		using ShaderID = ID<_Shader, uint16, 0>;
		static constexpr ShaderID SHADER_ID_NULL = ShaderID();

		struct _Program;
		using ProgramID = ID<_Program, uint16, 0>;
		static constexpr ProgramID PROGRAM_ID_NULL = ProgramID();

		struct _Semaphore;
		using SemaphoreID = ID<_Semaphore, uint32, 0>;
		static constexpr SemaphoreID SEMAPHORE_ID_NULL = SemaphoreID();

		static constexpr uint32 MAX_SET_PER_SHADER_PROGRAM = 6;
		static constexpr unsigned int SET_COUNT = 8;
		static constexpr uint32 MAX_BINDING_PER_SET = 16;
		static constexpr uint32 MAX_INPUT_PER_SHADER = 16;
		static constexpr uint32 MAX_INPUT_BINDING_PER_SHADER = 16;
		static constexpr uint32 MAX_COLOR_ATTACHMENT_PER_SHADER = 8;
		static constexpr uint32 MAX_INPUT_ATTACHMENT_PER_SHADER = 8;
		static constexpr uint32 MAX_DYNAMIC_BUFFER_PER_SET = 4;
		static constexpr uint32 MAX_SIGNAL_SEMAPHORE = 4;
		static constexpr uint32 MAX_VERTEX_BINDING = 16;

		enum class VertexElementType : uint8_t {
			BYTE,
			BYTE2,
			BYTE3,
			BYTE4,
			UBYTE,
			UBYTE2,
			UBYTE3,
			UBYTE4,
			SHORT,
			SHORT2,
			SHORT3,
			SHORT4,
			USHORT,
			USHORT2,
			USHORT3,
			USHORT4,
			INT,
			UINT,
			FLOAT,
			FLOAT2,
			FLOAT3,
			FLOAT4,
			HALF,
			HALF2,
			HALF3,
			HALF4,
			COUNT,

			DEFAULT = COUNT
		};

		using VertexElementFlagBits = enum {
			VERTEX_ELEMENT_INTEGER_TARGET = 0x1,
			VERTEX_ELEMENT_NORMALIZED = 0x2,
			VERTEX_ELEMENT_ENUM_END_BIT
		};
		using VertexElementFlags = uint8;
		static_assert(VERTEX_ELEMENT_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(VertexElementFlags), "");

		enum class ShaderStage : uint8 {
			NONE,
			VERTEX,
			GEOMETRY,
			FRAGMENT,
			COMPUTE,
			COUNT
		};

		using ShaderStageFlagBits = enum {
			SHADER_STAGE_VERTEX = 0x1,
			SHADER_STAGE_GEOMETRY = 0x2,
			SHADER_STAGE_FRAGMENT = 0x4,
			SHADER_STAGE_COMPUTE = 0x8,
			SHADER_STAGE_ENUM_END_BIT
		};
		using ShaderStageFlags = uint8;
		static_assert(SHADER_STAGE_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(ShaderStageFlags), "");

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
			BUFFER_USAGE_TRANSFER_SRC_BIT = 0x10,
			BUFFER_USAGE_TRANSFER_DST_BIT = 0x20,
			BUFFER_USAGE_ENUM_END_BIT
		};
		using BufferUsageFlags = uint8;
		static_assert(BUFFER_USAGE_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(BufferUsageFlags), "");

		using TextureUsageFlagBits = enum {
			TEXTURE_USAGE_SAMPLED_BIT = 0x1,
			TEXTURE_USAGE_COLOR_ATTACHMENT_BIT = 0x2,
			TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x4,
			TEXTURE_USAGE_INPUT_ATTACHMENT_BIT = 0x8,
			TEXTURE_USAGE_TRANSFER_SRC_BIT = 0x10,
			TEXTURE_USAGE_TRANSFER_DST_BIT = 0x20,
			TEXTURE_USAGE_STORAGE_BIT = 0x40,
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

			RGBA8UI,
			RGBA8,
			BGRA8,
			DEPTH24_STENCIL8UI,
			DEPTH32F,
			RGBA16F,
			R32UI,

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

		enum class RenderCommandType : uint8 {
			DRAW_INDEX,
			DRAW_VERTEX,
			DRAW_PRIMITIVE,
			DISPATCH,
			COUNT
		};

		struct RenderCommand {
			RenderCommandType type = RenderCommandType::COUNT;
		};

		template <RenderCommandType RENDER_COMMAND_TYPE>
		struct RenderCommandTyped : RenderCommand {
			static const RenderCommandType TYPE = RENDER_COMMAND_TYPE;

			RenderCommandTyped() { type = TYPE; }
		};

		struct RenderCommandDrawVertex : RenderCommandTyped<RenderCommandType::DRAW_VERTEX> {
			PipelineStateID pipelineStateID = PIPELINE_STATE_ID_NULL;
			ShaderArgSetID shaderArgSetIDs[MAX_SET_PER_SHADER_PROGRAM];
			BufferID vertexBufferID;
			uint16 vertexCount = 0;
		};

		struct RenderCommandDrawIndex : RenderCommandTyped<RenderCommandType::DRAW_INDEX> {
			PipelineStateID pipelineStateID = PIPELINE_STATE_ID_NULL;
			ShaderArgSetID shaderArgSetIDs[MAX_SET_PER_SHADER_PROGRAM];
			BufferID vertexBufferID;
			BufferID indexBufferID;
			uint16 indexOffset = 0;
			uint16 vertexOffset = 0;
			uint16 indexCount = 0;
		};

		struct RenderCommandDrawPrimitive : RenderCommandTyped<RenderCommandType::DRAW_PRIMITIVE> {
			PipelineStateID pipelineStateID = PIPELINE_STATE_ID_NULL;
			ShaderArgSetID shaderArgSetIDs[MAX_SET_PER_SHADER_PROGRAM];
			BufferID vertexBufferIDs[Soul::GPU::MAX_VERTEX_BINDING];
			BufferID indexBufferID;
		};

		enum class DescriptorType : uint8 {
			NONE,
			UNIFORM_BUFFER,
			SAMPLED_IMAGE,
			INPUT_ATTACHMENT,
			STORAGE_IMAGE,
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
				return type == DescriptorType::SAMPLED_IMAGE || type == DescriptorType::STORAGE_IMAGE;
			}

			static bool IsWriteableTexture(DescriptorType type) {
				return type == DescriptorType::STORAGE_IMAGE;
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
            union Color {
				Vec4f float32;
		        Vec4ui32 uint32;
		        Vec4i32 int32;
				Color() {
					float32 = {};
				}
            } color;

            struct {
                float depth;
                uint32 stencil;
            } depthStencil;

		};

		struct UniformDescriptor {
			BufferID bufferID;
			uint32 unitIndex;
		};

		struct SampledImageDescriptor {
			TextureID textureID;
			SamplerID samplerID;
		};

		struct StorageImageDescriptor {
		    TextureID textureID;
		    uint8 mipLevel;
		};

		struct InputAttachmentDescriptor {
		    TextureID textureID;
		};

		struct Descriptor {
			DescriptorType type = DescriptorType::NONE;
			union {
				UniformDescriptor uniformInfo;
				SampledImageDescriptor sampledImageInfo;
				StorageImageDescriptor storageImageInfo;
				InputAttachmentDescriptor inputAttachmentInfo;
			};
			ShaderStageFlags stageFlags = 0;

			inline static Descriptor Uniform(BufferID bufferID, uint32 unitIndex, ShaderStageFlags stageFlags) {
			    Descriptor descriptor = {};
			    descriptor.type = DescriptorType::UNIFORM_BUFFER;
			    descriptor.uniformInfo.bufferID = bufferID;
			    descriptor.uniformInfo.unitIndex = unitIndex;
				descriptor.stageFlags = stageFlags;
			    return descriptor;
			}

            inline static Descriptor SampledImage(TextureID textureID, SamplerID samplerID, ShaderStageFlags stageFlags) {
			    Descriptor descriptor = {};
			    descriptor.type = DescriptorType::SAMPLED_IMAGE;
			    descriptor.sampledImageInfo.textureID = textureID;
			    descriptor.sampledImageInfo.samplerID = samplerID;
				descriptor.stageFlags = stageFlags;
			    return descriptor;
			}

			inline static Descriptor StorageImage(TextureID textureID, uint8 mipLevel, ShaderStageFlags stageFlags) {
			    Descriptor descriptor = {};
			    descriptor.type = DescriptorType::STORAGE_IMAGE;
			    descriptor.storageImageInfo.textureID = textureID;
			    descriptor.storageImageInfo.mipLevel = mipLevel;
				descriptor.stageFlags = stageFlags;
			    return descriptor;
			}

			inline static Descriptor InputAttachment(TextureID textureID, ShaderStageFlags stageFlags) {
			    Descriptor descriptor = {};
			    descriptor.type = DescriptorType::INPUT_ATTACHMENT;
			    descriptor.inputAttachmentInfo.textureID = textureID;
				descriptor.stageFlags = stageFlags;
				return descriptor;
			}

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
			const char* name = nullptr;
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

		struct ProgramDesc {
			EnumArray<ShaderStage, ShaderID> shaderIDs;

			ProgramDesc() {
				shaderIDs = {};
			}

			inline bool operator==(const ProgramDesc& other) const {
				return (memcmp(this, &other, sizeof(ProgramDesc)) == 0);
			}

			inline bool operator!=(const ProgramDesc& other) const {
				return (memcmp(this, &other, sizeof(ProgramDesc)) != 0);
			}

			uint64 hash() const {
				return hashFNV1((uint8*)(this), sizeof(ProgramDesc));
			}
		};

		using AttachmentFlagBits = enum {
			ATTACHMENT_ACTIVE_BIT = 0x1,
			ATTACHMENT_FIRST_PASS_BIT = 0x2,
			ATTACHMENT_LAST_PASS_BIT = 0x4,
			ATTACHMENT_EXTERNAL_BIT = 0x8,
			ATTACHMENT_CLEAR_BIT = 0x10,
			ATTACHMENT_ENUM_END_BIT
		};
		using AttachmentFlags = uint8;
		static_assert(ATTACHMENT_ENUM_END_BIT - 1 < SOUL_UTYPE_MAX(AttachmentFlags), "");

		struct Attachment
		{
			TextureFormat format;
			AttachmentFlags flags;
		};

		struct InputLayoutDesc {
			Topology topology = Topology::TRIANGLE_LIST;
		};

		struct PipelineStateDesc {
			ProgramID programID = PROGRAM_ID_NULL;

			InputLayoutDesc inputLayout;

			struct InputBindingDesc {
				uint32 stride = 0;
			} inputBindings[MAX_INPUT_BINDING_PER_SHADER];

			struct InputAttrDesc {
				uint32 binding = 0;
				uint32 offset = 0;
				VertexElementType type = VertexElementType::DEFAULT;
				VertexElementFlags flags = 0;
			} inputAttributes[MAX_INPUT_PER_SHADER];

			struct ViewportDesc {
				uint16 offsetX = 0;
				uint16 offsetY = 0;
				uint16 width = 0;
				uint16 height = 0;
			} viewport;

			struct ScissorDesc {
				bool dynamic = false;
				uint16 offsetX = 0;
				uint16 offsetY = 0;
				uint16 width = 0;
				uint16 height = 0;
			} scissor;

			struct RasterDesc {
				float lineWidth = 1.0f;
				PolygonMode polygonMode = PolygonMode::FILL;
				CullMode cullMode = CullMode::NONE;
				FrontFace frontFace = FrontFace::CLOCKWISE;
			} raster;

			struct ColorAttachmentDesc {
				bool blendEnable = false;
				BlendFactor srcColorBlendFactor = BlendFactor::ZERO;
				BlendFactor dstColorBlendFactor = BlendFactor::ZERO;
				BlendOp colorBlendOp = BlendOp::ADD;
				BlendFactor srcAlphaBlendFactor = BlendFactor::ZERO;
				BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
				BlendOp alphaBlendOp = BlendOp::ADD;
			};
			ColorAttachmentDesc colorAttachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
			uint8 colorAttachmentCount = 0;

			struct DepthStencilAttachmentDesc {
				bool depthTestEnable = false;
				bool depthWriteEnable = false;
				CompareOp depthCompareOp = CompareOp::NEVER;
			};
			DepthStencilAttachmentDesc depthStencilAttachment;

			inline bool operator==(const PipelineStateDesc& other) const {
				return (memcmp(this, &other, sizeof(PipelineStateDesc)) == 0);
			}

			inline bool operator!=(const PipelineStateDesc& other) const {
				return (memcmp(this, &other, sizeof(PipelineStateDesc)) != 0);
			}

			uint64 hash() const {
				return hashFNV1((uint8*)(this), sizeof(PipelineStateDesc));
			}
		};

		// Private
		struct _Database;

		struct _Buffer {
			VkBuffer vkHandle = VK_NULL_HANDLE;
			VmaAllocation allocation{};
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
			VkImageView* mipViews;
			uint8 mipCount;
		};

		struct _DescriptorSetLayoutBinding {
			VkDescriptorType descriptorType;
			uint32 descriptorCount;
			VkShaderStageFlags stageFlags;
		};

		struct _DescriptorSetLayoutKey {
			_DescriptorSetLayoutBinding bindings[MAX_BINDING_PER_SET];

			inline bool operator==(const _DescriptorSetLayoutKey& other) const {
				return (memcmp(this, &other, sizeof(_DescriptorSetLayoutKey)) == 0);
			}

			inline bool operator!=(const _DescriptorSetLayoutKey& other) const {
				return (memcmp(this, &other, sizeof(_DescriptorSetLayoutKey)) != 0);
			}

			uint64 hash() const {
				return hashFNV1((uint8*)(this), sizeof(_DescriptorSetLayoutKey));
			}
		};

		struct _ShaderDescriptorBinding {
			DescriptorType type = {};
			uint8 count = 0;
			uint8 attachmentIndex = 0;
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
			DescriptorType type = DescriptorType::NONE;
			uint8 count = 0;
			uint8 attachmentIndex = 0;
			VkShaderStageFlags shaderStageFlags = 0;
			VkPipelineStageFlags pipelineStageFlags = 0;
		};

		struct _RenderPassKey
		{
			Attachment colorAttachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
			Attachment inputAttachments[MAX_INPUT_ATTACHMENT_PER_SHADER];
			Attachment depthAttachment;

			inline bool operator==(const _RenderPassKey& other) const {
				return (memcmp(this, &other, sizeof(_RenderPassKey)) == 0);
			}

			inline bool operator!=(const _RenderPassKey& other) const {
				return (memcmp(this, &other, sizeof(_RenderPassKey)) != 0);
			}

			uint64 hash() const {
				return hashFNV1((uint8*)(this), sizeof(_RenderPassKey));
			}
		};

		struct _Program {
			VkPipelineLayout pipelineLayout;
			VkDescriptorSetLayout descriptorLayouts[MAX_SET_PER_SHADER_PROGRAM];
			_ProgramDescriptorBinding bindings[MAX_SET_PER_SHADER_PROGRAM][MAX_BINDING_PER_SET];
			EnumArray<ShaderStage, ShaderID> shaderIDs;
		};

		struct _QueueData {
			int count = 0;
			uint32 indices[3] = {};
		};

		enum class _SemaphoreState : uint8 {
            INITIAL,
            SUBMITTED,
            PENDING
		};

		struct _Semaphore {
			VkSemaphore vkHandle = VK_NULL_HANDLE;
			VkPipelineStageFlags stageFlags = 0;
            _SemaphoreState state = _SemaphoreState::INITIAL;

            bool isPending() { return state == _SemaphoreState::PENDING; }
		};

		struct _CommandPool
		{
			VkCommandPool vkHandle = VK_NULL_HANDLE;
			Array<VkCommandBuffer> allocatedBuffers;
			uint16 count = 0;
		};

		struct alignas(SOUL_CACHELINE_SIZE) _ThreadContext {
			Runtime::AllocatorInitializer allocatorInitializer;
			_CommandPool secondaryCommandPool;

			_ThreadContext(Memory::Allocator* allocator) : allocatorInitializer(allocator)
			{
				allocatorInitializer.end();
			}
		};

		struct _FrameContext {

			Runtime::AllocatorInitializer allocatorInitializer;

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
				Array<VkRenderPass> renderPasses;
				Array<VkFramebuffer> frameBuffers;
				Array<VkPipeline> pipelines;
				Array<VkEvent> events;
				Array<SemaphoreID> semaphores;
			} garbages;

			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

			Array<_Buffer> stagingBuffers;
			VkCommandBuffer stagingCommandBuffer = VK_NULL_HANDLE;
			VkCommandBuffer clearCommandBuffer = VK_NULL_HANDLE;
			VkCommandBuffer genMipmapCommandBuffer = VK_NULL_HANDLE;

			bool stagingAvailable = false;
			bool stagingSynced = false;

			_FrameContext(Memory::Allocator* allocator) : allocatorInitializer(allocator),
				commandPools(VK_NULL_HANDLE),
				usedCommandBuffers(0)
			{
				allocatorInitializer.end();
			}
		};

		struct _Swapchain {
			VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
			VkSurfaceFormatKHR format = {};
			VkExtent2D extent = {};
			Array<TextureID> textures;
			Array<VkImage> images;
			Array<VkImageView> imageViews;
			Array<VkFence> fences;
		};

		struct _PipelineState {
			VkPipeline vkHandle;
			VkPipelineBindPoint bindPoint;
			ProgramID programID;
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

			using CPUAllocatorProxy = Memory::MultiProxy<Memory::ProfileProxy, Memory::CounterProxy>;
			using CPUAllocator = Memory::ProxyAllocator<
			        Memory::Allocator,
			        CPUAllocatorProxy>;
			CPUAllocator cpuAllocator;
			Runtime::AllocatorInitializer allocatorInitializer;

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
			uint32 frameCounter;
			uint32 currentFrame;

			VmaAllocator gpuAllocator;

			Pool<_Buffer> buffers;
			Pool<_Texture> textures;
			Pool<_Shader> shaders;

			HashMap<PipelineStateDesc, PipelineStateID> pipelineStateMaps;
			Pool<_PipelineState> pipelineStates;

			HashMap<_DescriptorSetLayoutKey, VkDescriptorSetLayout> descriptorSetLayoutMaps;

			HashMap<ProgramDesc, ProgramID> programMaps;
			Pool<_Program> programs;

			HashMap<_RenderPassKey, VkRenderPass> renderPassMaps;

			Pool<_Semaphore> semaphores;

			UInt64HashMap<VkSampler> samplerMap;

			UInt64HashMap<VkDescriptorSet> descriptorSets;
			Array<_ShaderArgSet> shaderArgSetIDs;

			EnumArray<QueueType, _Submission> submissions;

			std::mutex shaderArgSetRequestMutex;
			std::mutex pipelineStateRequestMutex;

			explicit _Database(Memory::Allocator* backingAllocator):
				cpuAllocator("GPU System", backingAllocator, CPUAllocatorProxy::Config{ Memory::CounterProxy::Config()}),
				allocatorInitializer(&cpuAllocator) {
				allocatorInitializer.end();
			}
		};

	}
}


