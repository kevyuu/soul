#pragma once

#include "core/flag_map.h"
#include "core/flag_set.h"
#include "core/hash_map.h"
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
#include "gpu/intern/bindless_descriptor_allocator.h"
#include "gpu/object_cache.h"
#include "gpu/object_pool.h"

namespace soul::gpu
{
  class System;
  class RenderGraph;
  class WSI;

  using Offset2D = vec2i32;
  using Extent2D = vec2ui32;

  struct Rect2D {
    Offset2D offset;
    Extent2D extent;

    auto operator==(const Rect2D& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const Rect2D& rect)
    {
      hasher.combine(rect.offset, rect.extent);
    }
  };

  struct Viewport {
    f32 x = 0;
    f32 y = 0;
    f32 width = 0;
    f32 height = 0;

    auto operator==(const Viewport& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const Viewport& viewport)
    {
      hasher.combine(viewport.x, viewport.y, viewport.width, viewport.height);
    }
  };

  using Offset3D = vec3i32;
  using Extent3D = vec3ui32;

  enum class ErrorKind { FILE_NOT_FOUND, OTHER, COUNT };

  struct Error {
    explicit Error(const ErrorKind error_kind, const char* message)
        : error_kind(error_kind), message(message)
    {
    }

    ErrorKind error_kind;
    const char* message;
  };

  enum class IndexType { UINT16, UINT32, COUNT };

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
  using VertexElementFlags = u8;
  static_assert(VERTEX_ELEMENT_ENUM_END_BIT - 1 <= std::numeric_limits<VertexElementFlags>::max());

  enum class ShaderStage : u8 {
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
  constexpr ShaderStageFlags SHADER_STAGES_VERTEX_FRAGMENT =
    ShaderStageFlags({ShaderStage::VERTEX, ShaderStage::FRAGMENT});
  constexpr ShaderStageFlags SHADER_STAGES_RAY_TRACING =
    ShaderStageFlags({ShaderStage::RAYGEN, ShaderStage::MISS, ShaderStage::CLOSEST_HIT});

  enum class ShaderGroup : u8 { RAYGEN, MISS, HIT, CALLABLE, COUNT };

  enum class PipelineStage : u32 {
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

  using PipelineStageFlags = FlagSet<PipelineStage>;
  const auto PIPELINE_STAGE_FLAGS_ALL = ~PipelineStageFlags();

  enum class AccessType : u32 {
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

  using AccessFlags = FlagSet<AccessType>;
  constexpr AccessFlags ACCESS_FLAGS_ALL = ~AccessFlags();
  constexpr AccessFlags ACCESS_FLAGS_WRITE = {
    AccessType::SHADER_WRITE,
    AccessType::COLOR_ATTACHMENT_WRITE,
    AccessType::DEPTH_STENCIL_ATTACHMENT_WRITE,
    AccessType::TRANSFER_WRITE,
    AccessType::HOST_WRITE,
    AccessType::MEMORY_WRITE,
    AccessType::AS_WRITE};

  enum class PipelineType : u8 { NON_SHADER, RASTER, COMPUTE, RAY_TRACING, COUNT };

  using PipelineFlags = FlagSet<PipelineType>;
  constexpr auto PIPELINE_FLAGS_NON_SHADER = PipelineFlags{PipelineType::NON_SHADER};
  constexpr auto PIPELINE_FLAGS_RASTER = PipelineFlags({PipelineType::RASTER});
  constexpr auto PIPELINE_FLAGS_COMPUTE = PipelineFlags{PipelineType::COMPUTE};
  constexpr auto PIPELINE_FLAGS_RAY_TRACING = PipelineFlags{PipelineType::RAY_TRACING};

  enum class QueueType : u8 { GRAPHIC, COMPUTE, TRANSFER, COUNT, NONE = COUNT };

  using QueueFlags = FlagSet<QueueType>;
  constexpr QueueFlags QUEUE_DEFAULT = {
    QueueType::GRAPHIC, QueueType::COMPUTE, QueueType::TRANSFER};

  enum class BufferUsage : u8 {
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

  enum class TextureUsage : u8 {
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

  enum class TextureType : u8 { D1, D2, D2_ARRAY, D3, CUBE, COUNT };

  enum class TextureFormat : u16 {
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
    SRGBA8,

    RGB16,
    RGB16F,
    RGB16UI,
    RGB16I,
    R11F_G11F_B10F,

    RGB32F,

    COUNT
  };

  enum class TextureFilter : u8 { NEAREST, LINEAR, COUNT };

  enum class TextureWrap : u8 {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRROR_CLAMP_TO_EDGE,
    COUNT
  };

  enum class Topology : u8 {
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    COUNT
  };

  enum class PolygonMode : u8 { FILL, LINE, POINT, COUNT };

  enum class CullMode : u8 { FRONT, BACK, COUNT };
  using CullModeFlags = FlagSet<CullMode>;

  enum class FrontFace : u8 { CLOCKWISE, COUNTER_CLOCKWISE, COUNT };

  enum class CompareOp : u8 {
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

  enum class BlendFactor : u8 {
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

  enum class BlendOp : u8 { ADD, SUBTRACT, REVERSE_SUBTRACT, MIN, MAX, COUNT };

  enum class TextureLayout : u8 {
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

      Color() : float32() {}

      explicit Color(vec4f val) : float32(val) {}

      explicit Color(vec4ui32 val) : uint32(val) {}

      explicit Color(vec4i32 val) : int32(val) {}
    } color;

    struct DepthStencil {
      f32 depth = 0.0f;
      u32 stencil = 0;
      DepthStencil() = default;

      DepthStencil(const f32 depth, const u32 stencil) : depth(depth), stencil(stencil) {}
    } depth_stencil;

    ClearValue() = default;

    ClearValue(vec4f color, f32 depth, u32 stencil) : color(color), depth_stencil(depth, stencil) {}

    ClearValue(vec4ui32 color, f32 depth, u32 stencil) : color(color), depth_stencil(depth, stencil)
    {
    }

    ClearValue(vec4i32 color, f32 depth, u32 stencil) : color(color), depth_stencil(depth, stencil)
    {
    }
  };

  class SubresourceIndex
  {
  private:
    using value_type = u32;

    static constexpr value_type LEVEL_MASK = 0xFFFF;
    static constexpr value_type LEVEL_BIT_SHIFT = 0;

