#pragma once

#include "core/compiler.h"
#include "core/type.h"

#include "gpu/impl/vulkan/type.h"

namespace soul::gpu
{
  SOUL_ALWAYS_INLINE auto vk_cast(const PolygonMode polygon_mode) -> VkPolygonMode
  {
    constexpr auto POLYGON_MODE_MAP = FlagMap<PolygonMode, VkPolygonMode>::FromValues(
      {VK_POLYGON_MODE_FILL, VK_POLYGON_MODE_LINE, VK_POLYGON_MODE_POINT});
    return POLYGON_MODE_MAP[polygon_mode];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const CullModeFlags flags) -> VkCullModeFlags
  {
    return flags.map<VkCullModeFlags>({VK_CULL_MODE_FRONT_BIT, VK_CULL_MODE_BACK_BIT});
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const FrontFace front_face) -> VkFrontFace
  {
    constexpr auto FRONT_FACE_MAP = FlagMap<FrontFace, VkFrontFace>::FromValues(
      {VK_FRONT_FACE_CLOCKWISE, VK_FRONT_FACE_COUNTER_CLOCKWISE});

    return FRONT_FACE_MAP[front_face];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const CompareOp compare_op) -> VkCompareOp
  {
    constexpr auto COMPARE_OP_MAP = FlagMap<CompareOp, VkCompareOp>::FromValues({
      VK_COMPARE_OP_NEVER,
      VK_COMPARE_OP_LESS,
      VK_COMPARE_OP_EQUAL,
      VK_COMPARE_OP_LESS_OR_EQUAL,
      VK_COMPARE_OP_GREATER,
      VK_COMPARE_OP_NOT_EQUAL,
      VK_COMPARE_OP_GREATER_OR_EQUAL,
      VK_COMPARE_OP_ALWAYS,
    });

    return COMPARE_OP_MAP[compare_op];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const TextureLayout layout) -> VkImageLayout
  {
    constexpr auto IMAGE_LAYOUT_MAP = FlagMap<TextureLayout, VkImageLayout>::FromValues({
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    });
    return IMAGE_LAYOUT_MAP[layout];
  }

  static constexpr auto FORMAT_MAP = FlagMap<TextureFormat, VkFormat>::FromValues({
    // 8-bits per element
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8_SNORM,
    VK_FORMAT_R8_UINT,
    VK_FORMAT_R8_SINT,
    VK_FORMAT_S8_UINT,

    // 16-bits per element
    VK_FORMAT_R16_SFLOAT,
    VK_FORMAT_R16_UINT,
    VK_FORMAT_R16_SINT,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8_SNORM,
    VK_FORMAT_R8G8_UINT,
    VK_FORMAT_R8G8_SINT,
    VK_FORMAT_R5G6B5_UNORM_PACK16,
    VK_FORMAT_R5G5B5A1_UNORM_PACK16,
    VK_FORMAT_R4G4B4A4_UNORM_PACK16,
    VK_FORMAT_D16_UNORM,

    // 24-bits per element
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_R8G8B8A8_SNORM,
    VK_FORMAT_R8G8B8A8_UINT,
    VK_FORMAT_R8G8B8A8_SINT,
    VK_FORMAT_UNDEFINED,

    // 32-bits per element
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R32_UINT,
    VK_FORMAT_R32_SINT,
    VK_FORMAT_R16G16_SFLOAT,
    VK_FORMAT_R16G16_UINT,
    VK_FORMAT_R16G16_SINT,
    VK_FORMAT_B10G11R11_UFLOAT_PACK32,
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_R8G8B8A8_SNORM,
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    VK_FORMAT_R8G8B8A8_UINT,
    VK_FORMAT_R8G8B8A8_SINT,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,

    // 48-bits per element
    VK_FORMAT_R16G16B16_SFLOAT,
    VK_FORMAT_R16G16B16_UINT,
    VK_FORMAT_R16G16B16_SINT,

    // 64-bits per element
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32_UINT,
    VK_FORMAT_R32G32_SINT,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_R16G16B16A16_UINT,
    VK_FORMAT_R16G16B16A16_SINT,

    // 96-bits per element
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32_SINT,

    // 128-bits per element
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,

  });

