#pragma once

#include <vulkan/vulkan_core.h>

#include "core/architecture.h"
#include "core/chunked_sparse_pool.h"
#include "core/flag_map.h"
#include "core/flag_set.h"
#include "core/hash_map.h"
#include "core/log.h"
#include "core/path.h"
#include "core/pool.h"
#include "core/sbo_vector.h"
#include "core/string.h"
#include "core/type.h"
#include "core/uint64_hash_map.h"
#include "core/variant.h"
#include "core/vector.h"

#include "memory/allocators/proxy_allocator.h"
#include "runtime/runtime.h"

#include "gpu/constant.h"
#include "gpu/id.h"
#include "gpu/object_cache.h"

namespace soul::gpu
{
  class System;
  class RenderGraph;
  class WSI;

  using Offset2D = vec2i32;
  using Extent2D = vec2u32;

  struct Rect2D
  {
    Offset2D offset;
    Extent2D extent;

    auto operator==(const Rect2D& other) const -> b8
    {
      return (all(offset == other.offset) && all(extent == other.extent));
    }

    friend void soul_op_hash_combine(auto& hasher, const Rect2D& rect)
    {
      hasher.combine(rect.offset, rect.extent);
    }
  };

  struct Viewport
  {
    f32 x      = 0;
    f32 y      = 0;
    f32 width  = 0;
    f32 height = 0;

    auto operator==(const Viewport& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const Viewport& viewport)
    {
      hasher.combine(viewport.x, viewport.y, viewport.width, viewport.height);
    }
  };

  using Offset3D = vec3i32;
  using Extent3D = vec3u32;

  enum class ErrorKind
  {
    FILE_NOT_FOUND,
    OTHER,
    COUNT
  };

  struct Error
  {
    explicit Error(const ErrorKind error_kind, CompStr message)
        : error_kind(error_kind), message(message)
    {
    }

    ErrorKind error_kind;
    CompStr message;
  };

  enum class IndexType
  {
    UINT16,
    UINT32,
    COUNT
  };

  enum class VertexElementType : uint8_t
  {
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
    VERTEX_ELEMENT_NORMALIZED     = 0x2,
    VERTEX_ELEMENT_ENUM_END_BIT
  };

  using VertexElementFlags = u8;
  static_assert(VERTEX_ELEMENT_ENUM_END_BIT - 1 <= std::numeric_limits<VertexElementFlags>::max());

  enum class PipelineType : u8
  {
    NON_SHADER,
    RASTER,
    COMPUTE,
    RAY_TRACING,
    COUNT
  };

  using PipelineFlags                       = FlagSet<PipelineType>;
  constexpr auto PIPELINE_FLAGS_NON_SHADER  = PipelineFlags{PipelineType::NON_SHADER};
  constexpr auto PIPELINE_FLAGS_RASTER      = PipelineFlags({PipelineType::RASTER});
  constexpr auto PIPELINE_FLAGS_COMPUTE     = PipelineFlags{PipelineType::COMPUTE};
  constexpr auto PIPELINE_FLAGS_RAY_TRACING = PipelineFlags{PipelineType::RAY_TRACING};

  enum class ShaderStage : u8
  {
    VERTEX,
    GEOMETRY,
    FRAGMENT,
    COMPUTE,
    RAYGEN,
    MISS,
    CLOSEST_HIT,
    COUNT
  };

  using ShaderStageFlags = FlagSet<ShaderStage>;
  constexpr ShaderStageFlags SHADER_STAGES_RASTER =
    ShaderStageFlags({ShaderStage::VERTEX, ShaderStage::GEOMETRY, ShaderStage::FRAGMENT});
  constexpr ShaderStageFlags SHADER_STAGES_VERTEX_FRAGMENT =
    ShaderStageFlags({ShaderStage::VERTEX, ShaderStage::FRAGMENT});
  constexpr ShaderStageFlags SHADER_STAGES_RAY_TRACING =
    ShaderStageFlags({ShaderStage::RAYGEN, ShaderStage::MISS, ShaderStage::CLOSEST_HIT});

  template <PipelineFlags pipeline_flags>
  constexpr auto get_all_shader_stages() -> ShaderStageFlags
  {
    ShaderStageFlags result;
    if (pipeline_flags.test(PipelineType::RASTER))
    {
      result |= {ShaderStage::VERTEX, ShaderStage::GEOMETRY, ShaderStage::FRAGMENT};
    }
    if (pipeline_flags.test(PipelineType::COMPUTE))
    {
      result |= {ShaderStage::COMPUTE};
    }
    if (pipeline_flags.test(PipelineType::RAY_TRACING))
    {
      result |= SHADER_STAGES_RAY_TRACING;
    }
    return result;
  };