    static constexpr value_type LAYER_MASK = 0xFFFF0000;
    static constexpr u8 LAYER_BIT_SHIFT = 16;

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

  struct SubresourceIndexRange {
    using value_type = SubresourceIndex;

    SubresourceIndex base;
    u16 level_count = 1;
    u16 layer_count = 1;

    class ConstIterator
    {
    private:
      u16 mip_ = 0;
      u16 layer_ = 1;
      u16 mip_end_ = 1;

    public:
      using pointer_type = SubresourceIndex*;
      using value_type = SubresourceIndex;
      using iterator_category = std::input_iterator_tag;
      using difference_type = std::ptrdiff_t;

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
        if (mip_ == mip_end_) {
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

  enum class MemoryProperty { DEVICE_LOCAL, HOST_VISIBLE, HOST_COHERENT, HOST_CACHED, COUNT };

  using MemoryPropertyFlags = FlagSet<MemoryProperty>;

  struct MemoryOption {
    MemoryPropertyFlags required;
    MemoryPropertyFlags preferred;
  };

  struct BufferRegionCopy {
    usize src_offset = 0;
    usize dst_offset = 0;
    usize size = 0;
  };

  struct BufferUpdateDesc {
    const void* data = nullptr;
    Span<const BufferRegionCopy*, u32> regions = nilspan;
  };

  struct BufferDesc {
    usize size = 0;
    BufferUsageFlags usage_flags;
    QueueFlags queue_flags = QUEUE_DEFAULT;
    std::optional<MemoryOption> memory_option = std::nullopt;
    const char* name = nullptr;
  };

  struct TextureSubresourceLayers {
    u32 mip_level;
    u32 base_array_layer;
    u32 layer_count;
  };

  struct TextureRegionCopy {
    TextureSubresourceLayers src_subresource = {};
    Offset3D src_offset;
    TextureSubresourceLayers dst_subresource = {};
    Offset3D dst_offset;
    Extent3D extent;
  };

  struct TextureRegionUpdate {
    usize buffer_offset = 0;
    u32 buffer_row_length = 0;
    u32 buffer_image_height = 0;
    TextureSubresourceLayers subresource = {};
    Offset3D offset;
    Extent3D extent;
  };

  struct TextureLoadDesc {
    const void* data = nullptr;
    usize data_size = 0;

    Span<const TextureRegionUpdate*, u32> regions = nilspan;

    b8 generate_mipmap = false;
  };

  enum class TextureSampleCount : u8 {
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

  struct TextureDesc {
    TextureType type = TextureType::D2;
    TextureFormat format = TextureFormat::COUNT;
    vec3ui32 extent;
    u32 mip_levels = 1;
    u16 layer_count = 1;
    TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
    TextureUsageFlags usage_flags;
    QueueFlags queue_flags;
    const char* name = nullptr;

    static auto d2(
      const char* name,
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec2ui32 dimension,
      TextureSampleCount sample_count = TextureSampleCount::COUNT_1) -> TextureDesc
    {
      return {
        .type = TextureType::D2,
        .format = format,
        .extent = vec3ui32(dimension.x, dimension.y, 1),
        .mip_levels = mip_levels,
        .sample_count = sample_count,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
        .name = name};
    }

    static auto d3(
      const char* name,
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec3ui32 dimension) -> TextureDesc
    {
      return {
        .type = TextureType::D3,
        .format = format,
        .extent = dimension,
        .mip_levels = mip_levels,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
        .name = name};
    }

    static auto d2_array(
      const char* name,
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec2ui32 dimension,
      u16 layer_count) -> TextureDesc
    {
      return {
        .type = TextureType::D2_ARRAY,
        .format = format,
        .extent = vec3ui32(dimension.x, dimension.y, 1),
        .mip_levels = mip_levels,
        .layer_count = layer_count,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
        .name = name};
    }

    static auto cube(
      const char* name,
      TextureFormat format,
      u32 mip_levels,
      TextureUsageFlags usage_flags,
      QueueFlags queue_flags,
      const vec2ui32 dimension) -> TextureDesc
    {
      return {
        .type = TextureType::CUBE,
        .format = format,
        .extent = vec3ui32(dimension.x, dimension.y, 1),
        .mip_levels = mip_levels,
        .layer_count = 6,
        .usage_flags = usage_flags,
        .queue_flags = queue_flags,
        .name = name};
    }

    [[nodiscard]]
    auto get_view_count() const -> usize
    {
      return soul::cast<usize>(mip_levels) * layer_count;
    }
  };

  struct SamplerDesc {
    TextureFilter min_filter = TextureFilter::COUNT;
    TextureFilter mag_filter = TextureFilter::COUNT;
    TextureFilter mipmap_filter = TextureFilter::COUNT;
    TextureWrap wrap_u = TextureWrap::CLAMP_TO_EDGE;
    TextureWrap wrap_v = TextureWrap::CLAMP_TO_EDGE;
    TextureWrap wrap_w = TextureWrap::CLAMP_TO_EDGE;
    b8 anisotropy_enable = false;
    f32 max_anisotropy = 0.0f;
    b8 compare_enable = false;
    CompareOp compare_op = CompareOp::COUNT;

    static constexpr auto same_filter_wrap(
      const TextureFilter filter,
      const TextureWrap wrap,
      const b8 anisotropy_enable = false,
      const f32 max_anisotropy = 0.0f,
      const b8 compare_enable = false,
      const CompareOp compare_op = CompareOp::ALWAYS) -> SamplerDesc
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
        .compare_op = compare_op};
    }
  };

  struct DrawIndexedIndirectCommand {
    u32 index_count = 0;    ///< number of vertices to draw.
    u32 instance_count = 0; ///< number of instances to draw.
    u32 first_index = 0;    ///< base index within the index_buffer.
    i32 vertex_offset = 0;  ///< value added to the vertex index before indesing into vertex buffer.
    u32 first_instance = 0; ///< instance ID of the first instance to draw.
  };

  enum class RTBuildMode : u8 { REBUILD, UPDATE, COUNT };

  enum class RTBuildFlag : u8 {
    ALLOW_UPDATE,
    ALLOW_COMPACTION,
    PREFER_FAST_TRACE,
    PREFER_FAST_BUILD,
    LOW_MEMORY,
    COUNT
  };

