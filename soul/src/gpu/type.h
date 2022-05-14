#pragma once

// TODO: Figure out how to do it without single header library
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VK_NO_PROTOTYPES
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include "gpu/object_pool.h"
#include "gpu/object_cache.h"
#include "gpu/id.h"
#include "gpu/constant.h"
#include "gpu/intern/render_compiler.h"
#include "gpu/intern/shader_arg_set_allocator.h"

#include "core/type.h"
#include "core/mutex.h"
#include "core/array.h"
#include "core/enum_array.h"
#include "core/pool.h"
#include "core/uint64_hash_map.h"
#include "core/hash_map.h"
#include "core/flag_set.h"

#include "runtime/runtime.h"
#include "memory/allocators/proxy_allocator.h"

#include <shared_mutex>

namespace soul::gpu
{
	class System;
	class RenderGraph;

	
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
	static_assert(VERTEX_ELEMENT_ENUM_END_BIT - 1 <= SOUL_UTYPE_MAX(VertexElementFlags));

	enum class ShaderStage : uint8 {
		VERTEX,
		GEOMETRY,
		FRAGMENT,
		COMPUTE,
		COUNT
	};
	using ShaderStageFlags = FlagSet<ShaderStage>;
	constexpr ShaderStageFlags SHADER_STAGES_VERTEX_FRAGMENT = ShaderStageFlags( { gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT });

	enum class ResourceOwner : uint8 {
		NONE,
		GRAPHIC_QUEUE,
		COMPUTE_QUEUE,
		TRANSFER_QUEUE,
		PRESENTATION_ENGINE,
		COUNT
	};

	enum class QueueType : uint8 {
		GRAPHIC,
		COMPUTE,
		TRANSFER,
		COUNT
	};
	using QueueFlags = FlagSet<QueueType>;
	constexpr QueueFlags QUEUE_DEFAULT = { QueueType::GRAPHIC, QueueType::COMPUTE, QueueType::TRANSFER };
	
	enum class BufferUsage : uint8
	{
		INDEX,
		VERTEX,
		UNIFORM,
		STORAGE,
		TRANSFER_SRC,
		TRANSFER_DST,
		COUNT
	};
	using BufferUsageFlags = FlagSet<BufferUsage>;

	enum class TextureUsage : uint8 {
		SAMPLED,
		COLOR_ATTACHMENT,
		DEPTH_STENCIL_ATTACHMENT,
		INPUT_ATTACHMENT,
		TRANSFER_SRC,
		TRANSFER_DST,
		STORAGE,
		COUNT
	};
	using TextureUsageFlags = FlagSet<TextureUsage>;
	
	enum class TextureType : uint8 {
		D1,
		D2,
		D2_ARRAY,
		D3,
		CUBE,
		COUNT
	};

	enum class TextureFormat : uint16 {

		DEPTH16,

		RGB8,
		DEPTH24,

		RGBA8UI,
		RGBA8,
		BGRA8,
		RG16UI,
		DEPTH24_STENCIL8UI,
		DEPTH32F,
		RGBA16F,
		R32UI,
		SRGBA_8,

		RGB16,
		RGB16F,
		RGB16UI,
		RGB16I,
		R11F_G11F_B10F,

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

			Color(Vec4f val) : float32(val) {}
			Color(Vec4ui32 val) : uint32(val) {}
			Color(Vec4i32 val) : int32(val) {}

		} color;

		struct DepthStencil {
			float depth = 0.0f;
			uint32 stencil = 0;
			DepthStencil() = default;
			DepthStencil(float depth, uint32 stencil) : depth(depth), stencil(stencil) {}
		} depthStencil;

