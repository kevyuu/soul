#pragma once
#include "core/type.h"
#include "gpu/type.h"

namespace soul::gpu
{
	static auto RESOURCE_OWNER_TO_QUEUE_TYPE = FlagMap<ResourceOwner, QueueType>::build_from_list({
		QueueType::COUNT,
		QueueType::GRAPHIC,
		QueueType::COMPUTE,
		QueueType::TRANSFER,
		QueueType::GRAPHIC
	});

	SOUL_ALWAYS_INLINE VkCompareOp vk_cast(const CompareOp compare_op) {
		auto COMPARE_OP_MAP = FlagMap<CompareOp, VkCompareOp>::build_from_list({
			VK_COMPARE_OP_NEVER,
			VK_COMPARE_OP_LESS,
			VK_COMPARE_OP_EQUAL,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_COMPARE_OP_GREATER,
			VK_COMPARE_OP_NOT_EQUAL,
			VK_COMPARE_OP_GREATER_OR_EQUAL,
			VK_COMPARE_OP_ALWAYS
		});
		
		return COMPARE_OP_MAP[compare_op];
	}

	SOUL_ALWAYS_INLINE VkImageLayout vk_cast(const TextureLayout layout) {
		auto IMAGE_LAYOUT_MAP = FlagMap<TextureLayout, VkImageLayout>::build_from_list({
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		});
		return IMAGE_LAYOUT_MAP[layout];
	}
	

	static auto FORMAT_MAP = FlagMap<TextureFormat, VkFormat>::build_from_list({
		VK_FORMAT_R8_UNORM,

	    VK_FORMAT_D16_UNORM,

		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_UNDEFINED,

		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R8G8B8A8_SRGB,

		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
	});
	SOUL_ALWAYS_INLINE VkFormat vk_cast(const TextureFormat format) {
		return FORMAT_MAP[format];
	}

	static auto IMAGE_TYPE_MAP = FlagMap<TextureType, VkImageType>::build_from_list({
		VK_IMAGE_TYPE_1D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_3D,
		VK_IMAGE_TYPE_2D,
	});
	
	SOUL_ALWAYS_INLINE VkImageType vk_cast(const TextureType type) {
		return IMAGE_TYPE_MAP[type];
	}

	SOUL_ALWAYS_INLINE VkImageViewType vk_cast_to_image_view_type(const TextureType type) {
		static constexpr auto IMAGE_VIEW_TYPE_MAP = FlagMap<TextureType, VkImageViewType>::build_from_list({
			VK_IMAGE_VIEW_TYPE_1D,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			VK_IMAGE_VIEW_TYPE_3D,
			VK_IMAGE_VIEW_TYPE_CUBE,
		});
		return IMAGE_VIEW_TYPE_MAP[type];
	}

	SOUL_ALWAYS_INLINE VkImageAspectFlags vk_cast_format_to_aspect_flags(const TextureFormat format) {
		static constexpr auto IMAGE_ASPECT_FLAGS_MAP = FlagMap<TextureFormat, VkImageAspectFlags>::build_from_list({
			VK_IMAGE_ASPECT_COLOR_BIT,

		    VK_IMAGE_ASPECT_DEPTH_BIT,

			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,

			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,

			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
		});
		return IMAGE_ASPECT_FLAGS_MAP[format];
	}

	static auto FILTER_MAP = FlagMap<TextureFilter, VkFilter>::build_from_list({
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR
	});
	SOUL_ALWAYS_INLINE VkFilter vk_cast(const TextureFilter filter) {
		return FILTER_MAP[filter];
	}

	static auto MIPMAP_FILTER_MAP = FlagMap<TextureFilter, VkSamplerMipmapMode>::build_from_list({
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	});
	
	SOUL_ALWAYS_INLINE VkSamplerMipmapMode vk_cast_mipmap_filter(const TextureFilter filter) {
		return MIPMAP_FILTER_MAP[filter];
	}

	SOUL_ALWAYS_INLINE VkSamplerAddressMode vk_cast(const TextureWrap wrap) {
		static constexpr auto MAPPING = FlagMap<TextureWrap, VkSamplerAddressMode>::build_from_list({
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
		});
		return MAPPING[wrap];
	}