  using RTBuildFlags = FlagSet<RTBuildFlag>;

  enum class RTGeometryType : u8 { TRIANGLE, AABB, COUNT };

  enum class RTGeometryFlag : u8 { OPAQUE, NO_DUPLICATE_ANY_HIT_INVOCATION, COUNT };

  using RTGeometryFlags = FlagSet<RTGeometryFlag>;

  struct RTTriangleDesc {
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

  struct RTAABBDesc {
    u32 count;
    GPUAddress data;
    uint64_t stride;
  };

  struct RTGeometryDesc {
    RTGeometryType type;
    RTGeometryFlags flags;

    union {
      RTTriangleDesc triangles;
      RTAABBDesc aabbs;
    } content;
  };

  enum class RTGeometryInstanceFlag : u32 {
    // The enum values are kept consistent with D3D12_RAYTRACING_INSTANCE_FLAGS
    // and VkGeometryInstanceFlagBitsKHR.
    TRIANGLE_FACING_CULL_DISABLE,
    TRIANGLE_FRONT_COUNTER_CLOCKWISE,
    FORCE_OPAQUE,
    NO_OPAQUE,
    COUNT
  };

  using RTGeometryInstanceFlags = FlagSet<RTGeometryInstanceFlag>;

  struct RTInstanceDesc {
    f32 transform[3][4] = {};
    u32 instance_id : 24 = {};
    u32 instance_mask : 8 = {};
    u32 sbt_offset : 24 = {};
    u32 flags : 8 = {};
    GPUAddress blas_gpu_address;

    RTInstanceDesc() = default;
    RTInstanceDesc(
      mat4f in_transform,
      u32 instance_id,
      u32 instance_mask,
      u32 sbt_offset,
      RTGeometryInstanceFlags flags,
      GPUAddress blas_gpu_address);
  };

  struct TlasDesc {
    const char* name = nullptr;
    usize size = 0;
  };

  struct BlasDesc {
    const char* name = nullptr;
    usize size = 0;
  };

  struct TlasBuildDesc {
    RTBuildFlags build_flags;
    RTGeometryFlags geometry_flags;
    GPUAddress instance_data;
    u32 instance_count = 0;
    u32 instance_offset = 0;
  };

  struct BlasBuildDesc {
    RTBuildFlags flags;
    u32 geometry_count;
    const RTGeometryDesc* geometry_descs;
  };

  struct ShaderFile {
    Path path;
  };

  struct ShaderString {
    String source;
  };

  using ShaderSource = Variant<ShaderFile, ShaderString>;

  struct ShaderEntryPoint {
    ShaderStage stage;
    StringView name;
  };

  static constexpr auto ENTRY_POINT_UNUSED = VK_SHADER_UNUSED_KHR;

  struct RTGeneralShaderGroup {
    u32 entry_point = ENTRY_POINT_UNUSED;
  };

  struct RTTriangleHitGroup {
    u32 any_hit_entry_point = ENTRY_POINT_UNUSED;
    u32 closest_hit_entry_point = ENTRY_POINT_UNUSED;
    u32 intersection_entry_point = ENTRY_POINT_UNUSED;
  };

  struct ShaderDefine {
    const char* key;
    const char* value;
  };

  struct ProgramDesc {
    Span<const Path*, u32> search_paths = nilspan;
    Span<const ShaderDefine*, u32> shader_defines = nilspan;
    Span<const ShaderSource*, u32> sources = nilspan;
    Span<const ShaderEntryPoint*, u32> entry_points = nilspan;
  };

  struct ShaderTableDesc {
    ProgramID program_id;
    RTGeneralShaderGroup raygen_group;
    Span<const RTGeneralShaderGroup*, u32> miss_groups = nilspan;
    Span<const RTTriangleHitGroup*, u32> hit_groups = nilspan;
    u32 max_recursion_depth = 0;
    const char* name = nullptr;
  };

  enum class RTPipelineFlag : u8 { SKIP_TRIANGLE, SKIP_PROCEDURAL_PRIMITIVES, COUNT };

  using RTPipelineFlags = FlagSet<RTPipelineFlag>;

  using AttachmentFlagBits = enum {
    ATTACHMENT_ACTIVE_BIT = 0x1,
    ATTACHMENT_FIRST_PASS_BIT = 0x2,
    ATTACHMENT_LAST_PASS_BIT = 0x4,
    ATTACHMENT_EXTERNAL_BIT = 0x8,
    ATTACHMENT_CLEAR_BIT = 0x10,
    ATTACHMENT_ENUM_END_BIT
  };
  using AttachmentFlags = u8;
  static_assert(ATTACHMENT_ENUM_END_BIT - 1 < std::numeric_limits<AttachmentFlags>::max());

  struct Attachment {
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

  struct InputLayoutDesc {
    Topology topology = Topology::TRIANGLE_LIST;

    auto operator==(const InputLayoutDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const InputLayoutDesc& val)
    {
      hasher.combine(val.topology);
    }
  };

  struct InputBindingDesc {
    u32 stride = 0;

    auto operator==(const InputBindingDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const InputBindingDesc& desc)
    {
      hasher.combine(desc.stride);
    }
  };

  struct InputAttrDesc {
    u32 binding = 0;
    u32 offset = 0;
    VertexElementType type = VertexElementType::DEFAULT;
    VertexElementFlags flags = 0;

    auto operator==(const InputAttrDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const InputAttrDesc& val)
    {
      hasher.combine(val.binding, val.offset, val.type, val.flags);
    }
  };

  struct ColorAttachmentDesc {
    b8 blend_enable = false;
    b8 color_write = true;
    BlendFactor src_color_blend_factor = BlendFactor::ZERO;
    BlendFactor dst_color_blend_factor = BlendFactor::ZERO;
    BlendOp color_blend_op = BlendOp::ADD;
    BlendFactor src_alpha_blend_factor = BlendFactor::ZERO;
    BlendFactor dst_alpha_blend_factor = BlendFactor::ZERO;
    BlendOp alpha_blend_op = BlendOp::ADD;

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

  struct DepthStencilAttachmentDesc {
    b8 depth_test_enable = false;
    b8 depth_write_enable = false;
    CompareOp depth_compare_op = CompareOp::NEVER;