		ClearValue() = default;
		ClearValue(Vec4f color, float depth, uint32 stencil) : color(color), depthStencil(depth, stencil) {}
		ClearValue(Vec4ui32 color, float depth, uint32 stencil) : color(color), depthStencil(depth, stencil) {}
		ClearValue(Vec4i32 color, float depth, uint32 stencil) : color(color), depthStencil(depth, stencil) {}

	};

	class SubresourceIndex
	{
	private:
		using value_type = uint16;

		static constexpr value_type LEVEL_MASK = 0xFF;
		static constexpr value_type LEVEL_BIT_SHIFT = 0;

		static constexpr value_type LAYER_MASK = 0xFF00;
		static constexpr uint8 LAYER_BIT_SHIFT = 8;

		value_type index_ = 0;

	public:
		constexpr SubresourceIndex() = default;
		constexpr explicit SubresourceIndex(const uint8 level, const uint8 layer) : index_(level | layer << 8) {}

		[[nodiscard]] constexpr uint8 get_level() const
		{
			return soul::cast<uint8>((index_ & LEVEL_MASK) >> LEVEL_BIT_SHIFT);
		}

		[[nodiscard]] constexpr uint8 get_layer() const
		{
			return soul::cast<uint8>((index_ & LAYER_MASK) >> LAYER_BIT_SHIFT);
		}

	};

	struct SubresourceIndexRange
	{
		using value_type = SubresourceIndex;

		SubresourceIndex base;
		uint32 levelCount = 1;
		uint32 layerCount = 1;

		class const_iterator
		{
		private:
			uint8 mip_ = 0;
			uint8 layer_ = 1;
			uint32 mip_end_ = 1;
		public:

			using pointer_type = SubresourceIndex*;
			using value_type = SubresourceIndex;
			using iterator_category = std::input_iterator_tag;
			using difference_type = std::ptrdiff_t;

			const_iterator() noexcept = default;
			const_iterator(uint8 mip, uint8 layer, uint32 mip_end_) noexcept : mip_(mip), layer_(layer), mip_end_(mip_end_) {}

			const value_type operator*() const { return SubresourceIndex(mip_, layer_); }

			const_iterator& operator++()
			{
				mip_ += 1;
				if (mip_ == mip_end_)
				{
					mip_ = 0;
					layer_ += 1;
				}
				return *this;
			}

			const const_iterator operator++(int) { const_iterator t(mip_, layer_, mip_end_); this->operator++(); return t; }

			bool operator==(const_iterator const& rhs) const { return (mip_ == rhs.mip_ && layer_ == rhs.layer_ && mip_end_ == rhs.mip_end_); }
			bool operator!=(const_iterator const& rhs) const { return (mip_ != rhs.mip_ || layer_ != rhs.layer_ || mip_end_ != rhs.mip_end_); }
		};

		const_iterator begin() noexcept { return const_iterator(base.get_level(), base.get_layer(), base.get_level() + levelCount); }
		const_iterator end() noexcept { return const_iterator(base.get_level(), base.get_layer() + layerCount, base.get_level() + levelCount); }
		const_iterator begin() const noexcept { return const_iterator(base.get_level(), base.get_layer(), base.get_level() + levelCount); }
		const_iterator end() const noexcept { return const_iterator(base.get_level(), base.get_layer() + layerCount, base.get_level() + levelCount); }
	};


	struct UniformDescriptor {
		BufferID bufferID;
		uint32 unitIndex;
	};

	struct SampledImageDescriptor {
		TextureID textureID;
		SamplerID samplerID;
		std::optional<SubresourceIndex> view = std::nullopt;
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
		ShaderStageFlags stageFlags;

		inline static Descriptor Uniform(BufferID bufferID, uint32 unitIndex, ShaderStageFlags stageFlags) {
			Descriptor descriptor = {};
			descriptor.type = DescriptorType::UNIFORM_BUFFER;
			descriptor.uniformInfo.bufferID = bufferID;
			descriptor.uniformInfo.unitIndex = unitIndex;
			descriptor.stageFlags = stageFlags;
			return descriptor;
		}

		inline static Descriptor SampledImage(TextureID textureID, SamplerID samplerID, ShaderStageFlags stageFlags, std::optional<SubresourceIndex> view = std::nullopt) {
			Descriptor descriptor = {};
			descriptor.type = DescriptorType::SAMPLED_IMAGE;
			descriptor.sampledImageInfo.textureID = textureID;
			descriptor.sampledImageInfo.samplerID = samplerID;
			descriptor.sampledImageInfo.view = view;
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



	struct BufferDesc {
		soul_size count = 0;
		uint16 typeSize = 0;
		uint16 typeAlignment = 0;
		BufferUsageFlags usageFlags;
		QueueFlags  queueFlags = QUEUE_DEFAULT;
	};

	struct TextureSubresourceLayers
	{
		uint32 mipLevel;
		uint32 baseArrayLayer;
		uint32 layerCount;
	};

	struct TextureCopyRegion
	{
		TextureSubresourceLayers srcSubresource = {};
		Vec3i32 srcOffset;
		TextureSubresourceLayers dstSubresource = {};
		Vec3i32 dstOffset;
		Vec3ui32 extent;
	};

	struct TextureRegion
	{
		Vec3i32 offset;
		Vec3ui32 extent;
		uint32 mipLevel = 0;
		uint32 baseArrayLayer = 0;
		uint32 layerCount = 0;
	};

	struct TextureRegionLoad
	{
		soul_size bufferOffset = 0;
		uint32 bufferRowLength = 0;
		uint32 bufferImageHeight = 0;
		TextureRegion textureRegion;
	};

	struct TextureLoadDesc
	{
		const void* data = nullptr;
		uint32 dataSize = 0;

		uint32 regionLoadCount = 0;
		TextureRegionLoad* regionLoads = nullptr;

		bool generateMipmap = false;
	};

	enum class TextureSampleCount : uint8
	{
		COUNT_1 = 1,
		COUNT_2 = 2,
		COUNT_4 = 4,
		COUNT_8 = 8,
		COUNT_16 = 16,
		COUNT_32 = 32,
		COUNT_64 = 64
	};

	struct TextureDesc {
		TextureType type = TextureType::D2;
		TextureFormat format = TextureFormat::COUNT;
		Vec3ui32 extent;
		uint32 mipLevels = 1;
		uint16 layerCount = 1;
		TextureSampleCount sampleCount = TextureSampleCount::COUNT_1;
		TextureUsageFlags usageFlags;
		QueueFlags queueFlags;
		const char* name = nullptr;

		static TextureDesc D2(const char* name, TextureFormat format, uint32 mipLevels, TextureUsageFlags usageFlags, QueueFlags queueFlags, Vec2ui32 dimension, TextureSampleCount sampleCount = TextureSampleCount::COUNT_1)
		{
			return {
				.type = TextureType::D2,
				.format = format,
				.extent = Vec3ui32(dimension.x, dimension.y, 1),
				.mipLevels =mipLevels,
				.sampleCount = sampleCount,
				.usageFlags = usageFlags,
				.queueFlags = queueFlags,
				.name = name
			};
		}

		static TextureDesc D2Array(const char* name, TextureFormat format, uint32 mipLevels, TextureUsageFlags usageFlags, QueueFlags queueFlags, Vec2ui32 dimension, uint16 layerCount)
		{
			return {
				.type = TextureType::D2_ARRAY,
				.format = format,
				.extent = Vec3ui32(dimension.x, dimension.y, 1),
				.mipLevels = mipLevels,
				.layerCount = layerCount,
				.usageFlags = usageFlags,
				.queueFlags = queueFlags,
				.name = name
			};
		}

		static TextureDesc Cube(const char* name, TextureFormat format, uint32 mipLevels, TextureUsageFlags usageFlags, QueueFlags queueFlags, Vec2ui32 dimension)
		{
			return {
				.type = TextureType::CUBE,
				.format = format,
				.extent = Vec3ui32(dimension.x, dimension.y, 1),
				.mipLevels = mipLevels,
				.layerCount = 6,
				.usageFlags = usageFlags,
				.queueFlags = queueFlags,
				.name = name
			};
		}

		[[nodiscard]] soul_size get_view_count() const
		{
			return mipLevels * layerCount;
		}

	};



	struct SamplerDesc {
		TextureFilter minFilter = TextureFilter::COUNT;
		TextureFilter magFilter = TextureFilter::COUNT;
		TextureFilter mipmapFilter = TextureFilter::COUNT;
		TextureWrap wrapU = TextureWrap::CLAMP_TO_EDGE;
		TextureWrap wrapV = TextureWrap::CLAMP_TO_EDGE;
		TextureWrap wrapW = TextureWrap::CLAMP_TO_EDGE;
		bool anisotropyEnable = false;
		float maxAnisotropy = 0.0f;
		bool compareEnable = false;
		CompareOp comapreOp = CompareOp::COUNT;

		static constexpr SamplerDesc SameFilterWrap(TextureFilter filter, TextureWrap wrap, bool anisotropyEnable = false, 
			float maxAnisotropy = 0.0f, bool compareEnable = false, CompareOp compareOp = CompareOp::ALWAYS)
		{
			SamplerDesc desc;
			desc.minFilter = filter;
			desc.magFilter = filter;
			desc.mipmapFilter = filter;
			desc.wrapU = wrap;
			desc.wrapV = wrap;
			desc.wrapW = wrap;
			desc.anisotropyEnable = anisotropyEnable;
			desc.maxAnisotropy = maxAnisotropy;
			desc.compareEnable = compareEnable;
			desc.comapreOp = compareOp;
			return desc;
		}
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

		bool operator==(const ProgramDesc& other) const {
			return (memcmp(this, &other, sizeof(ProgramDesc)) == 0);
		}

		bool operator!=(const ProgramDesc& other) const {
			return (memcmp(this, &other, sizeof(ProgramDesc)) != 0);
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
		TextureSampleCount sampleCount;
		AttachmentFlags flags;
	};

	struct InputLayoutDesc {
		Topology topology = Topology::TRIANGLE_LIST;
	};

	struct GraphicPipelineStateDesc {

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
			bool colorWrite = true;
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

		struct DepthBiasDesc
		{
			float constant = 0.0f;
			float slope = 0.0f;
		};
		DepthBiasDesc depthBias;

		bool operator==(const GraphicPipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(GraphicPipelineStateDesc)) == 0);
		}

		bool operator!=(const GraphicPipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(GraphicPipelineStateDesc)) != 0);
		}
	};


	struct ComputePipelineStateDesc
	{
		ProgramID programID = PROGRAM_ID_NULL;

		bool operator==(const ComputePipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(ComputePipelineStateDesc)) == 0);
		}

		bool operator!=(const ComputePipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(ComputePipelineStateDesc)) != 0);
		}
	};

	template <typename T>
	class VulkanPool
	{
	public:

		using ID = PoolID;

		explicit VulkanPool(memory::Allocator* allocator = GetDefaultAllocator()) noexcept : pool_(allocator) {}
		VulkanPool(const VulkanPool& other) = delete;
		VulkanPool& operator=(const VulkanPool& other) = delete;
		VulkanPool(VulkanPool&& other) = delete;
		VulkanPool& operator=(VulkanPool&& other) = delete;
		~VulkanPool() = default;

		void reserve(soul_size capacity)
		{
			lock_.lock();
			pool_.reserve(capacity);
			lock_.unlock();
		}

		ID add(const T& datum)
		{
			lock_.lock();
			const ID id = pool_.add(datum);
			lock_.unlock();
			return id;
		}

		ID add(T&& datum)
		{
			lock_.lock();
			const ID id = pool_.add(std::move(datum));
			lock_.unlock();
			return id;
		}

		void remove(ID id)
		{
			pool_.remove(id);
		}

		SOUL_NODISCARD T& operator[](ID id)
		{
			return pool_[id];
		}

		SOUL_NODISCARD const T& operator[](PoolID id) const
		{
			return pool_[id];
		}

		SOUL_NODISCARD T* ptr(PoolID id) const
		{
			return pool_.ptr(id);
		}

		void clear() { pool_.clear(); }
		void cleanup() { pool_.cleanup(); }

	private:
		mutable RWSpinMutex lock_;
		Pool<T> pool_;
	};

	namespace impl
	{

		struct PipelineState {
			VkPipeline vkHandle;
			VkPipelineBindPoint bindPoint;
			ProgramID programID;
		};

		struct ProgramDescriptorBinding {
			DescriptorType type = DescriptorType::NONE;
			uint8 count = 0;
			uint8 attachmentIndex = 0;
			VkShaderStageFlags shaderStageFlags = 0;
			VkPipelineStageFlags pipelineStageFlags = 0;
		};

		struct RenderPassKey
		{
			Attachment colorAttachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
			Attachment resolveAttachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
			Attachment inputAttachments[MAX_INPUT_ATTACHMENT_PER_SHADER];
			Attachment depthAttachment;

			bool operator==(const RenderPassKey& other) const {
				return (memcmp(this, &other, sizeof(RenderPassKey)) == 0);
			}

			bool operator!=(const RenderPassKey& other) const {
				return (memcmp(this, &other, sizeof(RenderPassKey)) != 0);
			}
		};

		struct QueueData {
			uint32 count = 0;
			uint32 indices[3] = {};
		};

		enum class semaphore_state : uint8 {
			INITIAL,
			SUBMITTED,
			PENDING
		};

		struct Swapchain {
			VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
			VkSurfaceFormatKHR format = {};
			VkExtent2D extent = {};
			Array<TextureID> textures;
			Array<VkImage> images;
			Array<VkImageView> imageViews;
			Array<VkFence> fences;
		};

		struct DescriptorSetLayoutBinding {
			VkDescriptorType descriptorType;
			uint32 descriptorCount;
			VkShaderStageFlags stageFlags;
		};

		struct DescriptorSetLayoutKey {
			DescriptorSetLayoutBinding bindings[MAX_BINDING_PER_SET];

			bool operator==(const DescriptorSetLayoutKey& other) const {
				return (memcmp(this, &other, sizeof(DescriptorSetLayoutKey)) == 0);
			}

			bool operator!=(const DescriptorSetLayoutKey& other) const {
				return (memcmp(this, &other, sizeof(DescriptorSetLayoutKey)) != 0);
			}
		};

		struct ShaderDescriptorBinding {
			DescriptorType type = {};
			uint8 count = 0;
			uint8 attachmentIndex = 0;
		};

		struct ShaderInput {
			VkFormat format = VK_FORMAT_UNDEFINED;
			uint32 offset = 0;
		};

		struct Buffer {
			BufferDesc desc;
			VkBuffer vkHandle = VK_NULL_HANDLE;
			soul_size unitSize;
			VmaAllocation allocation{};
			ResourceOwner owner = ResourceOwner::NONE;
		};

		struct Texture {
			TextureDesc desc;
			VkImage vkHandle = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkSharingMode sharingMode = {};
			ResourceOwner owner = ResourceOwner::NONE;
			VkImageView* views = nullptr;
		};

		struct Shader {
			VkShaderModule module = VK_NULL_HANDLE;
			ShaderDescriptorBinding bindings[MAX_SET_PER_SHADER_PROGRAM][MAX_BINDING_PER_SET];
			ShaderInput inputs[MAX_INPUT_PER_SHADER];
			uint32 inputStride = 0;
		};



		struct Program {
			VkPipelineLayout pipelineLayout;
			VkDescriptorSetLayout descriptorLayouts[MAX_SET_PER_SHADER_PROGRAM];
			ProgramDescriptorBinding bindings[MAX_SET_PER_SHADER_PROGRAM][MAX_BINDING_PER_SET];
			EnumArray<ShaderStage, ShaderID> shaderIDs;
		};

		struct Semaphore {
			VkSemaphore vkHandle = VK_NULL_HANDLE;
			VkPipelineStageFlags stageFlags = 0;
			semaphore_state state = semaphore_state::INITIAL;

			bool isPending() { return state == semaphore_state::PENDING; }
		};

		class CommandQueue {

		public:
			void init(VkDevice device, uint32 familyIndex, uint32 queueIndex);
			void wait(Semaphore* semaphore, VkPipelineStageFlags waitStages);
			void submit(VkCommandBuffer commandBuffer, const Array<Semaphore*>& semaphores, VkFence fence = VK_NULL_HANDLE);
			void submit(VkCommandBuffer commandBuffer, Semaphore* semaphore, VkFence fence = VK_NULL_HANDLE);
			void submit(VkCommandBuffer commandBuffer, uint32 semaphoreCount = 0, Semaphore* const* semaphores = nullptr, VkFence fence = VK_NULL_HANDLE);
			void flush(uint32 semaphoreCount, Semaphore* const* semaphores, VkFence fence);
			void present(const VkPresentInfoKHR& presentInfo);
			SOUL_NODISCARD uint32 getFamilyIndex() const { return familyIndex; }
		private:
			VkDevice device = VK_NULL_HANDLE;
			VkQueue vkHandle = VK_NULL_HANDLE;
			uint32 familyIndex = 0;
			Array<VkSemaphore> waitSemaphores;
			Array<VkPipelineStageFlags> waitStages;
			Array<VkCommandBuffer> commands;
		};

		class SecondaryCommandBuffer
		{
		private:
			VkCommandBuffer vk_handle_ = VK_NULL_HANDLE;
		public:
			constexpr SecondaryCommandBuffer() noexcept = default;
			explicit constexpr SecondaryCommandBuffer(VkCommandBuffer vk_handle) : vk_handle_(vk_handle) {}
			[[nodiscard]] constexpr VkCommandBuffer get_vk_handle() const noexcept { return vk_handle_; }
			void end();
		};

		class PrimaryCommandBuffer
		{
		private:
			VkCommandBuffer vk_handle_ = VK_NULL_HANDLE;
		public:
			constexpr PrimaryCommandBuffer() noexcept = default;
			explicit constexpr PrimaryCommandBuffer(VkCommandBuffer vk_handle) : vk_handle_(vk_handle) {}
			[[nodiscard]] constexpr VkCommandBuffer get_vk_handle() const noexcept { return vk_handle_; }
			[[nodiscard]] bool is_null() const noexcept { return vk_handle_ == VK_NULL_HANDLE; }
			void begin_render_pass(const VkRenderPassBeginInfo& render_pass_begin_info, VkSubpassContents subpass_contents);
			void end_render_pass();
			void execute_secondary_command_buffers(uint32_t count, const SecondaryCommandBuffer* secondary_command_buffers);
		};

		using CommandQueues = EnumArray<QueueType, CommandQueue>;

		class CommandPool {
		public:

			explicit CommandPool(memory::Allocator* allocator = GetDefaultAllocator()) : allocatorInitializer(allocator)
			{
				allocatorInitializer.end();
			}

			void init(VkDevice device, VkCommandBufferLevel level, uint32 queueFamilyIndex);
			void reset();
			VkCommandBuffer request();

		private:
			runtime::AllocatorInitializer allocatorInitializer;
			VkDevice device = VK_NULL_HANDLE;
			VkCommandPool vkHandle = VK_NULL_HANDLE;
			Array<VkCommandBuffer> allocatedBuffers;
			VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
			uint16 count = 0;
		};

		class CommandPools
		{
		public:
			explicit CommandPools(memory::Allocator* allocator = GetDefaultAllocator()) : allocator_(allocator), allocator_initializer_(allocator)
			{
				allocator_initializer_.end();
			}
			void init(VkDevice device, const CommandQueues& queues, soul_size thread_count);
			void reset();
			VkCommandBuffer requestCommandBuffer(QueueType queueType);

			PrimaryCommandBuffer request_command_buffer(const QueueType queue_type);
			SecondaryCommandBuffer request_secondary_command_buffer(VkRenderPass render_pass, const uint32_t subpass, VkFramebuffer framebuffer);

		private:
			memory::Allocator* allocator_;
			runtime::AllocatorInitializer allocator_initializer_;
			Array<EnumArray<QueueType, CommandPool>> primary_pools_;
			Array<CommandPool> secondary_pools_;
		};

		class GPUResourceInitializer
		{
		public:
			void init(VmaAllocator gpuAllocator, CommandPools* commandPools);
			void load(Buffer& buffer, const void* data);
			void load(Texture& texture, const TextureLoadDesc& loadDesc);
			void clear(Texture& texture, ClearValue clearValue);
			void generate_mipmap(Texture& texture);
			void flush(CommandQueues& command_queues, System& gpu_system);
			void reset();
		private:

			struct StagingBuffer
			{
				VkBuffer vkHandle;
				VmaAllocation allocation;
			};

			struct alignas(SOUL_CACHELINE_SIZE) ThreadContext
			{
				PrimaryCommandBuffer transfer_command_buffer_;
				PrimaryCommandBuffer clear_command_buffer_;
				PrimaryCommandBuffer mipmap_gen_command_buffer_;
				Array<StagingBuffer> staging_buffers_;
			};

			VmaAllocator gpu_allocator_ = nullptr;
			CommandPools* command_pools_ = nullptr;
			
			ThreadContext& get_thread_context();
			StagingBuffer get_staging_buffer(soul_size size);
			PrimaryCommandBuffer get_transfer_command_buffer();
			PrimaryCommandBuffer get_mipmap_gen_command_buffer();
			PrimaryCommandBuffer get_clear_command_buffer();
			void load_staging_buffer(const StagingBuffer&, const void* data, soul_size size);
			void load_staging_buffer(const StagingBuffer&, const void* data, soul_size count, soul_size type_size, soul_size stride);

			Array<ThreadContext> thread_contexts_;
		};

		class GPUResourceFinalizer
		{
		public:
			void init();
			void finalize(Buffer& buffer);
			void finalize(Texture& texture, TextureUsageFlags usage_flags);
			void flush(CommandPools& command_pools, CommandQueues& command_queues, System& gpu_system);
		private:
			struct alignas(SOUL_CACHELINE_SIZE) ThreadContext
			{
				EnumArray<QueueType, Array<VkImageMemoryBarrier>> image_barriers_;
				EnumArray<QueueType, QueueFlags> sync_dst_queues_;

				// TODO(kevinyu): Investigate why we cannot use explicitly or implicitly default constructor here.
				// - alignas seems to be relevant
				// - only happen on release mode
				ThreadContext() {}
				ThreadContext(const ThreadContext&) = default;
				ThreadContext& operator=(const ThreadContext&) = default;
				ThreadContext(ThreadContext&&) = default;
				ThreadContext& operator=(ThreadContext&&) = default;
				~ThreadContext() {}
			};

			Array<ThreadContext> thread_contexts_;
		};


		struct _FrameContext {

			runtime::AllocatorInitializer allocatorInitializer;
			CommandPools commandPools;
			
			VkFence fence = VK_NULL_HANDLE;
			SemaphoreID imageAvailableSemaphore;
			SemaphoreID renderFinishedSemaphore;

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

			GPUResourceInitializer gpuResourceInitializer;
			GPUResourceFinalizer gpuResourceFinalizer;

			_FrameContext(memory::Allocator* allocator) : allocatorInitializer(allocator)
			{
				allocatorInitializer.end();
			}
		};

		struct Database {

			using CPUAllocatorProxy = memory::MultiProxy<memory::ProfileProxy, memory::CounterProxy>;
			using CPUAllocator = memory::ProxyAllocator<
				memory::Allocator,
				CPUAllocatorProxy>;
			CPUAllocator cpuAllocator;

			memory::MallocAllocator vulkanCPUBackingAllocator{ "Vulkan CPU Backing Allocator" };
			using VulkanCPUAllocatorProxy = memory::MultiProxy<memory::MutexProxy, memory::ProfileProxy>;
			using VulkanCPUAllocator = memory::ProxyAllocator<
				memory::MallocAllocator,
				VulkanCPUAllocatorProxy>;
			VulkanCPUAllocator vulkanCPUAllocator;

			runtime::AllocatorInitializer allocatorInitializer;

			VkInstance instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debugMessenger;

			VkDevice device = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkPhysicalDeviceProperties physicalDeviceProperties;
			VkPhysicalDeviceFeatures physicalDeviceFeatures;

			CommandQueues queues;

			VkSurfaceKHR surface;
			VkSurfaceCapabilitiesKHR surfaceCaps;

			Swapchain swapchain;

			Array<_FrameContext> frameContexts;
			uint32 frameCounter;
			uint32 currentFrame;

			VmaAllocator gpuAllocator;

			ConcurrentObjectPool<Buffer> bufferPool;
			ConcurrentObjectPool<Texture> texturePool;
			ConcurrentObjectPool<Shader> shaders;

			PipelineStateCache pipeline_state_cache_;
			DescriptorSetLayoutCache descriptor_set_layout_cache;
			
			HashMap<ProgramDesc, ProgramID> programMaps;
			Pool<Program> programs;

			HashMap<RenderPassKey, VkRenderPass> renderPassMaps;

			Pool<Semaphore> semaphores;

			UInt64HashMap<VkSampler> samplerMap;
			ShaderArgSetAllocator arg_set_allocator;

			explicit Database(memory::Allocator* backingAllocator) :
				cpuAllocator("GPU System allocator", backingAllocator, CPUAllocatorProxy::Config{ memory::ProfileProxy::Config(), memory::CounterProxy::Config() }),
				vulkanCPUAllocator("Vulkan allocator", &vulkanCPUBackingAllocator, VulkanCPUAllocatorProxy::Config{ memory::MutexProxy::Config(), memory::ProfileProxy::Config() }),
				allocatorInitializer(&cpuAllocator) {
				allocatorInitializer.end();
			}
		};

	}


	// Render Command API
	enum class RenderCommandType : uint8 {
		DRAW_INDEX,
		DRAW_PRIMITIVE,
		DISPATCH,
		COPY_TEXTURE,
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

	struct RenderCommandDrawIndex : RenderCommandTyped<RenderCommandType::DRAW_INDEX> {
		static constexpr QueueType QUEUE_TYPE = QueueType::GRAPHIC;

		PipelineStateID pipelineStateID = PIPELINE_STATE_ID_NULL;
		ShaderArgSetID shaderArgSetIDs[MAX_SET_PER_SHADER_PROGRAM];
		BufferID vertexBufferID;
		BufferID indexBufferID;
		uint16 indexOffset = 0;
		uint16 vertexOffset = 0;
		uint16 indexCount = 0;
	};

	struct RenderCommandDrawPrimitive : RenderCommandTyped<RenderCommandType::DRAW_PRIMITIVE> {
		static constexpr QueueType QUEUE_TYPE = QueueType::GRAPHIC;
		PipelineStateID pipelineStateID = PIPELINE_STATE_ID_NULL;
		ShaderArgSetID shaderArgSetIDs[MAX_SET_PER_SHADER_PROGRAM];
		BufferID vertexBufferIDs[soul::gpu::MAX_VERTEX_BINDING];
		BufferID indexBufferID;
	};

	struct RenderCommandDispatch : RenderCommandTyped<RenderCommandType::DISPATCH>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::COMPUTE;
		PipelineStateID pipelineStateID = PIPELINE_STATE_ID_NULL;
		ShaderArgSetID shaderArgSetIDs[MAX_SET_PER_SHADER_PROGRAM];
		Vec3ui32 groupCount;
	};

	struct RenderCommandCopyTexture : RenderCommandTyped<RenderCommandType::COPY_TEXTURE>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::TRANSFER;
		TextureID srcTexture;
		TextureID dstTexture;
		uint32 regionCount = 0;
		const TextureCopyRegion* regions = nullptr;
	};

	template <typename Func, typename RenderCommandType>
	concept command_generator = std::invocable<Func, soul_size> && std::same_as<RenderCommandType, std::invoke_result_t<Func, soul_size>>;

	template <typename T>
	concept render_command = std::derived_from<T, RenderCommand>;

	template <typename T>
	concept graphic_render_command = render_command<T> && T::QUEUE_TYPE == QueueType::GRAPHIC;

	class GraphicCommandList
	{
	private:
		impl::PrimaryCommandBuffer primary_command_buffer_;
		const VkRenderPassBeginInfo& render_pass_begin_info_;
		impl::CommandPools& command_pools_;
		System& gpu_system_;

		static constexpr uint32 SECONDARY_COMMAND_BUFFER_THRESHOLD = 10;

	public:
		constexpr GraphicCommandList(impl::PrimaryCommandBuffer primary_command_buffer,
			const VkRenderPassBeginInfo& render_pass_begin_info, impl::CommandPools& command_pools, System& gpu_system) :
			primary_command_buffer_(primary_command_buffer), render_pass_begin_info_(render_pass_begin_info), command_pools_(command_pools),
			gpu_system_(gpu_system) {}

		template<
			graphic_render_command RenderCommandType,
			command_generator<RenderCommandType> CommandGenerator>
		void push(const soul_size count, CommandGenerator&& generator)
		{
			if (count > SECONDARY_COMMAND_BUFFER_THRESHOLD)
			{
				primary_command_buffer_.begin_render_pass(render_pass_begin_info_, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
				const uint32 thread_count = runtime::get_thread_count();

				Array<impl::SecondaryCommandBuffer> secondary_command_buffers;
				secondary_command_buffers.resize(thread_count);

				struct TaskData
				{
					Array<impl::SecondaryCommandBuffer>& cmdBuffers;
					soul_size commandCount;
					VkRenderPass renderPass;
					VkFramebuffer framebuffer;
					impl::CommandPools& commandPools;
					gpu::System& gpuSystem;
				};
				const TaskData task_data = {
					secondary_command_buffers,
					count,
					render_pass_begin_info_.renderPass,
					render_pass_begin_info_.framebuffer,
					command_pools_,
					gpu_system_
				};

				runtime::TaskID task_id = runtime::parallel_for_task_create(
					runtime::TaskID::ROOT(), thread_count, 1,
					[&task_data, &generator](int index)
					{
						auto&& command_buffers = task_data.cmdBuffers;
						const auto command_count = task_data.commandCount;
						impl::SecondaryCommandBuffer command_buffer = task_data.commandPools.request_secondary_command_buffer(task_data.renderPass, 0, task_data.framebuffer);
						const uint32 div = command_count / command_buffers.size();
						const uint32 mod = command_count % command_buffers.size();

						impl::RenderCompiler render_compiler(task_data.gpuSystem, command_buffer.get_vk_handle());
						if (soul::cast<uint32>(index) < mod)
						{
							soul_size start = index * (div + 1);

							for (soul_size i = 0; i < div + 1; i++)
							{
								render_compiler.compile_command(generator(start + i));
							}
						}
						else
						{
							soul_size start = mod * (div + 1) + (index - mod) * div;
							for (soul_size i = 0; i < div; i++)
							{
								render_compiler.compile_command(generator(start + i));
							}
						}
						command_buffer.end();
						command_buffers[index] = command_buffer;

					});
				runtime::run_task(task_id);
				runtime::wait_task(task_id);
				primary_command_buffer_.execute_secondary_command_buffers(soul::cast<uint32>(secondary_command_buffers.size()), secondary_command_buffers.data());
				primary_command_buffer_.end_render_pass();
			}
			else
			{
				primary_command_buffer_.begin_render_pass(render_pass_begin_info_, VK_SUBPASS_CONTENTS_INLINE);
				impl::RenderCompiler render_compiler(gpu_system_, primary_command_buffer_.get_vk_handle());
				for (soul_size command_idx = 0; command_idx < count; command_idx++)
				{
					render_compiler.compile_command(generator(command_idx));
				}
				primary_command_buffer_.end_render_pass();
			}
			
		}

		template <graphic_render_command RenderCommandType>
		void push(soul_size count, const RenderCommandType* render_commands)
		{
			push<RenderCommandType>(count, [render_commands](soul_size index)
			{
				return *(render_commands + index);
			});
		}

		template<graphic_render_command RenderCommandType>
		void push(const RenderCommandType& render_command)
		{
			push(1, &render_command);
		}
	};

	class ComputeCommandList
	{
	private:
		impl::RenderCompiler& render_compiler_;
	public:
		explicit ComputeCommandList(impl::RenderCompiler& render_compiler) : render_compiler_(render_compiler) {}

		void push(const RenderCommandDispatch& command)
		{
			render_compiler_.compile_command(command);
		}
	};

	class CopyCommandList
	{
		impl::RenderCompiler& render_compiler_;
	public:
		explicit CopyCommandList(impl::RenderCompiler& render_compiler) : render_compiler_(render_compiler) {}
		void push(const RenderCommandCopyTexture& command)
		{
			render_compiler_.compile_command(command);
		}
	};

}