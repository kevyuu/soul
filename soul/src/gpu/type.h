
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
#include "gpu/intern/bindless_descriptor_allocator.h"

#include "core/type.h"
#include "core/vector.h"
#include "core/sbo_vector.h"
#include "core/flag_map.h"
#include "core/pool.h"
#include "core/cstring.h"
#include "core/uint64_hash_map.h"
#include "core/hash_map.h"
#include "core/flag_set.h"

#include "runtime/runtime.h"
#include "memory/allocators/proxy_allocator.h"

#include <filesystem>


namespace soul::gpu
{
	class System;
	class RenderGraph;

	class WSI
	{
	public:
		virtual [[nodiscard]] VkSurfaceKHR create_vulkan_surface(VkInstance instance) = 0;
		virtual [[nodiscard]] vec2ui32 get_framebuffer_size() const = 0;
		virtual ~WSI() = default;
	};

	using Offset2D = vec2i32;
	using Extent2D = vec2ui32;

    struct Rect2D
	{
		Offset2D offset;
		Extent2D extent;
	};

	struct Viewport
	{
		float x = 0;
		float y = 0;
		float width = 0;
		float height = 0;
	};

	using Offset3D = vec3i32;
	using Extent3D = vec3ui32;

	enum class ErrorKind
	{
		FILE_NOT_FOUND,
		OTHER,
		COUNT
	};

	struct Error
	{
	    explicit Error(const ErrorKind error_kind, const char* message) : error_kind(error_kind), message(message) {}
		ErrorKind error_kind;
	    const char* message;
	};

	enum class IndexType
	{
	    UINT16,
		UINT32,
		COUNT
	};
	
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
	static_assert(VERTEX_ELEMENT_ENUM_END_BIT - 1 <= std::numeric_limits<VertexElementFlags>::max());

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
		R8,

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
			vec4f float32;
			vec4ui32 uint32;
			vec4i32 int32;
			Color() {
				float32 = {};
			}

