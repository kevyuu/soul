#pragma once
#include "core/util.h"
#include "gpu/data.h"

namespace Soul { namespace GPU {

	static constexpr EnumArray<ResourceOwner, QueueType> RESOURCE_OWNER_TO_QUEUE_TYPE({
		QueueType::COUNT,
		QueueType::GRAPHIC,
		QueueType::COMPUTE,
		QueueType::TRANSFER,
		QueueType::GRAPHIC
	});

	static VkCompareOp vkCast(CompareOp compareOp) {
		static VkCompareOp COMPARE_OP_MAP[] = {
			VK_COMPARE_OP_NEVER,
			VK_COMPARE_OP_LESS,
			VK_COMPARE_OP_EQUAL,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_COMPARE_OP_GREATER,
			VK_COMPARE_OP_NOT_EQUAL,
			VK_COMPARE_OP_GREATER_OR_EQUAL,
			VK_COMPARE_OP_ALWAYS
		};
		static_assert(SOUL_ARRAY_LEN(COMPARE_OP_MAP) == uint64(CompareOp::COUNT));

		return COMPARE_OP_MAP[uint64(compareOp)];
	}

	static VkImageLayout vkCast(TextureLayout layout) {
		static VkImageLayout IMAGE_LAYOUT_MAP[] = {
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
		};
		static_assert(SOUL_ARRAY_LEN(IMAGE_LAYOUT_MAP) == uint64(TextureLayout::COUNT));

		return IMAGE_LAYOUT_MAP[uint64(layout)];
	}

	static VkDescriptorType DESCRIPTOR_TYPE_MAP[] = {
		VK_DESCRIPTOR_TYPE_MAX_ENUM,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	};
	static_assert(SOUL_ARRAY_LEN(DESCRIPTOR_TYPE_MAP) == uint64(DescriptorType::COUNT));

	static VkDescriptorType vkCast(DescriptorType type) {
		return DESCRIPTOR_TYPE_MAP[uint64(type)];
	}

	static VkFormat FORMAT_MAP[] = {
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_UNDEFINED,

		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,

		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT
	};
	static_assert(SOUL_ARRAY_LEN(FORMAT_MAP) == uint64(TextureFormat::COUNT), "");

	static VkFormat vkCast(TextureFormat format) {
		return FORMAT_MAP[uint64(format)];
	}

	static constexpr VkImageType IMAGE_TYPE_MAP[] = {
		VK_IMAGE_TYPE_1D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_3D
	};
	static_assert(SOUL_ARRAY_LEN(IMAGE_TYPE_MAP) == uint64(TextureType::COUNT), "");

	static VkImageType vkCast(TextureType type) {
		return IMAGE_TYPE_MAP[uint64(type)];
	}

	static VkImageViewType vkCastToImageViewType(TextureType type) {
		static VkImageViewType IMAGE_VIEW_TYPE_MAP[] = {
			VK_IMAGE_VIEW_TYPE_1D,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_3D
		};
		static_assert(SOUL_ARRAY_LEN(IMAGE_VIEW_TYPE_MAP) == uint64(TextureType::COUNT), "");
		return IMAGE_VIEW_TYPE_MAP[uint64(type)];
	}

	static VkImageAspectFlags vkCastFormatToAspectFlags(TextureFormat format) {
		static VkImageAspectFlags IMAGE_ASPECT_FLAGS_MAP[] = {
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,

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
			VK_IMAGE_ASPECT_COLOR_BIT
		};
		static_assert(SOUL_ARRAY_LEN(IMAGE_ASPECT_FLAGS_MAP) == uint64(TextureFormat::COUNT), "");
		return IMAGE_ASPECT_FLAGS_MAP[uint64(format)];
	}

	static VkFilter FILTER_MAP[] = {
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR
	};
	static_assert(SOUL_ARRAY_LEN(FILTER_MAP) == uint64(TextureFilter::COUNT), "");

	static VkFilter vkCast(TextureFilter filter) {
		return FILTER_MAP[uint64(filter)];
	}

	static VkSamplerMipmapMode MIPMAP_FILTER_MAP[] = {
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	};
	static_assert(SOUL_ARRAY_LEN(MIPMAP_FILTER_MAP) == uint64(TextureFilter::COUNT), "");

	static VkSamplerMipmapMode vkCastMipmapFilter(TextureFilter filter) {
		return MIPMAP_FILTER_MAP[uint64(filter)];
	}

	static VkSamplerAddressMode WRAP_MAP[] = {
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
	};
	static_assert(SOUL_ARRAY_LEN(WRAP_MAP) == uint64(TextureWrap::COUNT), "");

	static VkSamplerAddressMode vkCast(TextureWrap wrap) {
		return WRAP_MAP[uint64(wrap)];
	}