  SOUL_ALWAYS_INLINE auto vk_cast(const TextureFormat format) -> VkFormat
  {
    return FORMAT_MAP[format];
  }

  static constexpr auto IMAGE_TYPE_MAP = FlagMap<TextureType, VkImageType>::FromValues({
    VK_IMAGE_TYPE_1D,
    VK_IMAGE_TYPE_2D,
    VK_IMAGE_TYPE_2D,
    VK_IMAGE_TYPE_3D,
    VK_IMAGE_TYPE_2D,
  });

  SOUL_ALWAYS_INLINE auto vk_cast(const TextureType type) -> VkImageType
  {
    return IMAGE_TYPE_MAP[type];
  }

  SOUL_ALWAYS_INLINE auto vk_cast_to_image_view_type(const TextureType type) -> VkImageViewType
  {
    static constexpr auto IMAGE_VIEW_TYPE_MAP = FlagMap<TextureType, VkImageViewType>::FromValues({
      VK_IMAGE_VIEW_TYPE_1D,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_VIEW_TYPE_2D_ARRAY,
      VK_IMAGE_VIEW_TYPE_3D,
      VK_IMAGE_VIEW_TYPE_CUBE,
    });
    return IMAGE_VIEW_TYPE_MAP[type];
  }

  SOUL_ALWAYS_INLINE auto vk_cast_format_to_aspect_flags(const TextureFormat format)
    -> VkImageAspectFlags
  {
    switch (format)
    {
    case TextureFormat::DEPTH24_STENCIL8:
    case TextureFormat::DEPTH32F_STENCIL8:
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    case TextureFormat::DEPTH16:
    case TextureFormat::DEPTH24:
    case TextureFormat::DEPTH32F: return VK_IMAGE_ASPECT_DEPTH_BIT;
    case TextureFormat::STENCIL8: return VK_IMAGE_ASPECT_STENCIL_BIT;
    default: return VK_IMAGE_ASPECT_COLOR_BIT;
    }
  }

  static constexpr auto FILTER_MAP =
    FlagMap<TextureFilter, VkFilter>::FromValues({VK_FILTER_NEAREST, VK_FILTER_LINEAR});

  SOUL_ALWAYS_INLINE auto vk_cast(const TextureFilter filter) -> VkFilter
  {
    return FILTER_MAP[filter];
  }

  static constexpr auto MIPMAP_FILTER_MAP = FlagMap<TextureFilter, VkSamplerMipmapMode>::FromValues(
    {VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR});

  SOUL_ALWAYS_INLINE auto vk_cast_mipmap_filter(const TextureFilter filter) -> VkSamplerMipmapMode
  {
    return MIPMAP_FILTER_MAP[filter];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const TextureWrap wrap) -> VkSamplerAddressMode
  {
    static constexpr auto MAPPING = FlagMap<TextureWrap, VkSamplerAddressMode>::FromValues({
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    });
    return MAPPING[wrap];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const BlendFactor blend_factor) -> VkBlendFactor
  {
    static constexpr auto MAPPING = FlagMap<BlendFactor, VkBlendFactor>::FromValues({
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
      VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
    });
    return MAPPING[blend_factor];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const BlendOp blend_op) -> VkBlendOp
  {
    static constexpr auto MAPPING = FlagMap<BlendOp, VkBlendOp>::FromValues({
      VK_BLEND_OP_ADD,
      VK_BLEND_OP_SUBTRACT,
      VK_BLEND_OP_REVERSE_SUBTRACT,
      VK_BLEND_OP_MIN,
      VK_BLEND_OP_MAX,
    });
    return MAPPING[blend_op];
  }