  enum class ShaderGroupKind : u8
  {
    RAYGEN,
    MISS,
    HIT,
    CALLABLE,
    COUNT
  };

  enum class PipelineStage : u32
  {
    TOP_OF_PIPE,
    DRAW_INDIRECT,
    VERTEX_INPUT,
    VERTEX_SHADER,
    TESSELLATION_CONTROL_SHADER,
    TESSELLATION_EVALUATION_SHADER,
    GEOMETRY_SHADER,
    FRAGMENT_SHADER,
    EARLY_FRAGMENT_TESTS,
    LATE_FRAGMENT_TESTS,
    COLOR_ATTACHMENT_OUTPUT,
    COMPUTE_SHADER,
    TRANSFER,
    BOTTOM_OF_PIPE,
    HOST,
    AS_BUILD,
    RAY_TRACING_SHADER,
    COUNT
  };

  using PipelineStageFlags            = FlagSet<PipelineStage>;
  const auto PIPELINE_STAGE_FLAGS_ALL = ~PipelineStageFlags();

  enum class AccessType : u32
  {
    INDIRECT_COMMAND_READ,
    INDEX_READ,
    VERTEX_ATTRIBUTE_READ,
    UNIFORM_READ,
    INPUT_ATTACHMENT_READ,
    SHADER_READ,
    SHADER_WRITE,
    COLOR_ATTACHMENT_READ,
    COLOR_ATTACHMENT_WRITE,
    DEPTH_STENCIL_ATTACHMENT_READ,
    DEPTH_STENCIL_ATTACHMENT_WRITE,
    TRANSFER_READ,
    TRANSFER_WRITE,
    HOST_READ,
    HOST_WRITE,
    MEMORY_READ,
    MEMORY_WRITE,
    AS_READ,
    AS_WRITE,
    COUNT
  };

  using AccessFlags                        = FlagSet<AccessType>;
  constexpr AccessFlags ACCESS_FLAGS_ALL   = ~AccessFlags();
  constexpr AccessFlags ACCESS_FLAGS_WRITE = {
    AccessType::SHADER_WRITE,
    AccessType::COLOR_ATTACHMENT_WRITE,
    AccessType::DEPTH_STENCIL_ATTACHMENT_WRITE,
    AccessType::TRANSFER_WRITE,
    AccessType::HOST_WRITE,
    AccessType::MEMORY_WRITE,
    AccessType::AS_WRITE,
  };

  enum class QueueType : u8
  {
    GRAPHIC,
    COMPUTE,
    TRANSFER,
    COUNT,
    NONE = COUNT
  };

  using QueueFlags                   = FlagSet<QueueType>;
  constexpr QueueFlags QUEUE_DEFAULT = {
    QueueType::GRAPHIC, QueueType::COMPUTE, QueueType::TRANSFER};

  enum class BufferUsage : u8
  {
    INDEX,
    VERTEX,
    INDIRECT,
    UNIFORM,
    STORAGE,
    TRANSFER_SRC,
    TRANSFER_DST,
    AS_STORAGE,
    AS_BUILD_INPUT,
    AS_SCRATCH_BUFFER,
    SHADER_BINDING_TABLE,
    COUNT
  };

  using BufferUsageFlags = FlagSet<BufferUsage>;

  enum class TextureUsage : u8
  {
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

  enum class TextureType : u8
  {
    D1,
    D2,
    D2_ARRAY,
    D3,
    CUBE,
    COUNT
  };

  enum class TextureFormat : uint16_t
  {
    // 8-bits per element
    R8,
    R8_SNORM,
    R8UI,
    R8I,
    STENCIL8,

    // 16-bits per element
    R16F,
    R16UI,
    R16I,
    RG8,
    RG8_SNORM,
    RG8UI,
    RG8I,
    RGB565,
    RGB5_A1,
    RGBA4,
    DEPTH16,

    // 24-bits per element
    RGB8,
    SRGB8,
    RGB8_SNORM,
    RGB8UI,
    RGB8I,
    DEPTH24,

    // 32-bits per element
    R32F,
    R32UI,
    R32I,
    RG16F,
    RG16UI,
    RG16I,
    R11F_G11F_B10F,
    RGB9_E5,
    RGBA8,
    SRGBA8,
    SBGRA8,
    RGBA8_SNORM,
    RGB10_A2,
    RGBA8UI,
    RGBA8I,
    DEPTH32F,
    DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8,

    // 48-bits per element
    RGB16F,
    RGB16UI,
    RGB16I,

    // 64-bits per element
    RG32F,
    RG32UI,
    RG32I,
    RGBA16F,
    RGBA16UI,
    RGBA16I,

    // 96-bits per element
    RGB32F,
    RGB32UI,
    RGB32I,

    // 128-bits per element
    RGBA32F,
    RGBA32UI,
    RGBA32I,

    COUNT,
  };

