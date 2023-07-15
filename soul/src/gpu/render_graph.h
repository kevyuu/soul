#pragma once

#include <span>

#include "core/slice.h"
#include "core/type_traits.h"
#include "gpu/command_list.h"
#include "gpu/type.h"
#include "memory/allocator.h"

namespace soul::gpu
{

  namespace impl
  {

    struct ResourceNode;
    struct RGResourceID;

    struct RGResourceID {
      u32 index = std::numeric_limits<u32>::max();

      static constexpr u8 EXTERNAL_BIT_POSITION = 31;

      static auto internal_id(const u32 index) -> RGResourceID { return {index}; }
      static auto external_id(const u32 index) -> RGResourceID
      {
        return {index | 1u << EXTERNAL_BIT_POSITION};
      }

      [[nodiscard]]
      auto is_external() const -> b8
      {
        return index & (1u << EXTERNAL_BIT_POSITION);
      }

      [[nodiscard]]
      auto get_index() const -> u32
      {
        return index & ~(1u << EXTERNAL_BIT_POSITION);
      }
    };

    constexpr RGResourceID RG_RESOURCE_ID_NULL = {std::numeric_limits<u32>::max()};

    using RGBufferID = RGResourceID;
    constexpr RGBufferID RG_BUFFER_ID_NULL = RG_RESOURCE_ID_NULL;

    using RGTextureID = RGResourceID;
    constexpr RGTextureID RG_TEXTURE_ID_NULL = RG_RESOURCE_ID_NULL;

    using RGTlasID = RGResourceID;
    constexpr RGTlasID RG_TLAS_ID_NULL = RG_RESOURCE_ID_NULL;

    struct RGInternalTexture;
    struct RGExternalTexture;
    struct RGInternalBuffer;
    struct RGExternalBuffer;
    struct RGExternalTlas;
    class RenderGraphExecution;

  } // namespace impl

  class RenderGraph;
  class RenderGraphRegistry;
  class PassBaseNode;
  template <PipelineFlags pipeline_flags>
  class RGDependencyBuilder;

  template <typename Func, typename Data, PipelineFlags pipeline_flags>
  concept pass_setup =
    std::invocable<Func, Data&, RGDependencyBuilder<pipeline_flags>&> &&
    std::same_as<void, std::invoke_result_t<Func, Data&, RGDependencyBuilder<pipeline_flags>&>>;

  template <typename Func, typename Data>
  concept non_shader_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_NON_SHADER>;

  template <typename Func, typename Data>
  concept raster_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_RASTER>;

  template <typename Func, typename Data>
  concept compute_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_COMPUTE>;

  template <typename Func, typename Data>
  concept ray_tracing_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_RAY_TRACING>;

  template <typename Func, typename Data, PipelineFlags pipeline_flags>
  concept pass_executable =
    std::invocable<Func, const Data&, RenderGraphRegistry&, CommandList<pipeline_flags>&> &&
    std::same_as<
      void,
      std::invoke_result_t<Func, const Data&, RenderGraphRegistry&, CommandList<pipeline_flags>&>>;

  template <typename Func, typename Data>
  concept non_shader_pass_executable = pass_executable<Func, Data, PIPELINE_FLAGS_NON_SHADER>;

  template <typename Func, typename Data>
  concept raster_pass_executable = pass_executable<Func, Data, PIPELINE_FLAGS_RASTER>;

  template <typename Func, typename Data>
  concept compute_pass_executable = pass_executable<Func, Data, PIPELINE_FLAGS_COMPUTE>;

  template <typename Func, typename Data>
  concept ray_tracing_pass_executable = pass_executable<Func, Data, PIPELINE_FLAGS_RAY_TRACING>;

  // ID
  using PassNodeID = ID<PassBaseNode, u16>;
  using ResourceNodeID = ID<impl::ResourceNode, u16>;

  enum class RGResourceType : u8 { BUFFER, TEXTURE, TLAS, BLAS_GROUP, USER_RESOURCE, COUNT };

  template <RGResourceType resource_type>
  struct TypedResourceNodeID {
    using this_type = TypedResourceNodeID<resource_type>;
    ResourceNodeID id;

    friend auto operator<=>(const this_type&, const this_type&) = default;
    [[nodiscard]]
    auto is_null() const -> b8
    {
      return id.is_null();
    }
    [[nodiscard]]
    auto is_valid() const -> b8
    {
      return id.is_valid();
    }
  };

  using BufferNodeID = TypedResourceNodeID<RGResourceType::BUFFER>;
  using TextureNodeID = TypedResourceNodeID<RGResourceType::TEXTURE>;
  using TlasNodeID = TypedResourceNodeID<RGResourceType::TLAS>;
  using BlasGroupNodeID = TypedResourceNodeID<RGResourceType::BLAS_GROUP>;
  using UserResourceNodeID = TypedResourceNodeID<RGResourceType::USER_RESOURCE>;