    auto operator==(const DepthStencilAttachmentDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const DepthStencilAttachmentDesc& desc)
    {
      hasher.combine(desc.depth_test_enable, desc.depth_write_enable, desc.depth_compare_op);
    }
  };

  struct DepthBiasDesc {
    f32 constant = 0.0f;
    f32 slope = 0.0f;

    auto operator==(const DepthBiasDesc&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const DepthBiasDesc& desc)
    {
      hasher.combine(desc.constant, desc.slope);
    }
  };

  struct GraphicPipelineStateDesc {
    ProgramID program_id;

    InputLayoutDesc input_layout;

    Array<InputBindingDesc, MAX_INPUT_BINDING_PER_SHADER> input_bindings;

    Array<InputAttrDesc, MAX_INPUT_PER_SHADER> input_attributes;

    Viewport viewport;

    Rect2D scissor = {};

    struct RasterDesc {
      f32 line_width = 1.0f;
      PolygonMode polygon_mode = PolygonMode::FILL;
      CullModeFlags cull_mode = {};
      FrontFace front_face = FrontFace::CLOCKWISE;

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

  struct ComputePipelineStateDesc {
    ProgramID program_id;

    auto operator==(const ComputePipelineStateDesc& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const ComputePipelineStateDesc& val)
    {
      hasher.combine(val.program_id);
    }
  };

  struct RayTracingPipelineStateDesc {
    ProgramID program_id;
  };

  struct GPUProperties {
    struct GPULimit {
      TextureSampleCountFlags color_sample_count_flags;
      TextureSampleCountFlags depth_sample_count_flags;
    } limit;
  };

  namespace impl
  {

    class PrimaryCommandBuffer;

    struct GraphicPipelineStateKey {
      GraphicPipelineStateDesc desc;
      TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
      auto operator==(const GraphicPipelineStateKey&) const -> bool = default;

      friend void soul_op_hash_combine(auto& hasher, const GraphicPipelineStateKey& key)
      {
        hasher.combine(key.desc, key.sample_count);
      }
    };

    struct ComputePipelineStateKey {
      ComputePipelineStateDesc desc;
      auto operator==(const ComputePipelineStateKey&) const -> bool = default;
      friend void soul_op_hash_combine(auto& hasher, const ComputePipelineStateKey& key)
      {
        hasher.combine(key.desc);
      }
    };

    struct PipelineStateKey {

      using variant = Variant<GraphicPipelineStateKey, ComputePipelineStateKey>;
      variant var;

      static auto From(const GraphicPipelineStateKey& key) -> PipelineStateKey
      {
        return {.var = variant::From(key)};
      }

      static auto From(const ComputePipelineStateKey& key) -> PipelineStateKey
      {
        return {.var = variant::From(key)};
      }

      auto operator==(const PipelineStateKey&) const -> bool = default;

      friend void soul_op_hash_combine(auto& hasher, const PipelineStateKey& key)
      {
        soul_op_hash_combine(hasher, key.var);
      }
    };

    using PipelineStateCache = ConcurrentObjectCache<PipelineStateKey, PipelineState>;

    struct PipelineState {
      VkPipeline vk_handle = VK_NULL_HANDLE;
      VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_MAX_ENUM;
      ProgramID program_id;
    };

    struct ProgramDescriptorBinding {
      u8 count = 0;
      u8 attachment_index = 0;
      VkShaderStageFlags shader_stage_flags = 0;
      VkPipelineStageFlags pipeline_stage_flags = 0;
    };

    struct RenderPassKey {
      Array<Attachment, MAX_COLOR_ATTACHMENT_PER_SHADER> color_attachments;
      Array<Attachment, MAX_COLOR_ATTACHMENT_PER_SHADER> resolve_attachments;
      Array<Attachment, MAX_INPUT_ATTACHMENT_PER_SHADER> input_attachments;
      Attachment depth_attachment;

      auto operator==(const RenderPassKey& other) const -> bool = default;

      friend void soul_op_hash_combine(auto& hasher, const RenderPassKey& val)
      {
        hasher.combine_span(val.color_attachments.cspan());
        hasher.combine_span(val.resolve_attachments.cspan());
        hasher.combine_span(val.input_attachments.cspan());
        hasher.combine(val.depth_attachment);
      }
    };

    struct QueueData {
      u32 count = 0;
      u32 indices[3] = {};
    };

    struct Swapchain {
      WSI* wsi = nullptr;
      VkSwapchainKHR vk_handle = VK_NULL_HANDLE;
      VkSurfaceFormatKHR format = {};
      VkExtent2D extent = {};
      u32 image_count = 0;
      SBOVector<TextureID> textures;
      SBOVector<VkImage> images;
      SBOVector<VkImageView> image_views;
    };

    struct DescriptorSetLayoutBinding {
      VkDescriptorType descriptor_type;
      u32 descriptor_count;
      VkShaderStageFlags stage_flags;

      friend void soul_op_hash_combine(auto& hasher, const DescriptorSetLayoutBinding& binding)
      {
        hasher.combine(binding.descriptor_type);
        hasher.combine(binding.descriptor_count);
        hasher.combine(binding.stage_flags);
      }
    };
    static_assert(soul::impl_soul_op_hash_combine_v<DescriptorSetLayoutBinding>);

    struct DescriptorSetLayoutKey {
      Array<DescriptorSetLayoutBinding, MAX_BINDING_PER_SET> bindings;

      auto operator==(const DescriptorSetLayoutKey& other) const -> bool = default;
      friend void soul_op_hash_combine(auto& hasher, const DescriptorSetLayoutKey& key) {}
    };
    static_assert(soul::impl_soul_op_hash_combine_v<gpu::impl::DescriptorSetLayoutKey>);

    using DescriptorSetLayoutCache =
      ConcurrentObjectCache<DescriptorSetLayoutKey, VkDescriptorSetLayout>;

    struct ShaderDescriptorBinding {
      u8 count = 0;
      u8 attachmentIndex = 0;
    };

    struct ShaderInput {
      VkFormat format = VK_FORMAT_UNDEFINED;
      u32 offset = 0;
    };

    using VisibleAccessMatrix = FlagMap<PipelineStage, AccessFlags>;
    constexpr auto VISIBLE_ACCESS_MATRIX_ALL = VisibleAccessMatrix::Fill(ACCESS_FLAGS_ALL);
    constexpr auto VISIBLE_ACCESS_MATRIX_NONE = VisibleAccessMatrix::Fill(AccessFlags());