  constexpr auto vk_cast(const TextureUsageFlags usage_flags) -> VkImageUsageFlags
  {
    return usage_flags.map<VkImageUsageFlags>({
      VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_USAGE_STORAGE_BIT,
    });
  }

  constexpr auto vk_cast(const BufferUsageFlags usage_flags) -> VkBufferUsageFlags
  {
    auto result = usage_flags.map<VkBufferUsageFlags>({
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
    });
    constexpr VkBufferUsageFlags need_buffer_device_address_bit =
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    if ((result & need_buffer_device_address_bit) != 0u)
    {
      result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    }
    return result;
  }

  constexpr auto cast_to_pipeline_stage_flags(const ShaderStageFlags stage_flags)
    -> PipelineStageFlags
  {
    return stage_flags.map<PipelineStageFlags>({
      {PipelineStage::VERTEX_SHADER},
      {PipelineStage::GEOMETRY_SHADER},
      {PipelineStage::FRAGMENT_SHADER},
      {PipelineStage::COMPUTE_SHADER},
      {PipelineStage::RAY_TRACING_SHADER},
      {PipelineStage::RAY_TRACING_SHADER},
      {PipelineStage::RAY_TRACING_SHADER},
    });
  }

  constexpr auto vk_cast_shader_stage_to_pipeline_stage_flags(const ShaderStageFlags stage_flags)
    -> VkPipelineStageFlags
  {
    return stage_flags.map<VkPipelineStageFlags>({
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    });
  }

  constexpr auto vk_cast(const ShaderStageFlags stage_flags) noexcept -> VkShaderStageFlags
  {
    return stage_flags.map<VkShaderStageFlags>({
      VK_SHADER_STAGE_VERTEX_BIT,
      VK_SHADER_STAGE_GEOMETRY_BIT,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      VK_SHADER_STAGE_COMPUTE_BIT,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR,
      VK_SHADER_STAGE_MISS_BIT_KHR,
      VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    });
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const ShaderStage shader_stage) noexcept -> VkShaderStageFlagBits
  {
    static constexpr auto MAPPING = FlagMap<ShaderStage, VkShaderStageFlagBits>::FromValues({
      VK_SHADER_STAGE_VERTEX_BIT,
      VK_SHADER_STAGE_GEOMETRY_BIT,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      VK_SHADER_STAGE_COMPUTE_BIT,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR,
      VK_SHADER_STAGE_MISS_BIT_KHR,
      VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    });
    return MAPPING[shader_stage];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const VertexElementType type, const VertexElementFlags flags)
    -> VkFormat
  {
    const b8 integer    = (flags & VERTEX_ELEMENT_INTEGER_TARGET) != 0;
    const b8 normalized = (flags & VERTEX_ELEMENT_NORMALIZED) != 0;
    using ElementType   = VertexElementType;
    if (normalized)
    {
      switch (type)
      {
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
      case ElementType::COUNT: SOUL_NOT_IMPLEMENTED(); return VK_FORMAT_UNDEFINED;
      }
    }
    switch (type)
    {
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
    case ElementType::SHORT4:
      return integer ? VK_FORMAT_R16G16B16A16_SINT : VK_FORMAT_R16G16B16A16_SSCALED;
    case ElementType::USHORT4:
      return integer ? VK_FORMAT_R16G16B16A16_UINT : VK_FORMAT_R16G16B16A16_USCALED;
    case ElementType::HALF4: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ElementType::FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case ElementType::COUNT: SOUL_NOT_IMPLEMENTED();
    }
    return VK_FORMAT_UNDEFINED;
  }