  struct RGTextureDesc {
    TextureType type = TextureType::D2;
    TextureFormat format = TextureFormat::RGBA8;
    vec3ui32 extent;
    u32 mip_levels = 1;
    u16 layer_count = 1;
    TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
    b8 clear = false;
    ClearValue clear_value;

    static auto create_d2(
      const TextureFormat format,
      const u32 mip_levels,
      const vec2ui32 dimension,
      b8 clear = false,
      ClearValue clear_value = {},
      const TextureSampleCount sample_count = TextureSampleCount::COUNT_1) -> RGTextureDesc
    {
      return {
        .type = TextureType::D2,
        .format = format,
        .extent = vec3ui32(dimension.x, dimension.y, 1),
        .mip_levels = mip_levels,
        .layer_count = 1,
        .sample_count = sample_count,
        .clear = clear,
        .clear_value = clear_value};
    }

    static auto create_d3(
      const TextureFormat format,
      const u32 mip_levels,
      const vec3ui32 dimension,
      b8 clear = false,
      ClearValue clear_value = {},
      const TextureSampleCount sample_count = TextureSampleCount::COUNT_1) -> RGTextureDesc
    {
      return {
        .type = TextureType::D3,
        .format = format,
        .extent = dimension,
        .mip_levels = mip_levels,
        .layer_count = 1,
        .sample_count = sample_count,
        .clear = clear,
        .clear_value = clear_value};
    }

    static auto create_d2_array(
      const TextureFormat format,
      const u32 mip_levels,
      const vec2ui32 dimension,
      const u16 layer_count,
      b8 clear = false,
      ClearValue clear_value = {}) -> RGTextureDesc
    {
      return {
        .type = TextureType::D2_ARRAY,
        .format = format,
        .extent = vec3ui32(dimension.x, dimension.y, 1),
        .mip_levels = mip_levels,
        .layer_count = layer_count,
        .clear = clear,
        .clear_value = clear_value};
    }
  };

  struct RGBufferDesc {
    usize size = 0;
  };

  struct ColorAttachmentDesc {
    TextureNodeID node_id;
    SubresourceIndex view = SubresourceIndex();
    b8 clear = false;
    ClearValue clear_value;
  };

  struct DepthStencilAttachmentDesc {
    TextureNodeID node_id;
    SubresourceIndex view;
    b8 depth_write_enable = true;
    b8 clear = false;
    ClearValue clear_value;
  };

  struct ResolveAttachmentDesc {
    TextureNodeID node_id;
    SubresourceIndex view;
    b8 clear = false;
    ClearValue clear_value;
  };

  enum class ShaderBufferReadUsage : u8 { UNIFORM, STORAGE, COUNT };

  struct ShaderBufferReadAccess {
    BufferNodeID node_id;
    ShaderStageFlags stage_flags;
    ShaderBufferReadUsage usage;
  };

  enum class ShaderBufferWriteUsage : u8 { UNIFORM, STORAGE, COUNT };

  struct ShaderBufferWriteAccess {
    BufferNodeID input_node_id;
    BufferNodeID output_node_id;
    ShaderStageFlags stage_flags;
    ShaderBufferWriteUsage usage;
  };

  enum class ShaderTextureReadUsage : u8 { UNIFORM, STORAGE, COUNT };

  struct ShaderTextureReadAccess {
    TextureNodeID node_id;
    ShaderStageFlags stage_flags;
    ShaderTextureReadUsage usage;
    SubresourceIndexRange view_range;
  };

  enum class ShaderTextureWriteUsage : u8 { STORAGE, COUNT };

  struct ShaderTextureWriteAccess {
    TextureNodeID input_node_id;
    TextureNodeID output_node_id;
    ShaderStageFlags stage_flags;
    ShaderTextureWriteUsage usage;
    SubresourceIndexRange view_range;
  };

  struct ShaderTlasReadAccess {
    TlasNodeID node_id;
    ShaderStageFlags stage_flags;
  };

  struct ShaderBlasGroupReadAccess {
    BlasGroupNodeID node_id;
    ShaderStageFlags stage_flags;
  };

  template <typename AttachmentDesc>
  struct AttachmentAccess {
    TextureNodeID out_node_id;
    AttachmentDesc desc;
  };

  using ColorAttachment = AttachmentAccess<ColorAttachmentDesc>;
  using DepthStencilAttachment = AttachmentAccess<DepthStencilAttachmentDesc>;
  using ResolveAttachment = AttachmentAccess<ResolveAttachmentDesc>;

  struct TransferSrcBufferAccess {
    BufferNodeID node_id;
  };

  enum class TransferDataSource : u8 { GPU, CPU, COUNT };