    struct ResourceCacheState {
      QueueType queue_owner = QueueType::COUNT;
      PipelineStageFlags unavailable_pipeline_stages;
      AccessFlags unavailable_accesses;
      PipelineStageFlags sync_stages = PIPELINE_STAGE_FLAGS_ALL;
      VisibleAccessMatrix visible_access_matrix = VISIBLE_ACCESS_MATRIX_ALL;

      auto commit_acquire_swapchain() -> void
      {
        queue_owner = QueueType::GRAPHIC;
        unavailable_pipeline_stages = {};
        unavailable_accesses = {};
        sync_stages = {};
        visible_access_matrix = VISIBLE_ACCESS_MATRIX_NONE;
      }

      auto commit_wait_semaphore(
        QueueType src_queue_type, QueueType dst_queue_type, PipelineStageFlags dst_stages) -> void
      {
        if (queue_owner != QueueType::COUNT && queue_owner != src_queue_type) {
          return;
        }
        queue_owner = dst_queue_type;
        sync_stages = dst_stages;
        unavailable_pipeline_stages = {};
        unavailable_accesses = {};
        dst_stages.for_each([this](const PipelineStage dst_stage) {
          visible_access_matrix[dst_stage] = ACCESS_FLAGS_ALL;
        });
      }

      auto commit_wait_event_or_barrier(
        QueueType queue_type,
        PipelineStageFlags src_stages,
        AccessFlags src_accesses,
        PipelineStageFlags dst_stages,
        AccessFlags dst_accesses,
        b8 layout_change = false) -> void
      {
        if (queue_owner != QueueType::COUNT && queue_owner != queue_type) {
          return;
        }
        if ((sync_stages & src_stages).none()) {
          return;
        }
        if ((unavailable_pipeline_stages & src_stages) != unavailable_pipeline_stages) {
          return;
        }
        if ((unavailable_accesses & src_accesses) != unavailable_accesses) {
          return;
        }
        queue_owner = queue_type;
        sync_stages = dst_stages;
        unavailable_pipeline_stages = {};
        unavailable_accesses = {};
        if (layout_change) {
          visible_access_matrix = VISIBLE_ACCESS_MATRIX_NONE;
        }
        dst_stages.for_each([this, dst_accesses](const PipelineStage dst_stage) {
          visible_access_matrix[dst_stage] |= dst_accesses;
        });
      }

      auto commit_access(QueueType queue, PipelineStageFlags stages, AccessFlags accesses) -> void
      {
        SOUL_ASSERT(0, (sync_stages & stages) == stages, "");
        SOUL_ASSERT(0, unavailable_accesses.none(), "");
        queue_owner = queue;
        unavailable_pipeline_stages |= stages;
        const auto write_accesses = (accesses & ACCESS_FLAGS_WRITE);
        if (write_accesses.any()) {
          unavailable_accesses |= write_accesses;
          visible_access_matrix = VISIBLE_ACCESS_MATRIX_NONE;
        }
      }

      [[nodiscard]]
      auto need_invalidate(PipelineStageFlags stages, AccessFlags accesses) -> b8
      {
        return stages
          .find_if([this, accesses](const PipelineStage pipeline_stage) {
            return accesses.test_any(~visible_access_matrix[pipeline_stage]);
          })
          .is_some();
      }

      auto join(const ResourceCacheState& other) -> void
      {
        unavailable_pipeline_stages |= other.unavailable_pipeline_stages;
        unavailable_accesses |= other.unavailable_accesses;
        for (const auto stage_flag : FlagIter<PipelineStage>()) {
          visible_access_matrix[stage_flag] &= other.visible_access_matrix[stage_flag];
        }
      }
    };

    struct Buffer {
      BufferDesc desc;
      VkBuffer vk_handle = VK_NULL_HANDLE;
      VmaAllocation allocation{};
      ResourceCacheState cache_state;
      DescriptorID storage_buffer_gpu_handle = DescriptorID::null();
      VkMemoryPropertyFlags memory_property_flags = 0;
    };

    struct TextureView {
      VkImageView vk_handle = VK_NULL_HANDLE;
      DescriptorID storage_image_gpu_handle = DescriptorID::null();
      DescriptorID sampled_image_gpu_handle = DescriptorID::null();
    };

    struct Texture {
      TextureDesc desc;
      VkImage vk_handle = VK_NULL_HANDLE;
      VmaAllocation allocation = VK_NULL_HANDLE;
      TextureView view;
      TextureView* views = nullptr;
      VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
      VkSharingMode sharing_mode = {};
      ResourceCacheState cache_state;
    };

    struct Blas {
      struct BlasGroupData {
        BlasGroupID group_id;
        usize index = 0;
      };

      BlasDesc desc;
      VkAccelerationStructureKHR vk_handle = VK_NULL_HANDLE;
      BufferID buffer;
      ResourceCacheState cache_state;
      BlasGroupData group_data;
    };

    struct BlasGroup {
      Vector<BlasID> blas_list = Vector<BlasID>();
      ResourceCacheState cache_state = ResourceCacheState();
      const char* name = nullptr;
    };

    struct Tlas {
      TlasDesc desc;
      VkAccelerationStructureKHR vk_handle = VK_NULL_HANDLE;
      BufferID buffer;
      DescriptorID descriptor_id;
      ResourceCacheState cache_state;
    };

    struct Shader {
      ShaderStage stage;
      VkShaderModule vk_handle = VK_NULL_HANDLE;
      String entry_point;
    };

    struct ShaderTable {
      using Buffers = FlagMap<ShaderGroup, BufferID>;
      using Regions = FlagMap<ShaderGroup, VkStridedDeviceAddressRegionKHR>;

      VkPipeline pipeline = VK_NULL_HANDLE;
      Buffers buffers = Buffers::Fill(BufferID::null());
      Regions vk_regions = Regions();
    };

    struct Program {
      VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
      SBOVector<Shader> shaders;
    };

    struct BinarySemaphore {
      enum class State : u8 { INIT, SIGNALLED, WAITED, COUNT };

      VkSemaphore vk_handle = VK_NULL_HANDLE;
      State state = State::INIT;