  SOUL_ALWAYS_INLINE auto soul_cast(VkSampleCountFlags flags) -> TextureSampleCountFlags
  {
    SOUL_ASSERT(0, util::get_last_one_bit_pos(flags).unwrap() <= VK_SAMPLE_COUNT_64_BIT);
    static constexpr TextureSampleCount MAP[] = {
      TextureSampleCount::COUNT_1,
      TextureSampleCount::COUNT_2,
      TextureSampleCount::COUNT_4,
      TextureSampleCount::COUNT_8,
      TextureSampleCount::COUNT_16,
      TextureSampleCount::COUNT_32,
      TextureSampleCount::COUNT_64,
    };
    TextureSampleCountFlags result;
    util::for_each_one_bit_pos(
      flags,
      [&result](usize bit_position)
      {
        result.set(MAP[bit_position]);
      });
    return result;
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const TextureSampleCount sample_count) -> VkSampleCountFlagBits
  {
    static constexpr auto MAP = FlagMap<TextureSampleCount, VkSampleCountFlagBits>::FromValues({
      VK_SAMPLE_COUNT_1_BIT,
      VK_SAMPLE_COUNT_2_BIT,
      VK_SAMPLE_COUNT_4_BIT,
      VK_SAMPLE_COUNT_8_BIT,
      VK_SAMPLE_COUNT_16_BIT,
      VK_SAMPLE_COUNT_32_BIT,
      VK_SAMPLE_COUNT_64_BIT,
    });
    return MAP[sample_count];
  }

  constexpr auto get_vk_offset_3d(const vec3i32 val) -> VkOffset3D
  {
    return {val.x, val.y, val.z};
  }

  constexpr auto get_vk_extent_3d(const vec3u32 val) -> VkExtent3D
  {
    return {val.x, val.y, val.z};
  }

  constexpr auto get_vk_subresource_layers(
    const TextureSubresourceLayers& subresource_layers,
    const VkImageAspectFlags aspect_flags) -> VkImageSubresourceLayers
  {
    return {
      .aspectMask     = aspect_flags,
      .mipLevel       = subresource_layers.mip_level,
      .baseArrayLayer = subresource_layers.base_array_layer,
      .layerCount     = subresource_layers.layer_count};
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const IndexType index_type) -> VkIndexType
  {
    static constexpr auto MAP =
      FlagMap<IndexType, VkIndexType>::FromValues({VK_INDEX_TYPE_UINT16, VK_INDEX_TYPE_UINT32});
    return MAP[index_type];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const MemoryPropertyFlags flags) -> VkMemoryPropertyFlags
  {
    return flags.map<VkMemoryPropertyFlags>({
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    });
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const RTGeometryType type) -> VkGeometryTypeKHR
  {
    static constexpr auto MAP = FlagMap<RTGeometryType, VkGeometryTypeKHR>::FromValues(
      {VK_GEOMETRY_TYPE_TRIANGLES_KHR, VK_GEOMETRY_TYPE_AABBS_KHR});
    return MAP[type];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const RTGeometryFlags flags) -> VkGeometryFlagsKHR
  {
    return flags.map<VkGeometryFlagsKHR>(
      {VK_GEOMETRY_OPAQUE_BIT_KHR, VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR});
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const PipelineStageFlags flags) -> VkPipelineStageFlags
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

  SOUL_ALWAYS_INLINE auto vk_cast(const AccessFlags flags) -> VkAccessFlags
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

  SOUL_ALWAYS_INLINE auto vk_cast(const RTBuildMode build_mode)
    -> VkBuildAccelerationStructureModeKHR
  {
    static constexpr auto MAP =
      FlagMap<RTBuildMode, VkBuildAccelerationStructureModeKHR>::FromValues({
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR,
      });
    return MAP[build_mode];
  }

  SOUL_ALWAYS_INLINE auto vk_cast(const RTBuildFlags flags) -> VkBuildAccelerationStructureFlagsKHR
  {
    return flags.map<VkBuildAccelerationStructureFlagsKHR>({
      VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
      VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR,
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
      VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR,
    });
  }
} // namespace soul::gpu