  struct TransferDstBufferAccess {
    TransferDataSource data_source = TransferDataSource::COUNT;
    BufferNodeID input_node_id;
    BufferNodeID output_node_id;
  };

  struct TransferSrcTextureAccess {
    TextureNodeID node_id;
    SubresourceIndexRange view_range;
  };

  struct TransferDstTextureAccess {
    TransferDataSource data_source = TransferDataSource::COUNT;
    TextureNodeID input_node_id;
    TextureNodeID output_node_id;
    SubresourceIndexRange view_range;
  };

  struct AsBuildDstTlasAccess {
    TlasNodeID input_node_id;
    TlasNodeID output_node_id;
  };

  struct AsBuildDstBlasGroupAccess {
    BlasGroupNodeID input_node_id;
    BlasGroupNodeID output_node_id;
  };

  namespace impl
  {
    struct RGInternalTexture {
      const char* name = nullptr;
      TextureType type = TextureType::D2;
      TextureFormat format = TextureFormat::COUNT;
      vec3ui32 extent = {};
      u32 mip_levels = 1;
      u16 layer_count = 1;
      TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
      b8 clear = false;
      ClearValue clear_value = {};

      [[nodiscard]]
      auto get_view_count() const -> usize
      {
        return soul::cast<u64>(mip_levels) * layer_count;
      }
    };

    struct RGExternalTexture {
      const char* name = nullptr;
      TextureID texture_id;
      b8 clear = false;
      ClearValue clear_value = {};
    };

    struct RGInternalBuffer {
      const char* name = nullptr;
      usize size = 0;
      b8 clear = false;
    };

    struct RGExternalBuffer {
      const char* name = nullptr;
      BufferID buffer_id;
      b8 clear = false;
    };

    struct RGExternalTlas {
      const char* name = nullptr;
      TlasID tlas_id;
    };

    struct RGExternalBlasGroup {
      const char* name = nullptr;
      BlasGroupID blas_group_id;
    };

    struct ResourceNode {
      RGResourceType resource_type;
      RGResourceID resource_id;
      PassNodeID creator;
      PassNodeID writer;
      ResourceNodeID write_target_node;
      Vector<PassNodeID> readers;

      ResourceNode(RGResourceType resource_type, RGResourceID resource_id)
          : resource_type(resource_type), resource_id(resource_id)
      {
      }

      ResourceNode(RGResourceType resource_type, RGResourceID resource_id, PassNodeID creator)
          : resource_type(resource_type), resource_id(resource_id), creator(creator)
      {
      }
    };

  } // namespace impl

  struct RGRenderTargetDesc {

    using ColorAttachments = SBOVector<ColorAttachmentDesc, 1>;
    using ResolveAttachments = SBOVector<ResolveAttachmentDesc, 1>;

    RGRenderTargetDesc() = default;

    RGRenderTargetDesc(vec2ui32 dimension, const ColorAttachmentDesc& color)
        : dimension(dimension), color_attachments(ColorAttachments::fill_n(1, color))
    {
    }

    RGRenderTargetDesc(
      vec2ui32 dimension,
      const ColorAttachmentDesc& color,
      const DepthStencilAttachmentDesc& depth_stencil)
        : dimension(dimension),
          color_attachments(ColorAttachments::fill_n(1, color)),
          depth_stencil_attachment(depth_stencil)
    {
    }

    RGRenderTargetDesc(
      vec2ui32 dimension,
      const TextureSampleCount sample_count,
      const ColorAttachmentDesc& color,
      const ResolveAttachmentDesc& resolve,
      const DepthStencilAttachmentDesc depth_stencil)
        : dimension(dimension),
          sample_count(sample_count),
          color_attachments(ColorAttachments::fill_n(1, color)),
          resolve_attachments(ResolveAttachments::fill_n(1, resolve)),
          depth_stencil_attachment(depth_stencil)
    {
    }

    RGRenderTargetDesc(vec2ui32 dimension, const DepthStencilAttachmentDesc& depth_stencil)
        : dimension(dimension), depth_stencil_attachment(depth_stencil)
    {
    }

    vec2ui32 dimension = {};
    TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
    SBOVector<ColorAttachmentDesc, 1> color_attachments;
    SBOVector<ResolveAttachmentDesc, 1> resolve_attachments;
    DepthStencilAttachmentDesc depth_stencil_attachment;
  };

  struct RGRenderTarget {
    vec2ui32 dimension = {};
    TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
    Vector<ColorAttachment> color_attachments;
    Vector<ResolveAttachment> resolve_attachments;
    DepthStencilAttachment depth_stencil_attachment;
  };