	SOUL_ALWAYS_INLINE VkBlendFactor vk_cast(const BlendFactor blend_factor) {
		static constexpr auto MAPPING = FlagMap<BlendFactor, VkBlendFactor>::build_from_list({
			VK_BLEND_FACTOR_ZERO,
			VK_BLEND_FACTOR_ONE,
			VK_BLEND_FACTOR_SRC_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
			VK_BLEND_FACTOR_DST_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_DST_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
			VK_BLEND_FACTOR_CONSTANT_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
			VK_BLEND_FACTOR_CONSTANT_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
			VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
			VK_BLEND_FACTOR_SRC1_COLOR,
			VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
			VK_BLEND_FACTOR_SRC1_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
		});
		return MAPPING[blend_factor];
	}

	SOUL_ALWAYS_INLINE VkBlendOp vk_cast(const BlendOp blend_op) {
		static constexpr auto MAPPING = FlagMap<BlendOp, VkBlendOp>::build_from_list({
			VK_BLEND_OP_ADD,
			VK_BLEND_OP_SUBTRACT,
			VK_BLEND_OP_REVERSE_SUBTRACT,
			VK_BLEND_OP_MIN,
			VK_BLEND_OP_MAX
		});
		return MAPPING[blend_op];
	}

	constexpr VkImageUsageFlags vk_cast(const TextureUsageFlags usage_flags) {
		return usage_flags.map<VkImageUsageFlags>({
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_USAGE_STORAGE_BIT
		});
	}

	constexpr VkBufferUsageFlags vk_cast(const BufferUsageFlags usage_flags) {
		return usage_flags.map<VkBufferUsageFlags>({
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		});
	}

	constexpr PipelineStageFlags cast_to_pipeline_stage_flags(const ShaderStageFlags stage_flags)
	{
		return stage_flags.map<PipelineStageFlags>({
			{PipelineStage::VERTEX_SHADER},
			{PipelineStage::GEOMETRY_SHADER},
			{PipelineStage::FRAGMENT_SHADER},
			{PipelineStage::COMPUTE_SHADER}
		});
	}

	constexpr VkPipelineStageFlags vk_cast_shader_stage_to_pipeline_stage_flags(const ShaderStageFlags stage_flags) {
		return stage_flags.map<VkPipelineStageFlags>({
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		});
	}

	constexpr VkShaderStageFlags vk_cast(const ShaderStageFlags stage_flags) noexcept {
		return stage_flags.map<VkShaderStageFlags>({
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
		});
	}

	SOUL_ALWAYS_INLINE VkShaderStageFlagBits vk_cast(const ShaderStage shader_stage) noexcept {
		static constexpr auto MAPPING = FlagMap<ShaderStage, VkShaderStageFlagBits>::build_from_list({
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
		});
		return MAPPING[shader_stage];
	}