      static auto null() -> BinarySemaphore { return {VK_NULL_HANDLE}; }
      [[nodiscard]]
      auto is_null() const -> b8
      {
        return vk_handle == VK_NULL_HANDLE;
      }
      [[nodiscard]]
      auto is_valid() const -> b8
      {
        return !is_null();
      }
    };

    struct TimelineSemaphore {
      u32 queue_family_index;
      VkSemaphore vk_handle;
      u64 counter;

      static auto null() -> TimelineSemaphore { return {}; }
      [[nodiscard]]
      auto is_null() const -> b8
      {
        return counter == 0;
      }
      [[nodiscard]]
      auto is_valid() const -> b8
      {
        return !is_null();
      }
    };

    using Semaphore = Variant<BinarySemaphore*, TimelineSemaphore>;

    [[nodiscard]]
    inline auto is_semaphore_valid(Semaphore semaphore) -> b8
    {
      return semaphore.visit(VisitorSet{
        [](BinarySemaphore* semaphore) { return semaphore->is_valid(); },
        [](const TimelineSemaphore& semaphore) { return semaphore.is_valid(); },
      });
    }

    [[nodiscard]]
    inline auto is_semaphore_null(Semaphore semaphore) -> b8
    {
      return semaphore.visit(VisitorSet{
        [](BinarySemaphore* semaphore) { return semaphore->is_null(); },
        [](const TimelineSemaphore& semaphore) { return semaphore.is_null(); },
      });
    }

    class CommandQueue
    {
    public:
      auto init(VkDevice device, u32 family_index, u32 queue_index) -> void;

      auto wait(Semaphore semaphore, VkPipelineStageFlags wait_stages) -> void;

      auto wait(BinarySemaphore* semaphore, VkPipelineStageFlags wait_stages) -> void;

      auto wait(TimelineSemaphore semaphore, VkPipelineStageFlags wait_stages) -> void;

      [[nodiscard]]
      auto get_vk_handle() const -> VkQueue
      {
        return vk_handle_;
      }

      auto get_timeline_semaphore() -> TimelineSemaphore;

      auto submit(PrimaryCommandBuffer command_buffer, BinarySemaphore* = nullptr) -> void;

      auto flush(BinarySemaphore* binary_semaphore = nullptr) -> void;

      auto present(VkSwapchainKHR swapchain, u32 swapchain_index, BinarySemaphore* semaphore)
        -> void;

      [[nodiscard]]
      auto get_family_index() const -> u32
      {
        return family_index_;
      }

      [[nodiscard]]
      auto is_waiting_binary_semaphore() const -> b8;

      [[nodiscard]]
      auto is_waiting_timeline_semaphore() const -> b8;

    private:
      auto init_timeline_semaphore() -> void;

      VkDevice device_ = VK_NULL_HANDLE;
      VkQueue vk_handle_ = VK_NULL_HANDLE;
      u32 family_index_ = 0;
      SBOVector<VkSemaphore> wait_semaphores_;
      SBOVector<VkPipelineStageFlags> wait_stages_;
      SBOVector<u64> wait_timeline_values_;
      SBOVector<VkCommandBuffer> commands_;

      VkSemaphore timeline_semaphore_ = VK_NULL_HANDLE;
      u64 current_timeline_values_ = 0;
    };

    class SecondaryCommandBuffer
    {
    private:
      VkCommandBuffer vk_handle_ = VK_NULL_HANDLE;

    public:
      constexpr SecondaryCommandBuffer() noexcept = default;

      explicit constexpr SecondaryCommandBuffer(VkCommandBuffer vk_handle) : vk_handle_(vk_handle)
      {
      }

      [[nodiscard]]
      constexpr auto get_vk_handle() const noexcept -> VkCommandBuffer
      {
        return vk_handle_;
      }
      auto end() -> void;
    };

    class PrimaryCommandBuffer
    {
    private:
      VkCommandBuffer vk_handle_ = VK_NULL_HANDLE;

    public:
      constexpr PrimaryCommandBuffer() noexcept = default;

      explicit constexpr PrimaryCommandBuffer(VkCommandBuffer vk_handle) : vk_handle_(vk_handle) {}

      [[nodiscard]]
      constexpr auto get_vk_handle() const noexcept -> VkCommandBuffer
      {
        return vk_handle_;
      }
      [[nodiscard]]
      auto is_null() const noexcept -> b8
      {
        return vk_handle_ == VK_NULL_HANDLE;
      }
    };

    using CommandQueues = FlagMap<QueueType, CommandQueue>;

    class CommandPool
    {
    public:
      explicit CommandPool(memory::Allocator* allocator = get_default_allocator())
          : allocator_initializer_(allocator)
      {
        allocator_initializer_.end();
      }

      auto init(VkDevice device, VkCommandBufferLevel level, u32 queue_family_index) -> void;
      auto reset() -> void;
      auto request() -> VkCommandBuffer;

    private:
      runtime::AllocatorInitializer allocator_initializer_;
      VkDevice device_ = VK_NULL_HANDLE;
      VkCommandPool vk_handle_ = VK_NULL_HANDLE;
      Vector<VkCommandBuffer> allocated_buffers_;
      VkCommandBufferLevel level_ = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
      u16 count_ = 0;
    };

    class CommandPools
    {
    public:
      explicit CommandPools(memory::Allocator* allocator = get_default_allocator())
          : allocator_(allocator), allocator_initializer_(allocator)
      {
        allocator_initializer_.end();
      }

      auto init(VkDevice device, const CommandQueues& queues, usize thread_count) -> void;
      auto reset() -> void;

      auto request_command_buffer(QueueType queue_type) -> PrimaryCommandBuffer;
      auto request_secondary_command_buffer(
        VkRenderPass render_pass, uint32_t subpass, VkFramebuffer framebuffer)
        -> SecondaryCommandBuffer;

    private:
      memory::Allocator* allocator_;
      runtime::AllocatorInitializer allocator_initializer_;
      Vector<FlagMap<QueueType, CommandPool>> primary_pools_;
      Vector<CommandPool> secondary_pools_;
    };

    class GPUResourceInitializer
    {
    public:
      auto init(VmaAllocator gpu_allocator, CommandPools* command_pools) -> void;
      auto load(Buffer& buffer, const void* data) -> void;
      auto load(Texture& texture, const TextureLoadDesc& load_desc) -> void;
      auto clear(Texture& texture, ClearValue clear_value) -> void;
      auto generate_mipmap(Texture& texture) -> void;
      auto flush(CommandQueues& command_queues, System& gpu_system) -> void;
      auto reset() -> void;