			Color(vec4f val) : float32(val) {}
			Color(vec4ui32 val) : uint32(val) {}
			Color(vec4i32 val) : int32(val) {}

		} color;

		struct DepthStencil {
			float depth = 0.0f;
			uint32 stencil = 0;
			DepthStencil() = default;
			DepthStencil(const float depth, const uint32 stencil) : depth(depth), stencil(stencil) {}
		} depth_stencil;

		ClearValue() = default;
		ClearValue(vec4f color, float depth, uint32 stencil) : color(color), depth_stencil(depth, stencil) {}
		ClearValue(vec4ui32 color, float depth, uint32 stencil) : color(color), depth_stencil(depth, stencil) {}
		ClearValue(vec4i32 color, float depth, uint32 stencil) : color(color), depth_stencil(depth, stencil) {}

	};

	class SubresourceIndex
	{
	private:
		using value_type = uint32;

		static constexpr value_type LEVEL_MASK = 0xFFFF;
		static constexpr value_type LEVEL_BIT_SHIFT = 0;

		static constexpr value_type LAYER_MASK = 0xFFFF0000;
		static constexpr uint8 LAYER_BIT_SHIFT = 16;

		value_type index_ = 0;

	public:
		constexpr SubresourceIndex() = default;
		constexpr explicit SubresourceIndex(const uint16 level, const uint16 layer) :
	    index_(soul::cast<uint32>(level | soul::cast<uint32>(layer) << LAYER_BIT_SHIFT)) {}

		[[nodiscard]] constexpr uint16 get_level() const
		{
			return soul::cast<uint16>((index_ & LEVEL_MASK) >> LEVEL_BIT_SHIFT);
		}

		[[nodiscard]] constexpr uint16 get_layer() const
		{
			return soul::cast<uint16>((index_ & LAYER_MASK) >> LAYER_BIT_SHIFT);
		}

	};

	struct SubresourceIndexRange
	{
		using value_type = SubresourceIndex;

		SubresourceIndex base;
		uint16 level_count = 1;
		uint16 layer_count = 1;

		class ConstIterator
		{
		private:
			uint16 mip_ = 0;
			uint16 layer_ = 1;
			uint16 mip_end_ = 1;
		public:

			using pointer_type = SubresourceIndex*;
			using value_type = SubresourceIndex;
			using iterator_category = std::input_iterator_tag;
			using difference_type = std::ptrdiff_t;

			ConstIterator() noexcept = default;
			ConstIterator(const uint16 mip, const uint16 layer, const uint16 mip_end) noexcept : mip_(mip), layer_(layer), mip_end_(mip_end) {}

			[[nodiscard]] value_type operator*() const { return SubresourceIndex(mip_, layer_); }

			ConstIterator& operator++()
			{
				mip_ += 1;
				if (mip_ == mip_end_)
				{
					mip_ = 0;
					layer_ += 1;
				}
				return *this;
			}

			[[nodiscard]] ConstIterator operator++(int) { const ConstIterator t{ mip_, layer_, mip_end_ }; this->operator++(); return t; }

			bool operator==(ConstIterator const& rhs) const { return (mip_ == rhs.mip_ && layer_ == rhs.layer_ && mip_end_ == rhs.mip_end_); }
			bool operator!=(ConstIterator const& rhs) const { return (mip_ != rhs.mip_ || layer_ != rhs.layer_ || mip_end_ != rhs.mip_end_); }
		};

		using const_iterator = ConstIterator;

		const_iterator begin() noexcept
	    { return const_iterator{ base.get_level(), base.get_layer(), soul::cast<uint16>(base.get_level() + level_count) }; }
	    const_iterator end() noexcept
	    { return const_iterator{ base.get_level(), soul::cast<uint16>(base.get_layer() + layer_count), soul::cast<uint16>(base.get_level() + level_count) }; }
		[[nodiscard]] const_iterator begin() const noexcept
	    { return const_iterator{ base.get_level(), base.get_layer(), soul::cast<uint16>(base.get_level() + level_count) }; }
		[[nodiscard]] const_iterator end() const noexcept
	    { return const_iterator{ base.get_level(), soul::cast<uint16>(base.get_layer() + layer_count), soul::cast<uint16>(base.get_level() + level_count) }; }
	};

	enum class MemoryProperty
	{
		DEVICE_LOCAL,
		HOST_VISIBLE,
		HOST_COHERENT,
		HOST_CACHED,
		COUNT
	};
	using MemoryPropertyFlags = FlagSet<MemoryProperty>;

	struct MemoryOption
	{
		MemoryPropertyFlags required;
		MemoryPropertyFlags preferred;
	};

	struct BufferRegionCopy
	{
		soul_size src_offset = 0;
		soul_size dst_offset = 0;
		soul_size size = 0;
	};

	struct BufferUpdateDesc
	{
		const void* data = nullptr;
		uint32 region_load_count = 0;
		BufferRegionCopy* region_loads = nullptr;
	};

	struct BufferDesc {
		soul_size size = 0;
		BufferUsageFlags usage_flags;
		QueueFlags  queue_flags = QUEUE_DEFAULT;
		std::optional<MemoryOption> memory_option = std::nullopt;
	};

	struct TextureSubresourceLayers
	{
		uint32 mip_level;
		uint32 base_array_layer;
		uint32 layer_count;
	};

	struct TextureRegionCopy
	{
		TextureSubresourceLayers src_subresource = {};
		Offset3D src_offset;
		TextureSubresourceLayers dst_subresource = {};
		Offset3D dst_offset;
		Extent3D extent;
	};

	struct TextureRegionUpdate
	{
		soul_size buffer_offset = 0;
		uint32 buffer_row_length = 0;
		uint32 buffer_image_height = 0;
		TextureSubresourceLayers subresource = {};
		Offset3D offset;
		Extent3D extent;
	};

	struct TextureLoadDesc
	{
		const void* data = nullptr;
		soul_size data_size = 0;

		uint32 region_count = 0;
		const TextureRegionUpdate* regions = nullptr;

		bool generate_mipmap = false;

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
		vec3ui32 extent;
		uint32 mip_levels = 1;
		uint16 layer_count = 1;
		TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
		TextureUsageFlags usage_flags;
		QueueFlags queue_flags;
		const char* name = nullptr;

		static TextureDesc d2(const char* name, TextureFormat format, uint32 mip_levels, TextureUsageFlags usage_flags, QueueFlags queue_flags, const vec2ui32 dimension, TextureSampleCount sample_count = TextureSampleCount::COUNT_1)
		{
			return {
				.type = TextureType::D2,
				.format = format,
				.extent = vec3ui32(dimension.x, dimension.y, 1),
				.mip_levels =mip_levels,
				.sample_count = sample_count,
				.usage_flags = usage_flags,
				.queue_flags = queue_flags,
				.name = name
			};
		}

		static TextureDesc d3(const char* name, TextureFormat format, uint32 mip_levels, TextureUsageFlags usage_flags, QueueFlags queue_flags, const vec3ui32 dimension)
		{
			return {
				.type = TextureType::D3,
	            .format = format,
	            .extent = dimension,
	            .mip_levels = mip_levels,
	            .usage_flags = usage_flags,
	            .queue_flags = queue_flags,
	            .name = name
			};
		}

		static TextureDesc d2_array(const char* name, TextureFormat format, uint32 mip_levels, TextureUsageFlags usage_flags, QueueFlags queue_flags, const vec2ui32 dimension, uint16 layer_count)
		{
			return {
				.type = TextureType::D2_ARRAY,
				.format = format,
				.extent = vec3ui32(dimension.x, dimension.y, 1),
				.mip_levels = mip_levels,
				.layer_count = layer_count,
				.usage_flags = usage_flags,
				.queue_flags = queue_flags,
				.name = name
			};
		}

		static TextureDesc cube(const char* name, TextureFormat format, uint32 mip_levels, TextureUsageFlags usage_flags, QueueFlags queue_flags, const vec2ui32 dimension)
		{
			return {
				.type = TextureType::CUBE,
				.format = format,
				.extent = vec3ui32(dimension.x, dimension.y, 1),
				.mip_levels = mip_levels,
				.layer_count = 6,
				.usage_flags = usage_flags,
				.queue_flags = queue_flags,
				.name = name
			};
		}

		[[nodiscard]] soul_size get_view_count() const
		{
			return mip_levels * layer_count;
		}

	};

	struct SamplerDesc {
		TextureFilter min_filter = TextureFilter::COUNT;
		TextureFilter mag_filter = TextureFilter::COUNT;
		TextureFilter mipmap_filter = TextureFilter::COUNT;
		TextureWrap wrap_u = TextureWrap::CLAMP_TO_EDGE;
		TextureWrap wrap_v = TextureWrap::CLAMP_TO_EDGE;
		TextureWrap wrap_w = TextureWrap::CLAMP_TO_EDGE;
		bool anisotropy_enable = false;
		float max_anisotropy = 0.0f;
		bool compare_enable = false;
		CompareOp compare_op = CompareOp::COUNT;

		static constexpr SamplerDesc same_filter_wrap(const TextureFilter filter, const TextureWrap wrap, const bool anisotropy_enable = false,
                                                    const float max_anisotropy = 0.0f, const bool compare_enable = false, const CompareOp compare_op = CompareOp::ALWAYS)
		{
			return {
				.min_filter = filter,
				.mag_filter = filter,
				.mipmap_filter = filter,
				.wrap_u = wrap,
				.wrap_v = wrap,
				.wrap_w = wrap,
				.anisotropy_enable = anisotropy_enable,
				.max_anisotropy = max_anisotropy,
				.compare_enable = compare_enable,
				.compare_op = compare_op
			};
		}
	};

	struct ShaderFile
	{
		std::filesystem::path path;
		ShaderFile() = default;
		explicit ShaderFile(std::filesystem::path path) : path(std::move(path)) {}
	};
	struct ShaderString
	{
		soul::CString source;
		ShaderString() = default;
		explicit ShaderString(CString str) : source(std::move(str)) {}
		[[nodiscard]] const char* c_str() const { return source.data(); }
	};
	using ShaderSource = std::variant<ShaderFile, ShaderString>;
	using EntryPoints = FlagMap<ShaderStage, const char*>;

	struct ShaderDefine
	{
		const char* key;
		const char* value;
	};

	struct ProgramDesc {
		std::filesystem::path* search_paths = nullptr;
		soul_size search_path_count = 0;
		ShaderDefine* shader_defines = nullptr;
		soul_size shader_define_count = 0;
		ShaderSource* sources = nullptr;
		soul_size source_count = 0;
		EntryPoints entry_point_names = FlagMap<ShaderStage, const char*>(nullptr);
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
	static_assert(ATTACHMENT_ENUM_END_BIT - 1 < std::numeric_limits<AttachmentFlags>::max());

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

		ProgramID program_id;

		InputLayoutDesc input_layout;

		struct InputBindingDesc {
			uint32 stride = 0;
		} input_bindings[MAX_INPUT_BINDING_PER_SHADER];

		struct InputAttrDesc {
			uint32 binding = 0;
			uint32 offset = 0;
			VertexElementType type = VertexElementType::DEFAULT;
			VertexElementFlags flags = 0;
		} input_attributes[MAX_INPUT_PER_SHADER];

		Viewport viewport;
		Rect2D scissor;

		struct RasterDesc {
			float line_width = 1.0f;
			PolygonMode polygon_mode = PolygonMode::FILL;
			CullMode cull_mode = CullMode::NONE;
			FrontFace front_face = FrontFace::CLOCKWISE;
		} raster;

		uint8 color_attachment_count = 0;
		struct ColorAttachmentDesc {
			bool blend_enable = false;
			bool color_write = true;
			BlendFactor src_color_blend_factor = BlendFactor::ZERO;
			BlendFactor dst_color_blend_factor = BlendFactor::ZERO;
			BlendOp color_blend_op = BlendOp::ADD;
			BlendFactor src_alpha_blend_factor = BlendFactor::ZERO;
			BlendFactor dst_alpha_blend_factor = BlendFactor::ZERO;
			BlendOp alpha_blend_op = BlendOp::ADD;
		};
		ColorAttachmentDesc color_attachments[MAX_COLOR_ATTACHMENT_PER_SHADER];

		struct DepthStencilAttachmentDesc {
			bool depth_test_enable = false;
			bool depth_write_enable = false;
			CompareOp depth_compare_op = CompareOp::NEVER;
		};
		DepthStencilAttachmentDesc depth_stencil_attachment;

		struct DepthBiasDesc
		{
			float constant = 0.0f;
			float slope = 0.0f;
		};
		DepthBiasDesc depth_bias;

		bool operator==(const GraphicPipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(GraphicPipelineStateDesc)) == 0);
		}

		bool operator!=(const GraphicPipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(GraphicPipelineStateDesc)) != 0);
		}
	};


	struct ComputePipelineStateDesc
	{
		ProgramID program_id;

		bool operator==(const ComputePipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(ComputePipelineStateDesc)) == 0);
		}

		bool operator!=(const ComputePipelineStateDesc& other) const {
			return (memcmp(this, &other, sizeof(ComputePipelineStateDesc)) != 0);
		}
	};

	namespace impl
	{

		struct PipelineState {
			VkPipeline vk_handle = VK_NULL_HANDLE;
			VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_MAX_ENUM;
			ProgramID program_id;
		};

		struct ProgramDescriptorBinding {
			uint8 count = 0;
			uint8 attachment_index = 0;
			VkShaderStageFlags shader_stage_flags = 0;
			VkPipelineStageFlags pipeline_stage_flags = 0;
		};

		struct RenderPassKey
		{
			Attachment color_attachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
			Attachment resolve_attachments[MAX_COLOR_ATTACHMENT_PER_SHADER];
			Attachment input_attachments[MAX_INPUT_ATTACHMENT_PER_SHADER];
			Attachment depth_attachment;

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

		enum class SemaphoreState : uint8 {
			INITIAL,
			SUBMITTED,
			PENDING
		};

		struct Swapchain {
			WSI* wsi;
			VkSwapchainKHR vk_handle = VK_NULL_HANDLE;
			VkSurfaceFormatKHR format = {};
			VkExtent2D extent = {};
			uint32 image_count = 0;
			SBOVector<TextureID> textures;
			SBOVector<VkImage> images;
			SBOVector<VkImageView> image_views;
			SBOVector<VkFence> fences;
		};

		struct DescriptorSetLayoutBinding {
			VkDescriptorType descriptor_type;
			uint32 descriptor_count;
			VkShaderStageFlags stage_flags;
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
			uint8 count = 0;
			uint8 attachmentIndex = 0;
		};

		struct ShaderInput {
			VkFormat format = VK_FORMAT_UNDEFINED;
			uint32 offset = 0;
		};

		enum class BufferInternalFlag : uint8
		{
		    TRANSIENT,
			COUNT
		};
		using BufferInternalFlags = FlagSet<BufferInternalFlag>;

		struct Buffer {
			BufferDesc desc;
			VkBuffer vk_handle = VK_NULL_HANDLE;
			VmaAllocation allocation{};
			ResourceOwner owner = ResourceOwner::NONE;
			DescriptorID storage_buffer_gpu_handle = DescriptorID::null();
			BufferInternalFlags internal_flags = {};
			VkMemoryPropertyFlags memory_property_flags = 0;
		};

		struct TextureView
		{
			VkImageView vk_handle = VK_NULL_HANDLE;
			DescriptorID storage_image_gpu_handle = DescriptorID::null();
			DescriptorID sampled_image_gpu_handle = DescriptorID::null();
		};

		struct Texture {
			TextureDesc desc;
			VkImage vk_handle = VK_NULL_HANDLE;
			TextureView view;
			VmaAllocation allocation = VK_NULL_HANDLE;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkSharingMode sharing_mode = {};
			ResourceOwner owner = ResourceOwner::NONE;
			TextureView* views = nullptr;
		};

		struct Shader
		{
			VkShaderModule vk_handle = VK_NULL_HANDLE;
			CString entry_point;
		};

		struct Program {
			VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
			FlagMap<ShaderStage, Shader> shaders;
		};

		struct Semaphore {
			VkSemaphore vk_handle = VK_NULL_HANDLE;
			VkPipelineStageFlags stage_flags = 0;
			SemaphoreState state = SemaphoreState::INITIAL;

			[[nodiscard]] bool is_pending() const { return state == SemaphoreState::PENDING; }
		};

		class CommandQueue {

		public:
			void init(VkDevice device, uint32 family_index, uint32 queue_index);
			void wait(Semaphore* semaphore, VkPipelineStageFlags wait_stages);
			void submit(VkCommandBuffer command_buffer, const Vector<Semaphore*>& semaphores, VkFence fence = VK_NULL_HANDLE);
			void submit(VkCommandBuffer command_buffer, Semaphore* semaphore, VkFence fence = VK_NULL_HANDLE);
			void submit(VkCommandBuffer command_buffer, uint32 semaphore_count = 0, Semaphore* const* semaphores = nullptr, VkFence fence = VK_NULL_HANDLE);
			void flush(uint32 semaphore_count, Semaphore* const* semaphores, VkFence fence);
			void present(const VkPresentInfoKHR& present_info);
			[[nodiscard]] uint32 get_family_index() const { return family_index_; }
		private:
			VkDevice device_ = VK_NULL_HANDLE;
			VkQueue vk_handle_ = VK_NULL_HANDLE;
			uint32 family_index_ = 0;
			Vector<VkSemaphore> wait_semaphores_;
			Vector<VkPipelineStageFlags> wait_stages_;
			Vector<VkCommandBuffer> commands_;
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

		using CommandQueues = FlagMap<QueueType, CommandQueue>;

		class CommandPool {
		public:

			explicit CommandPool(memory::Allocator* allocator = get_default_allocator()) : allocator_initializer_(allocator)
			{
				allocator_initializer_.end();
			}

			void init(VkDevice device, VkCommandBufferLevel level, uint32 queue_family_index);
			void reset();
			VkCommandBuffer request();

		private:
			runtime::AllocatorInitializer allocator_initializer_;
			VkDevice device_ = VK_NULL_HANDLE;
			VkCommandPool vk_handle_ = VK_NULL_HANDLE;
			Vector<VkCommandBuffer> allocated_buffers_;
			VkCommandBufferLevel level_ = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
			uint16 count_ = 0;
		};

		class CommandPools
		{
		public:
			explicit CommandPools(memory::Allocator* allocator = get_default_allocator()) : allocator_(allocator), allocator_initializer_(allocator)
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
			Vector<FlagMap<QueueType, CommandPool>> primary_pools_;
			Vector<CommandPool> secondary_pools_;
		};

		class GPUResourceInitializer
		{
		public:
			void init(VmaAllocator gpu_allocator, CommandPools* command_pools);
			void load(Buffer& buffer, const void* data);
			void load(Texture& texture, const TextureLoadDesc& load_desc);
			void clear(Texture& texture, ClearValue clear_value);
			void generate_mipmap(Texture& texture);
			void flush(CommandQueues& command_queues, System& gpu_system);
			void reset();
		private:

			struct StagingBuffer
			{
				VkBuffer vk_handle;
				VmaAllocation allocation;
			};

			struct alignas(SOUL_CACHELINE_SIZE) ThreadContext
			{
				PrimaryCommandBuffer transfer_command_buffer;
				PrimaryCommandBuffer clear_command_buffer;
				PrimaryCommandBuffer mipmap_gen_command_buffer;
				Vector<StagingBuffer> staging_buffers;
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

			Vector<ThreadContext> thread_contexts_;
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
				FlagMap<QueueType, Vector<VkImageMemoryBarrier>> image_barriers_;
				FlagMap<QueueType, QueueFlags> sync_dst_queues_;

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

			Vector<ThreadContext> thread_contexts_;
		};


		struct FrameContext {

			runtime::AllocatorInitializer allocatorInitializer;
			CommandPools command_pools;
			
			VkFence fence = VK_NULL_HANDLE;
			SemaphoreID image_available_semaphore;
			SemaphoreID render_finished_semaphore;

			uint32 swapchain_index = 0;

			struct Garbages {
				Vector<TextureID> textures;
				Vector<BufferID> buffers;
				Vector<VkRenderPass> renderPasses;
				Vector<VkFramebuffer> frameBuffers;
				Vector<VkPipeline> pipelines;
				Vector<VkEvent> events;
				Vector<SemaphoreID> semaphores;

				struct SwapchainGarbage
				{
					VkSwapchainKHR vk_handle = VK_NULL_HANDLE;
					SBOVector<VkImageView> image_views;
				} swapchain;
			} garbages;

			GPUResourceInitializer gpu_resource_initializer;
			GPUResourceFinalizer gpu_resource_finalizer;

			explicit FrameContext(memory::Allocator* allocator) : allocatorInitializer(allocator)
			{
				allocatorInitializer.end();
			}
		};

		struct Database {

			using CPUAllocatorProxy = memory::MultiProxy<memory::ProfileProxy, memory::CounterProxy>;
			using CPUAllocator = memory::ProxyAllocator<
				memory::Allocator,
				CPUAllocatorProxy>;
			CPUAllocator cpu_allocator;

			memory::MallocAllocator vulkan_cpu_backing_allocator{ "Vulkan CPU Backing Allocator" };
			using VulkanCPUAllocatorProxy = memory::MultiProxy<memory::MutexProxy, memory::ProfileProxy>;
			using VulkanCPUAllocator = memory::ProxyAllocator<
				memory::MallocAllocator,
				VulkanCPUAllocatorProxy>;
			VulkanCPUAllocator vulkan_cpu_allocator;

			runtime::AllocatorInitializer allocator_initializer;

			VkInstance instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

			VkDevice device = VK_NULL_HANDLE;
			VkPhysicalDevice physical_device = VK_NULL_HANDLE;
			VkPhysicalDeviceProperties physical_device_properties = {};
			VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};
			VkPhysicalDeviceFeatures physical_device_features = {};
			FlagMap<QueueType, uint32> queue_family_indices;

			CommandQueues queues;

			VkSurfaceKHR surface = VK_NULL_HANDLE;
			VkSurfaceCapabilitiesKHR surfaceCaps = {};

			Swapchain swapchain;

			Vector<FrameContext> frame_contexts;
			uint32 frame_counter = 0;
			uint32 current_frame = 0;

			VmaAllocator gpu_allocator = VK_NULL_HANDLE;
			soul::Vector<VmaPool> linear_pools;

			ConcurrentObjectPool<Buffer> buffer_pool;
			ConcurrentObjectPool<Texture> texture_pool;
			ConcurrentObjectPool<Shader> shaders;

			PipelineStateCache pipeline_state_cache;
			
			Pool<Program> programs;

			HashMap<RenderPassKey, VkRenderPass> render_pass_maps;

			Pool<Semaphore> semaphores;

			UInt64HashMap<SamplerID> sampler_map;
			BindlessDescriptorAllocator descriptor_allocator;

			explicit Database(memory::Allocator* backingAllocator) :
				cpu_allocator("GPU System allocator", backingAllocator, CPUAllocatorProxy::Config{ memory::ProfileProxy::Config(), memory::CounterProxy::Config() }),
				vulkan_cpu_allocator("Vulkan allocator", &vulkan_cpu_backing_allocator, VulkanCPUAllocatorProxy::Config{ memory::MutexProxy::Config(), memory::ProfileProxy::Config() }),
				allocator_initializer(&cpu_allocator) {
				allocator_initializer.end();
			}
		};

	}


	// Render Command API
	enum class RenderCommandType : uint8 {
		DRAW,
		DRAW_INDEX,
		COPY_TEXTURE,
		UPDATE_TEXTURE,
		UPDATE_BUFFER,
		COPY_BUFFER,
		DISPATCH,
		CLEAR_COLOR,
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

	struct RenderCommandDraw : RenderCommandTyped<RenderCommandType::DRAW>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::GRAPHIC;
		PipelineStateID pipeline_state_id;
		void* push_constant_data = nullptr;
		uint32 push_constant_size = 0;
		BufferID vertex_buffer_i_ds[soul::gpu::MAX_VERTEX_BINDING];
		uint16 vertex_offsets[MAX_VERTEX_BINDING] = {};
		uint32 vertex_count = 0;
		uint32 instance_count = 0;
		uint32 first_vertex = 0;
		uint32 first_instance = 0;
	};

	struct RenderCommandDrawIndex : RenderCommandTyped<RenderCommandType::DRAW_INDEX> {
		static constexpr QueueType QUEUE_TYPE = QueueType::GRAPHIC;
		PipelineStateID pipeline_state_id;
		void* push_constant_data = nullptr;
		uint32 push_constant_size = 0;
		BufferID vertex_buffer_ids[soul::gpu::MAX_VERTEX_BINDING];
		uint16 vertex_offsets[MAX_VERTEX_BINDING] = {};
		BufferID index_buffer_id;
		soul_size index_offset = 0;
		IndexType index_type = IndexType::UINT16;
		uint32 first_index = 0;
		uint32 index_count = 0;
	};

	struct RenderCommandUpdateTexture : RenderCommandTyped<RenderCommandType::UPDATE_TEXTURE>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::TRANSFER;
		TextureID dst_texture = TextureID::null();
		const void* data = nullptr;
		soul_size data_size = 0;
		uint32 region_count = 0;
		const TextureRegionUpdate* regions = nullptr;
	};

	struct RenderCommandCopyTexture : RenderCommandTyped<RenderCommandType::COPY_TEXTURE>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::TRANSFER;
		TextureID src_texture = TextureID::null();
		TextureID dst_texture = TextureID::null();
		uint32 region_count = 0;
		const TextureRegionCopy* regions = nullptr;
	};

	struct RenderCommandUpdateBuffer : RenderCommandTyped<RenderCommandType::UPDATE_BUFFER>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::TRANSFER;
		BufferID dst_buffer = BufferID::null();
		void* data = nullptr;
		uint32 region_count = 0;
		const BufferRegionCopy* regions = nullptr;
	};

	struct RenderCommandCopyBuffer : RenderCommandTyped<RenderCommandType::COPY_BUFFER>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::TRANSFER;
		BufferID src_buffer = BufferID::null();
		BufferID dst_buffer = BufferID::null();
		uint32 region_count = 0;
		const BufferRegionCopy* regions = nullptr;
	};

	struct RenderCommandDispatch : RenderCommandTyped<RenderCommandType::DISPATCH>
	{
		static constexpr QueueType QUEUE_TYPE = QueueType::COMPUTE;
		PipelineStateID pipeline_state_id;
		void* push_constant_data = nullptr;
		uint32 push_constant_size = 0;
		vec3ui32 group_count;
	};

	template <typename Func, typename RenderCommandType>
	concept command_generator = std::invocable<Func, soul_size> && std::same_as<RenderCommandType, std::invoke_result_t<Func, soul_size>>;

	template <typename T>
	concept render_command = std::derived_from<T, RenderCommand>;

	template <typename T>
	concept graphic_render_command = render_command<T> && T::QUEUE_TYPE == QueueType::GRAPHIC;

	template <typename T>
	concept transfer_render_command = render_command<T> && T::QUEUE_TYPE == QueueType::TRANSFER;

	template <typename T>
	concept compute_render_command = render_command<T> && T::QUEUE_TYPE == QueueType::COMPUTE;

}