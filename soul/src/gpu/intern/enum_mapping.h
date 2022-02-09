#pragma once
#include "core/type.h"
#include "core/util.h"
#include "gpu/type.h"

namespace soul::gpu
{

	static constexpr EnumArray<ResourceOwner, QueueType> RESOURCE_OWNER_TO_QUEUE_TYPE({
		QueueType::COUNT,
		QueueType::GRAPHIC,
		QueueType::COMPUTE,
		QueueType::TRANSFER,
		QueueType::GRAPHIC
	});

	SOUL_ALWAYS_INLINE VkCompareOp vkCast(CompareOp compareOp) {
		static constexpr EnumArray<CompareOp, VkCompareOp> COMPARE_OP_MAP({
			VK_COMPARE_OP_NEVER,
			VK_COMPARE_OP_LESS,
			VK_COMPARE_OP_EQUAL,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_COMPARE_OP_GREATER,
			VK_COMPARE_OP_NOT_EQUAL,
			VK_COMPARE_OP_GREATER_OR_EQUAL,
			VK_COMPARE_OP_ALWAYS
		});
		
		return COMPARE_OP_MAP[compareOp];
	}

	SOUL_ALWAYS_INLINE VkImageLayout vkCast(TextureLayout layout) {
		static constexpr EnumArray<TextureLayout, VkImageLayout> IMAGE_LAYOUT_MAP({
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

	static constexpr EnumArray<DescriptorType, VkDescriptorType> DESCRIPTOR_TYPE_MAP({
		VK_DESCRIPTOR_TYPE_MAX_ENUM,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	});
	SOUL_ALWAYS_INLINE VkDescriptorType vkCast(DescriptorType type) {
		return DESCRIPTOR_TYPE_MAP[type];
	}

	static constexpr EnumArray<TextureFormat, VkFormat> FORMAT_MAP({
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

		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
	});
	SOUL_ALWAYS_INLINE VkFormat vkCast(TextureFormat format) {
		return FORMAT_MAP[format];
	}

	static constexpr EnumArray<TextureType, VkImageType> IMAGE_TYPE_MAP({
		VK_IMAGE_TYPE_1D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_3D,
		VK_IMAGE_TYPE_2D,
	});
	
	SOUL_ALWAYS_INLINE VkImageType vkCast(TextureType type) {
		return IMAGE_TYPE_MAP[type];
	}

	SOUL_ALWAYS_INLINE VkImageViewType vkCastToImageViewType(TextureType type) {
		static constexpr EnumArray<TextureType, VkImageViewType> IMAGE_VIEW_TYPE_MAP({
			VK_IMAGE_VIEW_TYPE_1D,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			VK_IMAGE_VIEW_TYPE_3D,
			VK_IMAGE_VIEW_TYPE_CUBE,
		});
		return IMAGE_VIEW_TYPE_MAP[type];
	}

	SOUL_ALWAYS_INLINE VkImageAspectFlags vkCastFormatToAspectFlags(TextureFormat format) {
		static constexpr EnumArray<TextureFormat, VkImageAspectFlags> IMAGE_ASPECT_FLAGS_MAP({
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
		});
		return IMAGE_ASPECT_FLAGS_MAP[format];
	}

	static constexpr EnumArray<TextureFilter, VkFilter> FILTER_MAP({
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR
	});
	SOUL_ALWAYS_INLINE VkFilter vkCast(TextureFilter filter) {
		return FILTER_MAP[filter];
	}

	static constexpr EnumArray<TextureFilter, VkSamplerMipmapMode> MIPMAP_FILTER_MAP({
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	});
	
	SOUL_ALWAYS_INLINE VkSamplerMipmapMode vkCastMipmapFilter(TextureFilter filter) {
		return MIPMAP_FILTER_MAP[filter];
	}

	SOUL_ALWAYS_INLINE VkSamplerAddressMode vkCast(TextureWrap wrap) {
		static constexpr EnumArray<TextureWrap, VkSamplerAddressMode> MAPPING({
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
		});
		return MAPPING[wrap];
	}

	SOUL_ALWAYS_INLINE VkBlendFactor vkCast(BlendFactor blendFactor) {
		static constexpr EnumArray<BlendFactor, VkBlendFactor> MAPPING({
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
		return MAPPING[blendFactor];
	}

	SOUL_ALWAYS_INLINE VkBlendOp vkCast(BlendOp blendOp) {
		static constexpr EnumArray<BlendOp, VkBlendOp> MAPPING({
			VK_BLEND_OP_ADD,
			VK_BLEND_OP_SUBTRACT,
			VK_BLEND_OP_REVERSE_SUBTRACT,
			VK_BLEND_OP_MIN,
			VK_BLEND_OP_MAX
		});
		return MAPPING[blendOp];
	}

	constexpr VkImageUsageFlags vkCast(TextureUsageFlags usageFlags) {
		return usageFlags.map<VkImageUsageFlags>({
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_USAGE_STORAGE_BIT
		});
	}

	constexpr VkBufferUsageFlags vkCastBufferUsageFlags(BufferUsageFlags usageFlags) {
		return usageFlags.map<VkBufferUsageFlags>({
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		});
	}

	constexpr VkPipelineStageFlags vkCastShaderStageToPipelineStageFlags(ShaderStageFlags stageFlags) {
		return stageFlags.map<VkPipelineStageFlags>({
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		});
	}

	constexpr VkShaderStageFlags vkCast(ShaderStageFlags stageFlags) noexcept {
		return stageFlags.map<VkShaderStageFlags>({
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
		});
	}

	SOUL_ALWAYS_INLINE VkShaderStageFlags vkCast(ShaderStage stageFlag) noexcept {
		static constexpr EnumArray<ShaderStage, VkShaderStageFlags> MAPPING({
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
		});
		return MAPPING[stageFlag];
	}

	SOUL_ALWAYS_INLINE VkFormat vkCast(VertexElementType type, VertexElementFlags flags) {
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
			default:
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
		default:
			SOUL_NOT_IMPLEMENTED();
		}
		return VK_FORMAT_UNDEFINED;
	}

	SOUL_ALWAYS_INLINE VkSampleCountFlagBits vkCast(TextureSampleCount sampleCount)
	{
		static_assert(to_underlying(TextureSampleCount::COUNT_1) == VK_SAMPLE_COUNT_1_BIT);
		static_assert(to_underlying(TextureSampleCount::COUNT_2) == VK_SAMPLE_COUNT_2_BIT);
		static_assert(to_underlying(TextureSampleCount::COUNT_4) == VK_SAMPLE_COUNT_4_BIT);
		static_assert(to_underlying(TextureSampleCount::COUNT_8) == VK_SAMPLE_COUNT_8_BIT);
		static_assert(to_underlying(TextureSampleCount::COUNT_16) == VK_SAMPLE_COUNT_16_BIT);
		static_assert(to_underlying(TextureSampleCount::COUNT_32) == VK_SAMPLE_COUNT_32_BIT);
		static_assert(to_underlying(TextureSampleCount::COUNT_64) == VK_SAMPLE_COUNT_64_BIT);
		return static_cast<VkSampleCountFlagBits>(to_underlying(sampleCount));
	}

	constexpr VkOffset3D get_vk_offset_3d(const Vec3i32 val)
	{
		return { val.x, val.y, val.z };
	}

	constexpr VkExtent3D get_vk_extent_3d(const Vec3ui32 val)
	{
		return { val.x, val.y, val.z };
	}

	constexpr VkImageSubresourceLayers get_vk_subresource_layers(const TextureSubresourceLayers& subresource_layers, VkImageAspectFlags aspect_flags)
	{
		return {
			.aspectMask = aspect_flags,
			.mipLevel = subresource_layers.mipLevel,
			.baseArrayLayer = subresource_layers.baseArrayLayer,
			.layerCount = subresource_layers.layerCount
		};
	}
}