  template <PipelineFlags pipeline_flags>
  class RGDependencyBuilder
  {
  public:
    static constexpr auto PIPELINE_FLAGS = pipeline_flags;
    RGDependencyBuilder(PassNodeID pass_id, PassBaseNode& pass_node, RenderGraph& render_graph);
    auto add_shader_buffer(
      BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferReadUsage usage_type)
      -> BufferNodeID;
    auto add_shader_buffer(
      BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferWriteUsage usage_type)
      -> BufferNodeID;
    auto add_shader_texture(
      TextureNodeID node_id,
      ShaderStageFlags stage_flags,
      ShaderTextureReadUsage usage_type,
      SubresourceIndexRange view = SubresourceIndexRange()) -> TextureNodeID;
    auto add_shader_texture(
      TextureNodeID node_id,
      ShaderStageFlags stage_flags,
      ShaderTextureWriteUsage usage_type,
      SubresourceIndexRange view = SubresourceIndexRange()) -> TextureNodeID;
    auto add_shader_tlas(TlasNodeID node_id, ShaderStageFlags stage_flags) -> TlasNodeID;
    auto add_shader_blas_group(BlasGroupNodeID node_id, ShaderStageFlags stage_flags)
      -> BlasGroupNodeID;
    auto add_vertex_buffer(BufferNodeID node_id) -> BufferNodeID;
    auto add_index_buffer(BufferNodeID node_id) -> BufferNodeID;

    auto add_src_buffer(BufferNodeID node_id) -> BufferNodeID;
    auto add_dst_buffer(
      BufferNodeID node_id, TransferDataSource data_source = TransferDataSource::GPU)
      -> BufferNodeID;
    auto add_src_texture(TextureNodeID node_id) -> TextureNodeID;
    auto add_dst_texture(
      TextureNodeID node_id, TransferDataSource data_source = TransferDataSource::GPU)
      -> TextureNodeID;

    auto add_as_build_input(BufferNodeID node_id) -> BufferNodeID;
    auto add_as_build_input(BlasGroupNodeID node_id) -> BlasGroupNodeID;
    auto add_as_build_dst(TlasNodeID node_id) -> TlasNodeID;
    auto add_as_build_dst(BlasGroupNodeID node_id) -> BlasGroupNodeID;

    auto set_render_target(const RGRenderTargetDesc& render_target_desc) -> void;

  private:
    const PassNodeID pass_id_;
    PassBaseNode& pass_node_;
    RenderGraph& render_graph_;
  };

  class PassBaseNode
  {
  public:
    PassBaseNode(const char* name, const PipelineFlags pipeline_flags, const QueueType queue_type)
        : name_(name), pipeline_flags_(pipeline_flags), queue_type_(queue_type)
    {
    }

    PassBaseNode(const PassBaseNode&) = delete;
    auto operator=(const PassBaseNode&) -> PassBaseNode& = delete;
    PassBaseNode(PassBaseNode&&) = delete;
    auto operator=(PassBaseNode&&) -> PassBaseNode& = delete;
    virtual ~PassBaseNode() = default;

    [[nodiscard]]
    auto get_name() const -> const char*
    {
      return name_;
    }
    [[nodiscard]]
    auto get_pipeline_flags() const -> PipelineFlags
    {
      return pipeline_flags_;
    }
    [[nodiscard]]
    auto get_queue_type() const -> QueueType
    {
      return queue_type_;
    }

    [[nodiscard]]
    auto get_vertex_buffers() const -> std::span<const BufferNodeID>
    {
      return vertex_buffers_;
    }
    [[nodiscard]]
    auto get_index_buffers() const -> std::span<const BufferNodeID>
    {
      return index_buffers_;
    }

    [[nodiscard]]
    auto get_buffer_read_accesses() const -> std::span<const ShaderBufferReadAccess>
    {
      return shader_buffer_read_accesses_;
    }

    [[nodiscard]]
    auto get_buffer_write_accesses() const -> std::span<const ShaderBufferWriteAccess>
    {
      return shader_buffer_write_accesses_;
    }

    [[nodiscard]]
    auto get_texture_read_accesses() const -> std::span<const ShaderTextureReadAccess>
    {
      return shader_texture_read_accesses_;
    }

    [[nodiscard]]
    auto get_texture_write_accesses() const -> std::span<const ShaderTextureWriteAccess>
    {
      return shader_texture_write_accesses_;
    }

    [[nodiscard]]
    auto get_shader_tlas_read_accesses() const -> std::span<const ShaderTlasReadAccess>
    {
      return shader_tlas_read_accesses_;
    }

    [[nodiscard]]
    auto get_shader_blas_group_read_accesses() const -> std::span<const ShaderBlasGroupReadAccess>
    {
      return shader_blas_group_read_accesses_;
    }

    [[nodiscard]]
    auto get_source_buffers() const -> std::span<const TransferSrcBufferAccess>
    {
      return source_buffers_;
    }

    [[nodiscard]]
    auto get_destination_buffers() const -> std::span<const TransferDstBufferAccess>
    {
      return destination_buffers_;
    }