  enum class TextureFilter : u8
  {
    NEAREST,
    LINEAR,
    COUNT
  };

  enum class TextureWrap : u8
  {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRROR_CLAMP_TO_EDGE,
    COUNT
  };

  enum class Topology : u8
  {
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    COUNT
  };

  enum class PolygonMode : u8
  {
    FILL,
    LINE,
    POINT,
    COUNT
  };

  enum class CullMode : u8
  {
    FRONT,
    BACK,
    COUNT
  };
  using CullModeFlags = FlagSet<CullMode>;

  enum class FrontFace : u8
  {
    CLOCKWISE,
    COUNTER_CLOCKWISE,
    COUNT
  };

  enum class CompareOp : u8
  {
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

  enum class BlendFactor : u8
  {
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

  enum class BlendOp : u8
  {
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX,
    COUNT
  };

  enum class TextureLayout : u8
  {
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

  struct ClearValue
  {
    union ColorData
    {
      vec4f32 float32;
      vec4u32 uint32;
      vec4i32 int32;

      ColorData() : float32() {}

      explicit ColorData(vec4f32 val) : float32(val) {}

      explicit ColorData(vec4u32 val) : uint32(val) {}

      explicit ColorData(vec4i32 val) : int32(val) {}
    } ColorData;

    struct DepthStencilData
    {
      f32 depth          = 0.0f;
      u32 stencil        = 0;
      DepthStencilData() = default;

      DepthStencilData(const f32 depth, const u32 stencil) : depth(depth), stencil(stencil) {}
    } depth_stencil;

    ClearValue() = default;

    ClearValue(vec4f32 ColorData, f32 depth, u32 stencil)
        : ColorData(ColorData), depth_stencil(depth, stencil)
    {
    }

    ClearValue(vec4u32 ColorData, f32 depth, u32 stencil)
        : ColorData(ColorData), depth_stencil(depth, stencil)
    {
    }

    ClearValue(vec4i32 ColorData, f32 depth, u32 stencil)
        : ColorData(ColorData), depth_stencil(depth, stencil)
    {
    }

    static auto Color(vec4f32 color) -> ClearValue
    {
      return ClearValue(color, 0, 0);
    }
  };

  class SubresourceIndex
  {
  private:
    using value_type = u32;

    static constexpr value_type LEVEL_MASK      = 0xFFFF;
    static constexpr value_type LEVEL_BIT_SHIFT = 0;

    static constexpr value_type LAYER_MASK = 0xFFFF0000;
    static constexpr u8 LAYER_BIT_SHIFT    = 16;

    value_type index_ = 0;

  public:
    constexpr SubresourceIndex() = default;

    constexpr explicit SubresourceIndex(const u16 level, const u16 layer)
        : index_(soul::cast<u32>(level | soul::cast<u32>(layer) << LAYER_BIT_SHIFT))
    {
    }

    [[nodiscard]]
    constexpr auto get_level() const -> u16
    {
      return soul::cast<u16>((index_ & LEVEL_MASK) >> LEVEL_BIT_SHIFT);
    }

    [[nodiscard]]
    constexpr auto get_layer() const -> u16
    {
      return soul::cast<u16>((index_ & LAYER_MASK) >> LAYER_BIT_SHIFT);
    }
  };

  struct SubresourceIndexRange
  {
    using value_type = SubresourceIndex;

    SubresourceIndex base;
    u16 level_count = 1;
    u16 layer_count = 1;

    class ConstIterator
    {
    private:
      u16 mip_     = 0;
      u16 layer_   = 1;
      u16 mip_end_ = 1;

    public:
      using pointer_type      = SubresourceIndex*;
      using value_type        = SubresourceIndex;
      using iterator_category = std::input_iterator_tag;
      using difference_type   = std::ptrdiff_t;

      ConstIterator() noexcept = default;

      ConstIterator(const u16 mip, const u16 layer, const u16 mip_end) noexcept
          : mip_(mip), layer_(layer), mip_end_(mip_end)
      {
      }

      [[nodiscard]]
      auto
      operator*() const -> value_type
      {
        return SubresourceIndex(mip_, layer_);
      }

      auto operator++() -> ConstIterator&
      {
        mip_ += 1;
        if (mip_ == mip_end_)
        {
          mip_ = 0;
          layer_ += 1;
        }
        return *this;
      }

      [[nodiscard]]
      auto
      operator++(int) -> ConstIterator
      {
        const ConstIterator t{mip_, layer_, mip_end_};
        this->operator++();
        return t;
      }

      auto operator==(const ConstIterator& rhs) const -> bool = default;
    };

    using const_iterator = ConstIterator;

    auto begin() noexcept -> const_iterator
    {
      return const_iterator{
        base.get_level(), base.get_layer(), soul::cast<u16>(base.get_level() + level_count)};
    }