    private:
      struct StagingBuffer {
        VkBuffer vk_handle;
        VmaAllocation allocation;
      };

      struct alignas(SOUL_CACHELINE_SIZE) ThreadContext {
        PrimaryCommandBuffer transfer_command_buffer;
        PrimaryCommandBuffer clear_command_buffer;
        PrimaryCommandBuffer mipmap_gen_command_buffer;
        PrimaryCommandBuffer as_command_buffer;
        Vector<StagingBuffer> staging_buffers;
      };

      VmaAllocator gpu_allocator_ = nullptr;
      CommandPools* command_pools_ = nullptr;

      auto get_thread_context() -> ThreadContext&;
      auto get_staging_buffer(usize size) -> StagingBuffer;
      auto get_transfer_command_buffer() -> PrimaryCommandBuffer;
      auto get_mipmap_gen_command_buffer() -> PrimaryCommandBuffer;
      auto get_clear_command_buffer() -> PrimaryCommandBuffer;
      auto load_staging_buffer(const StagingBuffer&, const void* data, usize size) -> void;
      auto load_staging_buffer(
        const StagingBuffer&, const void* data, usize count, usize type_size, usize stride) -> void;

      Vector<ThreadContext> thread_contexts_;
    };

    class GPUResourceFinalizer
    {
    public:
      auto init() -> void;
      auto finalize(Buffer& buffer) -> void;
      auto finalize(Texture& texture, TextureUsageFlags usage_flags) -> void;
      auto flush(CommandPools& command_pools, CommandQueues& command_queues, System& gpu_system)
        -> void;

    private:
      struct alignas(SOUL_CACHELINE_SIZE) ThreadContext {
        FlagMap<QueueType, Vector<VkImageMemoryBarrier>> image_barriers_;
        FlagMap<QueueType, QueueFlags> sync_dst_queues_;

        // TODO(kevinyu): Investigate why we cannot use explicitly or implicitly default constructor
        // here.
        // - alignas seems to be relevant
        // - only happen on release mode
        ThreadContext() {} // NOLINT

        ThreadContext(const ThreadContext&) = delete;
        auto operator=(const ThreadContext&) -> ThreadContext& = delete;
        ThreadContext(ThreadContext&&) = default;
        auto operator=(ThreadContext&&) -> ThreadContext& = default;

        ~ThreadContext() {} // NOLINT
      };

      Vector<ThreadContext> thread_contexts_;
    };

    struct FrameContext {
      runtime::AllocatorInitializer allocator_initializer;
      CommandPools command_pools;

      TimelineSemaphore frame_end_semaphore = TimelineSemaphore::null();
      BinarySemaphore image_available_semaphore;
      BinarySemaphore render_finished_semaphore;

      u32 swapchain_index = 0;

      struct Garbages {
        Vector<ProgramID> programs;
        Vector<TextureID> textures;
        Vector<BufferID> buffers;
        Vector<VkAccelerationStructureKHR> as_vk_handles;
        Vector<DescriptorID> as_descriptors;
        Vector<VkRenderPass> render_passes;
        Vector<VkFramebuffer> frame_buffers;
        Vector<VkPipeline> pipelines;
        Vector<VkEvent> events;
        Vector<BinarySemaphore> semaphores;

        struct SwapchainGarbage {
          VkSwapchainKHR vk_handle = VK_NULL_HANDLE;
          SBOVector<VkImageView> image_views;
        } swapchain;
      } garbages;

      GPUResourceInitializer gpu_resource_initializer;
      GPUResourceFinalizer gpu_resource_finalizer;

      explicit FrameContext(memory::Allocator* allocator) : allocator_initializer(allocator)
      {
        allocator_initializer.end();
      }
    };

    struct Database {
      using CPUAllocatorProxy = memory::MultiProxy<memory::ProfileProxy, memory::CounterProxy>;
      using CPUAllocator = memory::ProxyAllocator<memory::Allocator, CPUAllocatorProxy>;
      CPUAllocator cpu_allocator;

      memory::MallocAllocator vulkan_cpu_backing_allocator{"Vulkan CPU Backing Allocator"};
      using VulkanCPUAllocatorProxy = memory::MultiProxy<memory::MutexProxy, memory::ProfileProxy>;
      using VulkanCPUAllocator =
        memory::ProxyAllocator<memory::MallocAllocator, VulkanCPUAllocatorProxy>;
      VulkanCPUAllocator vulkan_cpu_allocator;

      runtime::AllocatorInitializer allocator_initializer;

      VkInstance instance = VK_NULL_HANDLE;
      VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

      VkDevice device = VK_NULL_HANDLE;
      VkPhysicalDevice physical_device = VK_NULL_HANDLE;
      VkPhysicalDeviceProperties physical_device_properties = {};
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
      VkPhysicalDeviceAccelerationStructurePropertiesKHR as_properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
        .pNext = &ray_tracing_properties};
      GPUProperties gpu_properties = {};
      VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};
      VkPhysicalDeviceFeatures physical_device_features = {};
      FlagMap<QueueType, u32> queue_family_indices = {};

      CommandQueues queues;

      VkSurfaceKHR surface = VK_NULL_HANDLE;
      VkSurfaceCapabilitiesKHR surface_caps = {};

      Swapchain swapchain;

      Vector<FrameContext> frame_contexts;
      u32 frame_counter = 0;
      u32 current_frame = 0;

      VmaAllocator gpu_allocator = VK_NULL_HANDLE;
      Vector<VmaPool> linear_pools;

      BufferPool buffer_pool;
      TexturePool texture_pool;
      BlasPool blas_pool;
      BlasGroupPool blas_group_pool;
      TlasPool tlas_pool;
      ShaderPool shaders;

      impl::PipelineStateCache pipeline_state_cache;

      Pool<Program> programs;
      ShaderTablePool shader_table_pool;

      HashMap<RenderPassKey, VkRenderPass, HashOp<RenderPassKey>> render_pass_maps;

      UInt64HashMap<SamplerID> sampler_map;
      BindlessDescriptorAllocator descriptor_allocator;