    [[nodiscard]]
    auto get_source_textures() const -> std::span<const TransferSrcTextureAccess>
    {
      return source_textures_;
    }

    [[nodiscard]]
    auto get_destination_textures() const -> std::span<const TransferDstTextureAccess>
    {
      return destination_textures_;
    }

    [[nodiscard]]
    auto get_as_build_input_buffers() const -> std::span<const BufferNodeID>
    {
      return as_build_input_buffers_;
    }

    [[nodiscard]]
    auto get_as_build_input_blas_groups() const -> std::span<const BlasGroupNodeID>
    {
      return as_build_input_blas_groups_;
    }

    [[nodiscard]]
    auto get_as_build_destination_tlas_list() const -> std::span<const AsBuildDstTlasAccess>
    {
      return as_build_dst_tlas_list_;
    }

    [[nodiscard]]
    auto get_as_build_destination_blas_group_list() const
      -> std::span<const AsBuildDstBlasGroupAccess>
    {
      return as_build_dst_blas_group_list_;
    }

    [[nodiscard]]
    auto get_render_target() const -> const RGRenderTarget&
    {
      return render_target_;
    }

    [[nodiscard]]
    auto get_color_attachment_node_id() const -> TextureNodeID
    {
      return render_target_.color_attachments[0].out_node_id;
    }

  protected:
    const char* name_ = nullptr;
    const PipelineFlags pipeline_flags_;
    const QueueType queue_type_;

  private:
    virtual auto execute(
      RenderGraphRegistry& registry,
      impl::RenderCompiler& render_compiler,
      const VkRenderPassBeginInfo* render_pass_begin_info,
      impl::CommandPools& command_pools,
      System& gpu_system) const -> void = 0;

    Vector<TransferSrcBufferAccess> source_buffers_;
    Vector<TransferDstBufferAccess> destination_buffers_;
    Vector<TransferSrcTextureAccess> source_textures_;
    Vector<TransferDstTextureAccess> destination_textures_;

    Vector<BufferNodeID> as_build_input_buffers_;
    Vector<BlasGroupNodeID> as_build_input_blas_groups_;
    Vector<AsBuildDstTlasAccess> as_build_dst_tlas_list_;
    Vector<AsBuildDstBlasGroupAccess> as_build_dst_blas_group_list_;

    Vector<ShaderBufferReadAccess> shader_buffer_read_accesses_;
    Vector<ShaderBufferWriteAccess> shader_buffer_write_accesses_;
    Vector<ShaderTextureReadAccess> shader_texture_read_accesses_;
    Vector<ShaderTextureWriteAccess> shader_texture_write_accesses_;
    Vector<ShaderTlasReadAccess> shader_tlas_read_accesses_;
    Vector<ShaderBlasGroupReadAccess> shader_blas_group_read_accesses_;
    Vector<BufferNodeID> vertex_buffers_;
    Vector<BufferNodeID> index_buffers_;

    RGRenderTarget render_target_;

    template <PipelineFlags pipeline_flags>
    friend class RGDependencyBuilder;

    friend class impl::RenderGraphExecution;
  };

  template <PipelineFlags pipeline_flags, typename Parameter, typename Execute>
  class PassNode final : public PassBaseNode
  {
  public:
    PassNode() = delete;
    PassNode(const PassNode&) = delete;
    auto operator=(const PassNode&) -> PassNode& = delete;
    PassNode(PassNode&&) = delete;
    auto operator=(PassNode&&) -> PassNode& = delete;
    ~PassNode() override = default;

    PassNode(const char* name, const QueueType queue_type, Execute&& execute)
        : PassBaseNode(name, pipeline_flags, queue_type), execute_(std::forward<Execute>(execute))
    {
    }

    [[nodiscard]]
    auto get_parameter() const -> const Parameter&
    {
      return parameter_;
    }
    auto execute(
      RenderGraphRegistry& render_graph_execution,
      impl::RenderCompiler& render_compiler,
      const VkRenderPassBeginInfo* render_pass_begin_info,
      impl::CommandPools& command_pools,
      System& gpu_system) const -> void override;

  private:
    auto get_parameter() -> Parameter& { return parameter_; }

    Parameter parameter_;
    Execute execute_;
    friend class RenderGraph;
  };

  class RenderGraph
  {
  public:
    explicit RenderGraph(memory::Allocator* allocator = get_default_allocator())
        : allocator_(allocator)
    {
    }

    RenderGraph(const RenderGraph& other) = delete;
    RenderGraph(RenderGraph&& other) = delete;
    auto operator=(const RenderGraph& other) -> RenderGraph& = delete;
    auto operator=(RenderGraph&& other) -> RenderGraph& = delete;
    ~RenderGraph();