    auto end() noexcept -> const_iterator
    {
      return const_iterator{
        base.get_level(),
        soul::cast<u16>(base.get_layer() + layer_count),
        soul::cast<u16>(base.get_level() + level_count)};
    }

    [[nodiscard]]
    auto begin() const noexcept -> const_iterator
    {
      return const_iterator{
        base.get_level(), base.get_layer(), soul::cast<u16>(base.get_level() + level_count)};
    }

    [[nodiscard]]
    auto end() const noexcept -> const_iterator
    {
      return const_iterator{
        base.get_level(),
        soul::cast<u16>(base.get_layer() + layer_count),
        soul::cast<u16>(base.get_level() + level_count)};
    }
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
    usize src_offset = 0;
    usize dst_offset = 0;
    usize size       = 0;
  };

  struct BufferUpdateDesc
  {
    const void* data                           = nullptr;
    Span<const BufferRegionCopy*, u32> regions = nilspan;
  };

  struct BufferDesc
  {
    usize size = 0;
    BufferUsageFlags usage_flags;
    QueueFlags queue_flags             = QUEUE_DEFAULT;
    Option<MemoryOption> memory_option = nilopt;
  };

  struct TextureSubresourceRange
  {
    u32 base_mip_level;
    u32 level_count;
    u32 base_array_layer;
    u32 layer_count;
  };

  struct TextureSubresourceLayers

  {
    u32 mip_level;
    u32 base_array_layer;
    u32 layer_count;
  };

  struct TextureRegionCopy
  {
    TextureSubresourceLayers src_subresource = {};
    Offset3D src_offset;
    TextureSubresourceLayers dst_subresource = {};
    Offset3D dst_offset;
    Extent3D extent;

    static auto Texture2D(vec2u32 extent_xy) -> TextureRegionCopy
    {
      return {
        .src_subresource = {.layer_count = 1},
        .dst_subresource = {.layer_count = 1},
        .extent          = vec3u32(extent_xy, 1),
      };
    }
  };

  struct TextureRegionUpdate
  {
    usize buffer_offset                  = 0;
    u32 buffer_row_length                = 0;
    u32 buffer_image_height              = 0;
    TextureSubresourceLayers subresource = {};
    Offset3D offset;
    Extent3D extent;
  };

  struct TextureLoadDesc
  {
    const void* data = nullptr;
    usize data_size  = 0;

    Span<const TextureRegionUpdate*, u32> regions = nilspan;

    b8 generate_mipmap = false;
  };

  enum class TextureSampleCount : u8
  {
    COUNT_1,
    COUNT_2,
    COUNT_4,
    COUNT_8,
    COUNT_16,
    COUNT_32,
    COUNT_64,
    COUNT
  };

  using TextureSampleCountFlags = FlagSet<TextureSampleCount>;

  struct TextureDesc
  {
    TextureType type     = TextureType::D2;
    TextureFormat format = TextureFormat::COUNT;
    vec3u32 extent;
    u32 mip_levels                  = 1;
    u16 layer_count                 = 1;
    TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
    TextureUsageFlags usage_flags;
    QueueFlags queue_flags;

    static auto d2(
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec2u32 dimension,
      TextureSampleCount sample_count = TextureSampleCount::COUNT_1) -> TextureDesc
    {
      return {
        .type         = TextureType::D2,
        .format       = format,
        .extent       = vec3u32(dimension.x, dimension.y, 1),
        .mip_levels   = mip_levels,
        .sample_count = sample_count,
        .usage_flags  = usage_flags,
        .queue_flags  = queue_flags,
      };
    }

    static auto d3(
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec3u32 dimension) -> TextureDesc
    {
      return {
        .type        = TextureType::D3,
        .format      = format,
        .extent      = dimension,
        .mip_levels  = mip_levels,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
      };
    }

    static auto d2_array(
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec2u32 dimension,
      u16 layer_count) -> TextureDesc
    {
      return {
        .type        = TextureType::D2_ARRAY,
        .format      = format,
        .extent      = vec3u32(dimension.x, dimension.y, 1),
        .mip_levels  = mip_levels,
        .layer_count = layer_count,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
      };
    }

    static auto cube(
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec2u32 dimension) -> TextureDesc
    {
      return {
        .type        = TextureType::CUBE,
        .format      = format,
        .extent      = vec3u32(dimension.x, dimension.y, 1),
        .mip_levels  = mip_levels,
        .layer_count = 6,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
      };
    }

    [[nodiscard]]
    auto get_view_count() const -> usize
    {
      return soul::cast<usize>(mip_levels) * layer_count;
    }
  };