      explicit Database(memory::Allocator* backing_allocator)
          : cpu_allocator(
              "GPU System allocator",
              backing_allocator,
              CPUAllocatorProxy::Config{
                memory::ProfileProxy::Config(), memory::CounterProxy::Config()}),
            vulkan_cpu_allocator(
              "Vulkan allocator",
              &vulkan_cpu_backing_allocator,
              VulkanCPUAllocatorProxy::Config{
                memory::MutexProxy::Config(), memory::ProfileProxy::Config()}),
            allocator_initializer(&cpu_allocator)
      {
        allocator_initializer.end();
      }
    };

  } // namespace impl

  using PipelineStateID = ID<
    struct pipeline_state_id_tag,
    impl::PipelineStateCache::ID,
    impl::PipelineStateCache::NULLVAL>;

  // Render Command API
  enum class RenderCommandType : u8 {
    DRAW,
    DRAW_INDEX,
    DRAW_INDEXED_INDIRECT,
    COPY_TEXTURE,
    UPDATE_TEXTURE,
    UPDATE_BUFFER,
    COPY_BUFFER,
    DISPATCH,
    CLEAR_COLOR,
    RAY_TRACE,
    BUILD_TLAS,
    BUILD_BLAS,
    BATCH_BUILD_BLAS,
    COUNT
  };

  struct RenderCommand {
    RenderCommandType type = RenderCommandType::COUNT;
  };

  template <RenderCommandType RENDER_COMMAND_TYPE>
  struct RenderCommandTyped : RenderCommand {
    static const RenderCommandType TYPE = RENDER_COMMAND_TYPE;

    RenderCommandTyped() : RenderCommand(TYPE) {}
  };

  struct RenderCommandDraw : RenderCommandTyped<RenderCommandType::DRAW> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RASTER;
    PipelineStateID pipeline_state_id;
    void* push_constant_data = nullptr;
    u32 push_constant_size = 0;
    BufferID vertex_buffer_i_ds[MAX_VERTEX_BINDING];
    u16 vertex_offsets[MAX_VERTEX_BINDING] = {};
    u32 vertex_count = 0;
    u32 instance_count = 0;
    u32 first_vertex = 0;
    u32 first_instance = 0;
  };

  struct RenderCommandDrawIndex : RenderCommandTyped<RenderCommandType::DRAW_INDEX> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RASTER;
    PipelineStateID pipeline_state_id;
    const void* push_constant_data = nullptr;
    u32 push_constant_size = 0;
    BufferID vertex_buffer_ids[MAX_VERTEX_BINDING];
    u16 vertex_offsets[MAX_VERTEX_BINDING] = {};
    BufferID index_buffer_id;
    usize index_offset = 0;
    IndexType index_type = IndexType::UINT16;
    u32 first_index = 0;
    u32 index_count = 0;
  };

  struct RenderCommandDrawIndexedIndirect
      : RenderCommandTyped<RenderCommandType::DRAW_INDEXED_INDIRECT> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RASTER;
    PipelineStateID pipeline_state_id;
    const void* push_constant_data = nullptr;
    u32 push_constant_size = 0;
    BufferID vertex_buffer_ids[MAX_VERTEX_BINDING];
    u16 vertex_offsets[MAX_VERTEX_BINDING] = {};
    BufferID index_buffer_id;
    usize index_offset = 0;
    IndexType index_type = IndexType::UINT16;
    BufferID buffer_id;
    u64 offset = 0;
    u32 draw_count = 0;
    u32 stride = 0;
  };

  struct RenderCommandUpdateTexture : RenderCommandTyped<RenderCommandType::UPDATE_TEXTURE> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    TextureID dst_texture = TextureID::null();
    const void* data = nullptr;
    usize data_size = 0;
    Span<const TextureRegionUpdate*, u32> regions = nilspan;
  };

  struct RenderCommandCopyTexture : RenderCommandTyped<RenderCommandType::COPY_TEXTURE> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    TextureID src_texture = TextureID::null();
    TextureID dst_texture = TextureID::null();
    Span<const TextureRegionCopy*, u32> regions = nilspan;
  };

  struct RenderCommandUpdateBuffer : RenderCommandTyped<RenderCommandType::UPDATE_BUFFER> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    BufferID dst_buffer = BufferID::null();
    void* data = nullptr;
    Span<const BufferRegionCopy*, u32> regions = nilspan;
  };

  struct RenderCommandCopyBuffer : RenderCommandTyped<RenderCommandType::COPY_BUFFER> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    BufferID src_buffer = BufferID::null();
    BufferID dst_buffer = BufferID::null();
    Span<const BufferRegionCopy*, u32> regions = nilspan;
  };

  struct RenderCommandDispatch : RenderCommandTyped<RenderCommandType::DISPATCH> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::COMPUTE;
    PipelineStateID pipeline_state_id;
    void* push_constant_data = nullptr;
    u32 push_constant_size = 0;
    vec3ui32 group_count;
  };

  struct RenderCommandRayTrace : RenderCommandTyped<RenderCommandType::RAY_TRACE> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::RAY_TRACING;
    ShaderTableID shader_table_id;
    void* push_constant_data = nullptr;
    u32 push_constant_size = 0;
    vec3ui32 dimension;
  };

  struct RenderCommandBuildTlas : RenderCommandTyped<RenderCommandType::BUILD_TLAS> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    TlasID tlas_id;
    TlasBuildDesc build_desc;
  };

  struct RenderCommandBuildBlas : RenderCommandTyped<RenderCommandType::BUILD_BLAS> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    BlasID src_blas_id;
    BlasID dst_blas_id;
    RTBuildMode build_mode = RTBuildMode::REBUILD;
    BlasBuildDesc build_desc;
  };

  struct RenderCommandBatchBuildBlas : RenderCommandTyped<RenderCommandType::BATCH_BUILD_BLAS> {
    static constexpr PipelineType PIPELINE_TYPE = PipelineType::NON_SHADER;
    Span<const RenderCommandBuildBlas*, u32> builds = nilspan;
    usize max_build_memory_size = 0;
  };

  template <typename Func, typename RenderCommandType>
  concept command_generator = std::invocable<Func, usize> &&
                              std::same_as<RenderCommandType, std::invoke_result_t<Func, usize>>;

  template <typename T, PipelineFlags pipeline_flags>
  concept render_command =
    std::derived_from<T, RenderCommand> && pipeline_flags.test(T::PIPELINE_TYPE);

} // namespace soul::gpu