    template <typename Parameter, PipelineFlags pipeline_flags, typename Setup, typename Executable>
    auto add_pass(const char* name, QueueType queue_type, Setup&& setup, Executable&& execute)
      -> const auto&
    {
      static_assert(pass_setup<Setup, Parameter, pipeline_flags>);
      static_assert(pass_executable<Executable, Parameter, pipeline_flags>);

      using Node = PassNode<pipeline_flags, Parameter, Executable>;
      auto node = allocator_->create<Node>(name, queue_type, std::forward<Executable>(execute));
      pass_nodes_.push_back(node);
      RGDependencyBuilder<pipeline_flags> builder(PassNodeID(pass_nodes_.size() - 1), *node, *this);
      setup(node->get_parameter(), builder);
      return *node;
    }

    template <typename Parameter, typename Setup, typename Executable>
    auto add_raster_pass(
      const char* name,
      const RGRenderTargetDesc& render_target,
      Setup&& setup,
      Executable&& execute) -> const auto&
    {
      static_assert(raster_pass_setup<Setup, Parameter>);
      static_assert(raster_pass_executable<Executable, Parameter>);

      constexpr auto pipeline_flags = PipelineFlags{PipelineType::RASTER};
      using Node = PassNode<pipeline_flags, Parameter, Executable>;
      auto* node =
        allocator_->create<Node>(name, QueueType::GRAPHIC, std::forward<Executable>(execute));
      pass_nodes_.push_back(node);
      const auto pass_node_id = PassNodeID(pass_nodes_.size() - 1);
      RGDependencyBuilder<pipeline_flags> builder(pass_node_id, *node, *this);
      builder.set_render_target(render_target);
      setup(node->get_parameter(), builder);

      return *node;
    }

    template <typename Parameter, typename Setup, typename Executable>
    auto add_compute_pass(const char* name, Setup&& setup, Executable&& execute) -> const auto&
    {
      static_assert(compute_pass_setup<Setup, Parameter>);
      static_assert(compute_pass_executable<Executable, Parameter>);

      return add_pass<Parameter, PIPELINE_FLAGS_COMPUTE>(
        name, QueueType::COMPUTE, std::forward<Setup>(setup), std::forward<Executable>(execute));
    }

    template <typename Parameter, typename Setup, typename Executable>
    auto add_non_shader_pass(
      const char* name, QueueType queue_type, Setup&& setup, Executable&& execute) -> const auto&
    {
      static_assert(non_shader_pass_setup<Setup, Parameter>);
      static_assert(non_shader_pass_executable<Executable, Parameter>);

      return add_pass<Parameter, PIPELINE_FLAGS_NON_SHADER>(
        name, queue_type, std::forward<Setup>(setup), std::forward<Executable>(execute));
    }

    template <typename Parameter, typename Setup, typename Executable>
    auto add_ray_tracing_pass(const char* name, Setup&& setup, Executable&& execute) -> const auto&
    {
      static_assert(ray_tracing_pass_setup<Setup, Parameter>);
      static_assert(ray_tracing_pass_executable<Executable, Parameter>);

      return add_pass<Parameter, PIPELINE_FLAGS_RAY_TRACING>(
        name, QueueType::COMPUTE, std::forward<Setup>(setup), std::forward<Executable>(execute));
    }

    auto import_texture(const char* name, TextureID texture_id) -> TextureNodeID;
    auto create_texture(const char* name, const RGTextureDesc& desc) -> TextureNodeID;

    auto import_buffer(const char* name, BufferID buffer_id) -> BufferNodeID;
    auto create_buffer(const char* name, const RGBufferDesc& desc) -> BufferNodeID;

    auto import_tlas(const char* name, TlasID tlas_id) -> TlasNodeID;
    auto import_blas_group(const char* name, BlasGroupID blas_group_id) -> BlasGroupNodeID;

    [[nodiscard]]
    auto get_pass_nodes() const -> const Vector<PassBaseNode*>&
    {
      return pass_nodes_;
    }

    [[nodiscard]]
    auto get_internal_buffers() const -> const Vector<impl::RGInternalBuffer>&
    {
      return internal_buffers_;
    }

    [[nodiscard]]
    auto get_internal_textures() const -> const Vector<impl::RGInternalTexture>&
    {
      return internal_textures_;
    }

    [[nodiscard]]
    auto get_external_buffers() const -> const Vector<impl::RGExternalBuffer>&
    {
      return external_buffers_;
    }

    [[nodiscard]]
    auto get_external_textures() const -> const Vector<impl::RGExternalTexture>&
    {
      return external_textures_;
    }

    [[nodiscard]]
    auto get_external_tlas_list() const -> std::span<const impl::RGExternalTlas>
    {
      return external_tlas_list_;
    }

    [[nodiscard]]
    auto get_external_blas_group_list() const -> std::span<const impl::RGExternalBlasGroup>
    {
      return external_blas_group_list_;
    }