  struct SamplerDesc
  {
    TextureFilter min_filter    = TextureFilter::COUNT;
    TextureFilter mag_filter    = TextureFilter::COUNT;
    TextureFilter mipmap_filter = TextureFilter::COUNT;
    TextureWrap wrap_u          = TextureWrap::CLAMP_TO_EDGE;
    TextureWrap wrap_v          = TextureWrap::CLAMP_TO_EDGE;
    TextureWrap wrap_w          = TextureWrap::CLAMP_TO_EDGE;
    b8 anisotropy_enable        = false;
    f32 max_anisotropy          = 0.0f;
    b8 compare_enable           = false;
    CompareOp compare_op        = CompareOp::COUNT;

    static constexpr auto same_filter_wrap(
      const TextureFilter filter,
      const TextureWrap wrap,
      const b8 anisotropy_enable = false,
      const f32 max_anisotropy   = 0.0f,
      const b8 compare_enable    = false,
      const CompareOp compare_op = CompareOp::ALWAYS) -> SamplerDesc
    {
      return {
        .min_filter        = filter,
        .mag_filter        = filter,
        .mipmap_filter     = filter,
        .wrap_u            = wrap,
        .wrap_v            = wrap,
        .wrap_w            = wrap,
        .anisotropy_enable = anisotropy_enable,
        .max_anisotropy    = max_anisotropy,
        .compare_enable    = compare_enable,
        .compare_op        = compare_op};
    }
  };

  struct DrawIndexedIndirectCommand
  {
    u32 index_count    = 0; ///< number of vertices to draw.
    u32 instance_count = 0; ///< number of instances to draw.
    u32 first_index    = 0; ///< base index within the index_buffer.
    i32 vertex_offset  = 0; ///< value added to the vertex index before indesing into vertex buffer.
    u32 first_instance = 0; ///< instance ID of the first instance to draw.
  };

  struct DispatchIndirectCommand
  {
    u32 x = 0;
    u32 y = 0;
    u32 z = 0;
  };

  enum class RTBuildMode : u8
  {
    REBUILD,
    UPDATE,
    COUNT
  };

  enum class RTBuildFlag : u8
  {
    ALLOW_UPDATE,
    ALLOW_COMPACTION,
    PREFER_FAST_TRACE,
    PREFER_FAST_BUILD,
    LOW_MEMORY,
    COUNT
  };

  using RTBuildFlags = FlagSet<RTBuildFlag>;

  enum class RTGeometryType : u8
  {
    TRIANGLE,
    AABB,
    COUNT
  };

  enum class RTGeometryFlag : u8
  {
    OPAQUE,
    NO_DUPLICATE_ANY_HIT_INVOCATION,
    COUNT
  };

  using RTGeometryFlags = FlagSet<RTGeometryFlag>;

  struct RTTriangleDesc
  {
    TextureFormat vertex_format;
    GPUAddress vertex_data;
    u64 vertex_stride;
    u32 vertex_count;
    IndexType index_type;
    GPUAddress index_data;
    GPUAddress transform_data;
    u32 index_count;
    u32 index_offset;
    u32 first_vertex;
    u32 transform_offset;
  };

  struct RTAABBDesc
  {
    u32 count;
    GPUAddress data;
    uint64_t stride;
  };

  struct RTGeometryDesc
  {
    RTGeometryType type;
    RTGeometryFlags flags;

    union
    {
      RTTriangleDesc triangles;
      RTAABBDesc aabbs;
    } content;
  };

  enum class RTGeometryInstanceFlag : u32
  {
    // The enum values are kept consistent with D3D12_RAYTRACING_INSTANCE_FLAGS
    // and VkGeometryInstanceFlagBitsKHR.
    TRIANGLE_FACING_CULL_DISABLE,
    TRIANGLE_FRONT_COUNTER_CLOCKWISE,
    FORCE_OPAQUE,
    NO_OPAQUE,
    COUNT
  };

  using RTGeometryInstanceFlags = FlagSet<RTGeometryInstanceFlag>;

  struct RTInstanceDesc
  {
    f32 transform[3][4]    = {};
    u32 instance_id   : 24 = {};
    u32 instance_mask : 8  = {};
    u32 sbt_offset    : 24 = {};
    u32 flags         : 8  = {};
    GPUAddress blas_gpu_address;

    RTInstanceDesc() = default;
    RTInstanceDesc(
      mat4f32 in_transform,
      u32 instance_id,
      u32 instance_mask,
      u32 sbt_offset,
      RTGeometryInstanceFlags flags,
      GPUAddress blas_gpu_address);
  };

  struct TlasDesc
  {
    usize size = 0;
  };

  struct BlasDesc
  {
    usize size = 0;
  };

  struct TlasBuildDesc
  {
    RTBuildFlags build_flags;
    RTGeometryFlags geometry_flags;
    GPUAddress instance_data;
    u32 instance_count  = 0;
    u32 instance_offset = 0;
  };