	SOUL_ALWAYS_INLINE VkFormat vk_cast(const VertexElementType type, const VertexElementFlags flags) {
		const bool integer = flags & VERTEX_ELEMENT_INTEGER_TARGET;
		const bool normalized =flags & VERTEX_ELEMENT_NORMALIZED;
		using ElementType = VertexElementType;
		if (normalized) {
			switch (type) {
			// Single Component Types
			case ElementType::BYTE: return VK_FORMAT_R8_SNORM;
			case ElementType::UBYTE: return VK_FORMAT_R8_UNORM;
			case ElementType::SHORT: return VK_FORMAT_R16_SNORM;
			case ElementType::USHORT: return VK_FORMAT_R16_UNORM;
			// Two Component Types
			case ElementType::BYTE2: return VK_FORMAT_R8G8_SNORM;
			case ElementType::UBYTE2: return VK_FORMAT_R8G8_UNORM;
			case ElementType::SHORT2: return VK_FORMAT_R16G16_SNORM;
			case ElementType::USHORT2: return VK_FORMAT_R16G16_UNORM;
			// Three Component Types
			case ElementType::BYTE3: return VK_FORMAT_R8G8B8_SNORM;
			case ElementType::UBYTE3: return VK_FORMAT_R8G8B8_UNORM;
			case ElementType::SHORT3: return VK_FORMAT_R16G16B16_SNORM;
			case ElementType::USHORT3: return VK_FORMAT_R16G16B16_UNORM;
			// Four Component Types
			case ElementType::BYTE4: return VK_FORMAT_R8G8B8A8_SNORM;
			case ElementType::UBYTE4: return VK_FORMAT_R8G8B8A8_UNORM;
			case ElementType::SHORT4: return VK_FORMAT_R16G16B16A16_SNORM;
			case ElementType::USHORT4: return VK_FORMAT_R16G16B16A16_UNORM;
			case ElementType::INT:
			case ElementType::UINT:
			case ElementType::FLOAT:
			case ElementType::FLOAT2:
			case ElementType::FLOAT3:
			case ElementType::FLOAT4:
			case ElementType::HALF:
			case ElementType::HALF2:
			case ElementType::HALF3:
			case ElementType::HALF4:
			case ElementType::COUNT:
				SOUL_NOT_IMPLEMENTED();
				return VK_FORMAT_UNDEFINED;
			}
		}
		switch (type) {
		// Single Component Types
		case ElementType::BYTE: return integer ? VK_FORMAT_R8_SINT : VK_FORMAT_R8_SSCALED;
		case ElementType::UBYTE: return integer ? VK_FORMAT_R8_UINT : VK_FORMAT_R8_USCALED;
		case ElementType::SHORT: return integer ? VK_FORMAT_R16_SINT : VK_FORMAT_R16_SSCALED;
		case ElementType::USHORT: return integer ? VK_FORMAT_R16_UINT : VK_FORMAT_R16_USCALED;
		case ElementType::HALF: return VK_FORMAT_R16_SFLOAT;
		case ElementType::INT: return VK_FORMAT_R32_SINT;
		case ElementType::UINT: return VK_FORMAT_R32_UINT;
		case ElementType::FLOAT: return VK_FORMAT_R32_SFLOAT;
		// Two Component Types
		case ElementType::BYTE2: return integer ? VK_FORMAT_R8G8_SINT : VK_FORMAT_R8G8_SSCALED;
		case ElementType::UBYTE2: return integer ? VK_FORMAT_R8G8_UINT : VK_FORMAT_R8G8_USCALED;
		case ElementType::SHORT2: return integer ? VK_FORMAT_R16G16_SINT : VK_FORMAT_R16G16_SSCALED;
		case ElementType::USHORT2: return integer ? VK_FORMAT_R16G16_UINT : VK_FORMAT_R16G16_USCALED;
		case ElementType::HALF2: return VK_FORMAT_R16G16_SFLOAT;
		case ElementType::FLOAT2: return VK_FORMAT_R32G32_SFLOAT;
		// Three Component Types
		case ElementType::BYTE3: return VK_FORMAT_R8G8B8_SINT;
		case ElementType::UBYTE3: return VK_FORMAT_R8G8B8_UINT;
		case ElementType::SHORT3: return VK_FORMAT_R16G16B16_SINT;
		case ElementType::USHORT3: return VK_FORMAT_R16G16B16_UINT;
		case ElementType::HALF3: return VK_FORMAT_R16G16B16_SFLOAT;
		case ElementType::FLOAT3: return VK_FORMAT_R32G32B32_SFLOAT;
		// Four Component Types
		case ElementType::BYTE4: return integer ? VK_FORMAT_R8G8B8A8_SINT : VK_FORMAT_R8G8B8A8_SSCALED;
		case ElementType::UBYTE4: return integer ? VK_FORMAT_R8G8B8A8_UINT : VK_FORMAT_R8G8B8A8_USCALED;
		case ElementType::SHORT4: return integer ? VK_FORMAT_R16G16B16A16_SINT : VK_FORMAT_R16G16B16A16_SSCALED;
		case ElementType::USHORT4: return integer ? VK_FORMAT_R16G16B16A16_UINT : VK_FORMAT_R16G16B16A16_USCALED;
		case ElementType::HALF4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ElementType::FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case ElementType::COUNT:
			SOUL_NOT_IMPLEMENTED();
		}
		return VK_FORMAT_UNDEFINED;
	}