    [[nodiscard]]
    auto get_texture_desc(TextureNodeID node_id, const System& gpu_system) const -> RGTextureDesc;
    [[nodiscard]]
    auto get_buffer_desc(BufferNodeID node_id, const System& gpu_system) const -> RGBufferDesc;

  private:
    Vector<PassBaseNode*> pass_nodes_;

    Vector<impl::ResourceNode> resource_nodes_;

    Vector<impl::RGInternalBuffer> internal_buffers_;
    Vector<impl::RGInternalTexture> internal_textures_;

    Vector<impl::RGExternalBuffer> external_buffers_;
    Vector<impl::RGExternalTexture> external_textures_;

    Vector<impl::RGExternalTlas> external_tlas_list_;
    Vector<impl::RGExternalBlasGroup> external_blas_group_list_;

    memory::Allocator* allocator_;

    friend class impl::RenderGraphExecution;
    template <PipelineFlags pipeline_flags>
    friend class RGDependencyBuilder;

    auto create_resource_node(RGResourceType resource_type, impl::RGResourceID resource_id)
      -> ResourceNodeID;
    auto read_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id) -> void;
    auto write_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id)
      -> ResourceNodeID;
    [[nodiscard]]
    auto get_resource_node(ResourceNodeID node_id) const -> const impl::ResourceNode&;
    auto get_resource_node(ResourceNodeID node_id) -> impl::ResourceNode&;
    [[nodiscard]]
    auto get_resource_nodes() const -> std::span<const impl::ResourceNode>;

    template <RGResourceType resource_type>
    auto create_resource_node(impl::RGResourceID resource_id) -> TypedResourceNodeID<resource_type>
    {
      return {create_resource_node(resource_type, resource_id)};
    }

    template <RGResourceType resource_type>
    auto read_resource_node(TypedResourceNodeID<resource_type> node_id, PassNodeID pass_node_id)
      -> void
    {
      read_resource_node(node_id.id, pass_node_id);
    }

    template <RGResourceType resource_type>
    auto write_resource_node(
      TypedResourceNodeID<resource_type> resource_node_id, PassNodeID pass_node_id)
      -> TypedResourceNodeID<resource_type>
    {
      return {write_resource_node(resource_node_id.id, pass_node_id)};
    }

    template <RGResourceType resource_type>
    auto get_resource_node(TypedResourceNodeID<resource_type> node_id) const
      -> const impl::ResourceNode&
    {
      return get_resource_node(node_id.id);
    }
  };

  template <PipelineFlags pipeline_flags>
  RGDependencyBuilder<pipeline_flags>::RGDependencyBuilder(
    const PassNodeID pass_id, PassBaseNode& pass_node, RenderGraph& render_graph)
      : pass_id_(pass_id), pass_node_(pass_node), render_graph_(render_graph)
  {
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_shader_buffer(
    BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferReadUsage usage_type)
    -> BufferNodeID
  {
    static_assert(pipeline_flags.test_any(
      {PipelineType::RASTER, PipelineType::COMPUTE, PipelineType::RAY_TRACING}));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.shader_buffer_read_accesses_.push_back(
      ShaderBufferReadAccess{node_id, stage_flags, usage_type});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_shader_buffer(
    BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferWriteUsage usage_type)
    -> BufferNodeID
  {
    static_assert(pipeline_flags.test_any(
      {PipelineType::RASTER, PipelineType::COMPUTE, PipelineType::RAY_TRACING}));
    const auto out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
    pass_node_.shader_buffer_write_accesses_.push_back(
      ShaderBufferWriteAccess{node_id, out_node_id, stage_flags, usage_type});
    return out_node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_shader_texture(
    TextureNodeID node_id,
    ShaderStageFlags stage_flags,
    ShaderTextureReadUsage usage_type,
    SubresourceIndexRange view) -> TextureNodeID
  {
    static_assert(pipeline_flags.test_any(
      {PipelineType::RASTER, PipelineType::COMPUTE, PipelineType::RAY_TRACING}));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.shader_texture_read_accesses_.push_back(
      ShaderTextureReadAccess{node_id, stage_flags, usage_type, view});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_shader_texture(
    TextureNodeID node_id,
    ShaderStageFlags stage_flags,
    ShaderTextureWriteUsage usage_type,
    SubresourceIndexRange view) -> TextureNodeID
  {
    static_assert(pipeline_flags.test_any(
      {PipelineType::RASTER, PipelineType::COMPUTE, PipelineType::RAY_TRACING}));
    const auto out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
    pass_node_.shader_texture_write_accesses_.push_back(
      ShaderTextureWriteAccess{node_id, out_node_id, stage_flags, usage_type, view});
    return out_node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_shader_tlas(
    TlasNodeID node_id, ShaderStageFlags stage_flags) -> TlasNodeID
  {
    static_assert(pipeline_flags.test_any({PipelineType::RAY_TRACING}));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.shader_tlas_read_accesses_.push_back(ShaderTlasReadAccess{node_id, stage_flags});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_shader_blas_group(
    BlasGroupNodeID node_id, ShaderStageFlags stage_flags) -> BlasGroupNodeID
  {
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.shader_blas_group_read_accesses_.push_back(
      ShaderBlasGroupReadAccess{node_id, stage_flags});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_vertex_buffer(BufferNodeID node_id) -> BufferNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::RASTER));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.vertex_buffers_.push_back(node_id);
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_index_buffer(BufferNodeID node_id) -> BufferNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::RASTER));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.index_buffers_.push_back(node_id);
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_src_buffer(BufferNodeID node_id) -> BufferNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.source_buffers_.push_back(TransferSrcBufferAccess{node_id});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_dst_buffer(
    BufferNodeID node_id, TransferDataSource data_source) -> BufferNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    BufferNodeID out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
    pass_node_.destination_buffers_.push_back(TransferDstBufferAccess{
      .data_source = data_source, .input_node_id = node_id, .output_node_id = out_node_id});
    return out_node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_src_texture(TextureNodeID node_id) -> TextureNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.source_textures_.push_back(TransferSrcTextureAccess{node_id});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_dst_texture(
    TextureNodeID node_id, TransferDataSource data_source) -> TextureNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    TextureNodeID out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
    pass_node_.destination_textures_.push_back(TransferDstTextureAccess{
      .data_source = data_source, .input_node_id = node_id, .output_node_id = out_node_id});
    return out_node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_as_build_input(BufferNodeID node_id) -> BufferNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.as_build_input_buffers_.push_back({node_id});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_as_build_input(BlasGroupNodeID node_id)
    -> BlasGroupNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    render_graph_.read_resource_node(node_id, pass_id_);
    pass_node_.as_build_input_blas_groups_.push_back({node_id});
    return node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_as_build_dst(TlasNodeID node_id) -> TlasNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    const auto out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
    pass_node_.as_build_dst_tlas_list_.push_back(
      AsBuildDstTlasAccess{.input_node_id = node_id, .output_node_id = out_node_id});
    return out_node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::add_as_build_dst(BlasGroupNodeID node_id)
    -> BlasGroupNodeID
  {
    static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
    const auto out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
    pass_node_.as_build_dst_blas_group_list_.push_back(
      AsBuildDstBlasGroupAccess{.input_node_id = node_id, .output_node_id = out_node_id});
    return out_node_id;
  }

  template <PipelineFlags pipeline_flags>
  auto RGDependencyBuilder<pipeline_flags>::set_render_target(
    const RGRenderTargetDesc& render_target_desc) -> void
  {
    static_assert(pipeline_flags.test(PipelineType::RASTER));
    auto to_output_attachment =
      [this]<typename AttachmentDesc>(const AttachmentDesc attachment_desc) -> auto {
      AttachmentAccess<AttachmentDesc> access = {
        .out_node_id = render_graph_.write_resource_node(attachment_desc.node_id, pass_id_),
        .desc = attachment_desc};
      return access;
    };

    std::ranges::transform(
      render_target_desc.color_attachments,
      std::back_inserter(pass_node_.render_target_.color_attachments),
      to_output_attachment);
    std::ranges::transform(
      render_target_desc.resolve_attachments,
      std::back_inserter(pass_node_.render_target_.resolve_attachments),
      to_output_attachment);
    if (render_target_desc.depth_stencil_attachment.node_id.is_valid()) {
      const auto& depth_desc = render_target_desc.depth_stencil_attachment;
      const TextureNodeID out_node_id =
        depth_desc.depth_write_enable
          ? depth_desc.node_id
          : render_graph_.write_resource_node(depth_desc.node_id, pass_id_);
      pass_node_.render_target_.depth_stencil_attachment = {
        .out_node_id = out_node_id, .desc = depth_desc};
    }
    pass_node_.render_target_.dimension = render_target_desc.dimension;
    pass_node_.render_target_.sample_count = render_target_desc.sample_count;
  }

  template <PipelineFlags pipeline_flags, typename Parameter, typename Execute>
  auto PassNode<pipeline_flags, Parameter, Execute>::execute(
    RenderGraphRegistry& registry,
    impl::RenderCompiler& render_compiler,
    const VkRenderPassBeginInfo* render_pass_begin_info,
    impl::CommandPools& command_pools,
    System& gpu_system) const -> void
  {
    CommandList<pipeline_flags> command_list(
      render_compiler, render_pass_begin_info, command_pools, gpu_system);
    execute_(parameter_, registry, command_list);
  }

} // namespace soul::gpu