  struct BlasBuildDesc
  {
    RTBuildFlags flags;
    u32 geometry_count;
    const RTGeometryDesc* geometry_descs;
  };

  struct ShaderFile
  {
    Path path;
  };

  struct ShaderString
  {
    String source;
  };

  using ShaderSource = Variant<ShaderFile, ShaderString>;

  struct ShaderEntryPoint
  {
    ShaderStage stage;
    StringView name;
  };

  static constexpr auto ENTRY_POINT_UNUSED = VK_SHADER_UNUSED_KHR;

  struct RTGeneralShaderGroup
  {
    u32 entry_point = ENTRY_POINT_UNUSED;
  };

  struct RTTriangleHitGroup
  {
    u32 any_hit_entry_point      = ENTRY_POINT_UNUSED;
    u32 closest_hit_entry_point  = ENTRY_POINT_UNUSED;
    u32 intersection_entry_point = ENTRY_POINT_UNUSED;
  };

  struct ShaderDefine
  {
    String key;
    String value;
  };

  struct ProgramDesc
  {
    Span<const Path*, u32> search_paths             = nilspan;
    Span<const ShaderDefine*, u32> shader_defines   = nilspan;
    Span<const ShaderSource*, u32> sources          = nilspan;
    Span<const ShaderEntryPoint*, u32> entry_points = nilspan;
  };

  struct ShaderTableDesc
  {
    ProgramID program_id;
    RTGeneralShaderGroup raygen_group;
    Span<const RTGeneralShaderGroup*, u32> miss_groups = nilspan;
    Span<const RTTriangleHitGroup*, u32> hit_groups    = nilspan;
    u32 max_recursion_depth                            = 0;
  };

  enum class RTPipelineFlag : u8
  {
    SKIP_TRIANGLE,
    SKIP_PROCEDURAL_PRIMITIVES,
    COUNT
  };

  using RTPipelineFlags = FlagSet<RTPipelineFlag>;

  using AttachmentFlagBits = enum {
    ATTACHMENT_ACTIVE_BIT     = 0x1,
    ATTACHMENT_FIRST_PASS_BIT = 0x2,
    ATTACHMENT_LAST_PASS_BIT  = 0x4,
    ATTACHMENT_EXTERNAL_BIT   = 0x8,
    ATTACHMENT_CLEAR_BIT      = 0x10,
    ATTACHMENT_ENUM_END_BIT
  };

  using AttachmentFlags = u8;
  static_assert(ATTACHMENT_ENUM_END_BIT - 1 < std::numeric_limits<AttachmentFlags>::max());

  struct Attachment
  {
    TextureFormat format;
    TextureSampleCount sampleCount;
    AttachmentFlags flags;
    auto operator==(const Attachment&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const Attachment& val)
    {
      hasher.combine(val.format);
      hasher.combine(val.sampleCount);
      hasher.combine(val.flags);
    }
  };

  struct InputLayoutDesc
  {
    Topology topology = Topology::TRIANGLE_LIST;

    auto operator==(const InputLayoutDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const InputLayoutDesc& val)
    {
      hasher.combine(val.topology);
    }
  };

  struct InputBindingDesc
  {
    u32 stride = 0;

    auto operator==(const InputBindingDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const InputBindingDesc& desc)
    {
      hasher.combine(desc.stride);
    }
  };

  struct InputAttrDesc
  {
    u32 binding              = 0;
    u32 offset               = 0;
    VertexElementType type   = VertexElementType::DEFAULT;
    VertexElementFlags flags = 0;

    auto operator==(const InputAttrDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const InputAttrDesc& val)
    {
      hasher.combine(val.binding, val.offset, val.type, val.flags);
    }
  };

  struct ColorAttachmentDesc
  {
    b8 blend_enable                    = false;
    b8 color_write                     = true;
    BlendFactor src_color_blend_factor = BlendFactor::ZERO;
    BlendFactor dst_color_blend_factor = BlendFactor::ZERO;
    BlendOp color_blend_op             = BlendOp::ADD;
    BlendFactor src_alpha_blend_factor = BlendFactor::ZERO;
    BlendFactor dst_alpha_blend_factor = BlendFactor::ZERO;
    BlendOp alpha_blend_op             = BlendOp::ADD;

    auto operator==(const ColorAttachmentDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const ColorAttachmentDesc& desc)
    {
      hasher.combine(
        desc.blend_enable,
        desc.color_write,
        desc.src_color_blend_factor,
        desc.dst_color_blend_factor,
        desc.color_blend_op,
        desc.src_alpha_blend_factor,
        desc.dst_alpha_blend_factor,
        desc.alpha_blend_op);
    }
  };

  struct DepthStencilAttachmentDesc
  {
    b8 depth_test_enable       = false;
    b8 depth_write_enable      = false;
    CompareOp depth_compare_op = CompareOp::NEVER;