	static VkBlendFactor vkCast(BlendFactor blendFactor) {
		static constexpr VkBlendFactor MAPPING[] = {
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
		};
		static_assert(SOUL_ARRAY_LEN(MAPPING) == uint64(BlendFactor::COUNT), "");
		return MAPPING[uint64(blendFactor)];
	}

	static VkBlendOp vkCast(BlendOp blendOp) {
		static constexpr VkBlendOp MAPPING[] = {
			VK_BLEND_OP_ADD,
			VK_BLEND_OP_SUBTRACT,
			VK_BLEND_OP_REVERSE_SUBTRACT,
			VK_BLEND_OP_MIN,
			VK_BLEND_OP_MAX
		};
		static_assert(SOUL_ARRAY_LEN(MAPPING) == uint64(BlendOp::COUNT), "");
		return MAPPING[uint64(blendOp)];
	}

	static VkImageUsageFlags vkCastImageUsageFlags(TextureUsageFlags usageFlags) {
		static VkImageUsageFlags BIT_MAPPING[] = {
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_USAGE_STORAGE_BIT,
		};
		return Util::CastFlags(usageFlags, BIT_MAPPING);
	}

	static VkBufferUsageFlags vkCastBufferUsageFlags(BufferUsageFlags usageFlags) {
		static VkBufferUsageFlags BIT_MAPPING[] = {
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};

		return Util::CastFlags(usageFlags, BIT_MAPPING);

	}

	static VkPipelineStageFlags vkCastShaderStageToPipelineStageFlags(ShaderStageFlags stageFlags) {
		static VkPipelineStageFlags BIT_MAPPING[] = {
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		};
		return Util::CastFlags(stageFlags, BIT_MAPPING);
	}

	static VkShaderStageFlags vkCast(ShaderStageFlags stageFlags) {
		static VkShaderStageFlags BIT_MAPPING[] = {
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
		};
		return Util::CastFlags(stageFlags, BIT_MAPPING);
	}

	static VkShaderStageFlags vkCast(ShaderStage stageFlag) {
		static VkShaderStageFlags MAPPING[] = {
			0,
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			VK_SHADER_STAGE_COMPUTE_BIT,
		};
		return MAPPING[uint64(stageFlag)];
	}

	static VkFormat vkCast(VertexElementType type, VertexElementFlags flags) {
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
			case ElementType::BYTE3: return VK_FORMAT_R8G8B8_SNORM;      // NOT MINSPEC
			case ElementType::UBYTE3: return VK_FORMAT_R8G8B8_UNORM;     // NOT MINSPEC
			case ElementType::SHORT3: return VK_FORMAT_R16G16B16_SNORM;  // NOT MINSPEC
			case ElementType::USHORT3: return VK_FORMAT_R16G16B16_UNORM; // NOT MINSPEC
			// Four Component Types
			case ElementType::BYTE4: return VK_FORMAT_R8G8B8A8_SNORM;
			case ElementType::UBYTE4: return VK_FORMAT_R8G8B8A8_UNORM;
			case ElementType::SHORT4: return VK_FORMAT_R16G16B16A16_SNORM;
			case ElementType::USHORT4: return VK_FORMAT_R16G16B16A16_UNORM;
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
		case ElementType::BYTE3: return VK_FORMAT_R8G8B8_SINT;      // NOT MINSPEC
		case ElementType::UBYTE3: return VK_FORMAT_R8G8B8_UINT;     // NOT MINSPEC
		case ElementType::SHORT3: return VK_FORMAT_R16G16B16_SINT;  // NOT MINSPEC
		case ElementType::USHORT3: return VK_FORMAT_R16G16B16_UINT; // NOT MINSPEC
		case ElementType::HALF3: return VK_FORMAT_R16G16B16_SFLOAT; // NOT MINSPEC
		case ElementType::FLOAT3: return VK_FORMAT_R32G32B32_SFLOAT;
			// Four Component Types
		case ElementType::BYTE4: return integer ? VK_FORMAT_R8G8B8A8_SINT : VK_FORMAT_R8G8B8A8_SSCALED;
		case ElementType::UBYTE4: return integer ? VK_FORMAT_R8G8B8A8_UINT : VK_FORMAT_R8G8B8A8_USCALED;
		case ElementType::SHORT4: return integer ? VK_FORMAT_R16G16B16A16_SINT : VK_FORMAT_R16G16B16A16_SSCALED;
		case ElementType::USHORT4: return integer ? VK_FORMAT_R16G16B16A16_UINT : VK_FORMAT_R16G16B16A16_USCALED;
		case ElementType::HALF4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ElementType::FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		return VK_FORMAT_UNDEFINED;
	}

}}