	SOUL_ALWAYS_INLINE TextureSampleCountFlags soul_cast(VkSampleCountFlags flags)
	{
		SOUL_ASSERT(0, util::get_last_one_bit_pos(flags) <= VK_SAMPLE_COUNT_64_BIT , "");
		static constexpr TextureSampleCount MAP[] = {
			TextureSampleCount::COUNT_1,
			TextureSampleCount::COUNT_2,
			TextureSampleCount::COUNT_4,
			TextureSampleCount::COUNT_8,
			TextureSampleCount::COUNT_16,
			TextureSampleCount::COUNT_32,
			TextureSampleCount::COUNT_64
		};
		TextureSampleCountFlags result;
		util::for_each_one_bit_pos(flags, [&result](soul_size bit_position)
		{
			result.set(MAP[bit_position]);
		});
		return result;
	    
	}

	SOUL_ALWAYS_INLINE VkSampleCountFlagBits vk_cast(const TextureSampleCount sample_count)
	{
		static constexpr auto MAP = FlagMap<TextureSampleCount, VkSampleCountFlagBits>::build_from_list({
			VK_SAMPLE_COUNT_1_BIT,
			VK_SAMPLE_COUNT_2_BIT,
			VK_SAMPLE_COUNT_4_BIT,
			VK_SAMPLE_COUNT_8_BIT,
			VK_SAMPLE_COUNT_16_BIT,
			VK_SAMPLE_COUNT_32_BIT,
			VK_SAMPLE_COUNT_64_BIT
		});
		return MAP[sample_count];
	}

	constexpr VkOffset3D get_vk_offset_3d(const vec3i32 val)
	{
		return { val.x, val.y, val.z };
	}

	constexpr VkExtent3D get_vk_extent_3d(const vec3ui32 val)
	{
		return { val.x, val.y, val.z };
	}

	constexpr VkImageSubresourceLayers get_vk_subresource_layers(const TextureSubresourceLayers& subresource_layers, const VkImageAspectFlags aspect_flags)
	{
		return {
			.aspectMask = aspect_flags,
			.mipLevel = subresource_layers.mip_level,
			.baseArrayLayer = subresource_layers.base_array_layer,
			.layerCount = subresource_layers.layer_count
		};
	}

	SOUL_ALWAYS_INLINE VkIndexType vk_cast(const IndexType index_type)
	{
		static constexpr auto MAP = FlagMap<IndexType, VkIndexType>::build_from_list({
			VK_INDEX_TYPE_UINT16,
			VK_INDEX_TYPE_UINT32
		});
		return MAP[index_type];
	}

	SOUL_ALWAYS_INLINE VkMemoryPropertyFlags vk_cast(const MemoryPropertyFlags flags)
	{
		return flags.map<VkMemoryPropertyFlags>({
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_MEMORY_PROPERTY_HOST_CACHED_BIT
		});
	}

	SOUL_ALWAYS_INLINE VkPipelineStageFlags vk_cast(const PipelineStageFlags flags)
	{
		return flags.map<VkPipelineStageFlags>({
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		    VK_PIPELINE_STAGE_HOST_BIT,
		    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		});
	}

	SOUL_ALWAYS_INLINE VkAccessFlags vk_cast(const AccessFlags flags)
	{
		return flags.map<VkAccessFlags>({
			VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
		    VK_ACCESS_INDEX_READ_BIT,
		    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
		    VK_ACCESS_UNIFORM_READ_BIT,
		    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		    VK_ACCESS_SHADER_READ_BIT,
		    VK_ACCESS_SHADER_WRITE_BIT,
		    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		    VK_ACCESS_TRANSFER_READ_BIT,
		    VK_ACCESS_TRANSFER_WRITE_BIT,
		    VK_ACCESS_HOST_READ_BIT,
		    VK_ACCESS_HOST_WRITE_BIT,
		    VK_ACCESS_MEMORY_READ_BIT,
		    VK_ACCESS_MEMORY_WRITE_BIT,
		    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		});
	}

	SOUL_ALWAYS_INLINE VkRect2D vk_cast(const Rect2D area)
	{
		return {
			.offset = {area.offset.x, area.offset.y},
			.extent = {area.extent.x, area.extent.y}
		};
	}
}