    auto operator==(const DepthStencilAttachmentDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const DepthStencilAttachmentDesc& desc)
    {
      hasher.combine(desc.depth_test_enable, desc.depth_write_enable, desc.depth_compare_op);
    }
  };

  struct DepthBiasDesc
  {
    f32 constant = 0.0f;
    f32 slope    = 0.0f;

    auto operator==(const DepthBiasDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const DepthBiasDesc& desc)
    {
      hasher.combine(desc.constant, desc.slope);
    }
  };

  struct GraphicPipelineStateDesc
  {
    ProgramID program_id;

    InputLayoutDesc input_layout;

    Array<InputBindingDesc, MAX_INPUT_BINDING_PER_SHADER> input_bindings;

    Array<InputAttrDesc, MAX_INPUT_PER_SHADER> input_attributes;

    Viewport viewport;

    Rect2D scissor = {};

    struct RasterDesc
    {
      f32 line_width           = 1.0f;
      PolygonMode polygon_mode = PolygonMode::FILL;
      CullModeFlags cull_mode  = {};
      FrontFace front_face     = FrontFace::CLOCKWISE;

      auto operator==(const RasterDesc&) const -> bool = default;

      friend void soul_op_hash_combine(auto& hasher, const RasterDesc& desc)
      {
        hasher.combine(desc.line_width, desc.polygon_mode, desc.cull_mode, desc.front_face);
      }

    } raster;

    u8 color_attachment_count = 0;

    Array<ColorAttachmentDesc, MAX_COLOR_ATTACHMENT_PER_SHADER> color_attachments;

    DepthStencilAttachmentDesc depth_stencil_attachment;

    DepthBiasDesc depth_bias;

    auto operator==(const GraphicPipelineStateDesc& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const GraphicPipelineStateDesc& desc)
    {
      hasher.combine(
        desc.program_id,
        desc.input_layout,
        desc.viewport,
        desc.scissor,
        desc.raster,
        desc.color_attachment_count,
        desc.depth_stencil_attachment,
        desc.depth_bias);
      hasher.combine_span(desc.input_bindings.span());
      hasher.combine_span(desc.input_attributes.span());
      hasher.combine_span(desc.color_attachments.span());
    }
  };

  struct ComputePipelineStateDesc
  {
    ProgramID program_id;

    auto operator==(const ComputePipelineStateDesc& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const ComputePipelineStateDesc& val)
    {
      hasher.combine(val.program_id);
    }
  };

  struct RayTracingPipelineStateDesc
  {
    ProgramID program_id;
  };

  struct GPUProperties
  {
    struct GPULimit
    {
      TextureSampleCountFlags color_sample_count_flags;
      TextureSampleCountFlags depth_sample_count_flags;
    } limit;
  };

  // Render Command API
  enum class RenderCommandType : u8
  {
    DRAW,
    DRAW_INDEX,
    DRAW_INDEXED_INDIRECT,
    COPY_TEXTURE,
    UPDATE_TEXTURE,
    CLEAR_TEXTURE,
    UPDATE_BUFFER,
    COPY_BUFFER,
    DISPATCH,
    DISPATCH_INDIRECT,
    CLEAR_COLOR,
    RAY_TRACE,
    BUILD_TLAS,
    BUILD_BLAS,
    BATCH_BUILD_BLAS,
    COUNT
  };

  struct RenderCommand
  {
    RenderCommandType type = RenderCommandType::COUNT;
  };

  template <RenderCommandType RENDER_COMMAND_TYPE>
  struct RenderCommandTyped : RenderCommand
  {
    static const RenderCommandType TYPE = RENDER_COMMAND_TYPE;

    RenderCommandTyped() : RenderCommand(TYPE) {}
  };

  struct RenderCommandDraw : RenderCommandTyped<RenderCommandType::DRAW>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RASTER;
    PipelineStateID pipeline_state_id;
    void* push_constant_data = nullptr;
    u32 push_constant_size   = 0;
    BufferID vertex_buffer_i_ds[MAX_VERTEX_BINDING];
    u16 vertex_offsets[MAX_VERTEX_BINDING] = {};
    u32 vertex_count                       = 0;
    u32 instance_count                     = 0;
    u32 first_vertex                       = 0;
    u32 first_instance                     = 0;
  };

  struct RenderCommandDrawIndex : RenderCommandTyped<RenderCommandType::DRAW_INDEX>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RASTER;
    PipelineStateID pipeline_state_id;
    const void* push_constant_data = nullptr;
    u32 push_constant_size         = 0;
    BufferID vertex_buffer_ids[MAX_VERTEX_BINDING];
    u16 vertex_offsets[MAX_VERTEX_BINDING] = {};
    BufferID index_buffer_id;
    usize index_offset   = 0;
    IndexType index_type = IndexType::UINT16;
    u32 first_index      = 0;
    u32 index_count      = 0;
    u32 instance_count   = 1;
    u32 first_instance   = 0;
  };

  struct RenderCommandDrawIndexedIndirect
      : RenderCommandTyped<RenderCommandType::DRAW_INDEXED_INDIRECT>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RASTER;
    PipelineStateID pipeline_state_id;
    const void* push_constant_data = nullptr;
    u32 push_constant_size         = 0;
    BufferID vertex_buffer_ids[MAX_VERTEX_BINDING];
    u16 vertex_offsets[MAX_VERTEX_BINDING] = {};
    BufferID index_buffer_id;
    usize index_offset   = 0;
    IndexType index_type = IndexType::UINT16;
    BufferID buffer_id;
    u64 offset     = 0;
    u32 draw_count = 0;
    u32 stride     = 0;
  };

  struct RenderCommandUpdateTexture : RenderCommandTyped<RenderCommandType::UPDATE_TEXTURE>
  {
    static constexpr PipelineType PIPELINE_TYPE   = PipelineType::NON_SHADER;
    TextureID dst_texture                         = TextureID::Null();
    const void* data                              = nullptr;
    usize data_size                               = 0;
    Span<const TextureRegionUpdate*, u32> regions = nilspan;
  };

  struct RenderCommandCopyTexture : RenderCommandTyped<RenderCommandType::COPY_TEXTURE>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    TextureID src_texture                       = TextureID::Null();
    TextureID dst_texture                       = TextureID::Null();
    Span<const TextureRegionCopy*, u32> regions = nilspan;
  };

  struct RenderCommandClearTexture : RenderCommandTyped<RenderCommandType::CLEAR_TEXTURE>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    TextureID dst_texture                       = TextureID::Null();
    ClearValue clear_value;
    Option<TextureSubresourceRange> subresource_range;
  };

  struct RenderCommandUpdateBuffer : RenderCommandTyped<RenderCommandType::UPDATE_BUFFER>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    BufferID dst_buffer                         = BufferID::Null();
    void* data                                  = nullptr;
    Span<const BufferRegionCopy*, u32> regions  = nilspan;
  };

  struct RenderCommandCopyBuffer : RenderCommandTyped<RenderCommandType::COPY_BUFFER>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    BufferID src_buffer                         = BufferID::Null();
    BufferID dst_buffer                         = BufferID::Null();
    Span<const BufferRegionCopy*, u32> regions  = nilspan;
  };

  struct RenderCommandDispatch : RenderCommandTyped<RenderCommandType::DISPATCH>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::COMPUTE;
    PipelineStateID pipeline_state_id;
    void* push_constant_data = nullptr;
    u32 push_constant_size   = 0;
    vec3u32 group_count;
  };

  struct RenderCommandDispatchIndirect : RenderCommandTyped<RenderCommandType::DISPATCH_INDIRECT>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::COMPUTE;
    PipelineStateID pipeline_state_id;
    Span<const byte*> push_constant = {nullptr, 0};
    gpu::BufferID buffer;
    usize offset = 0;
  };

  static_assert(ts_pointer<const void*>);

  struct RenderCommandRayTrace : RenderCommandTyped<RenderCommandType::RAY_TRACE>

  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RAY_TRACING;
    ShaderTableID shader_table_id;
    void* push_constant_data = nullptr;
    u32 push_constant_size   = 0;
    vec3u32 dimension;
  };

  struct RenderCommandBuildTlas : RenderCommandTyped<RenderCommandType::BUILD_TLAS>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    TlasID tlas_id;
    TlasBuildDesc build_desc;
  };

  struct RenderCommandBuildBlas : RenderCommandTyped<RenderCommandType::BUILD_BLAS>
  {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    BlasID src_blas_id;
    BlasID dst_blas_id;
    RTBuildMode build_mode = RTBuildMode::REBUILD;
    BlasBuildDesc build_desc;
  };

  struct RenderCommandBatchBuildBlas : RenderCommandTyped<RenderCommandType::BATCH_BUILD_BLAS>
  {
    static constexpr PipelineType PIPELINE_TYPE     = PipelineType::NON_SHADER;
    Span<const RenderCommandBuildBlas*, u32> builds = nilspan;
    usize max_build_memory_size                     = 0;
  };

  template <typename Func, typename RenderCommandType>
  concept command_generator = std::invocable<Func, usize> &&
                              std::same_as<RenderCommandType, std::invoke_result_t<Func, usize>>;

  template <typename T, PipelineFlags pipeline_flags>
  concept render_command =
    std::derived_from<T, RenderCommand> && pipeline_flags.test(T::PIPELINE_TYPE);

} // namespace soul::gpu
