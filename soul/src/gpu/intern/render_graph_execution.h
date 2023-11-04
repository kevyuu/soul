#pragma once

#include "gpu/render_graph.h"
#include "gpu/type.h"
#include "memory/allocator.h"

namespace soul::gpu::impl
{

  struct BufferAccess {
    PipelineStageFlags stage_flags;
    AccessFlags access_flags;
    u32 buffer_info_idx = 0;
  };

  struct TextureAccess {
    PipelineStageFlags stage_flags;
    AccessFlags access_flags;

    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    u32 texture_info_idx = 0;
    SubresourceIndex view;
  };

  struct ResourceAccess {
    PipelineStageFlags stage_flags;
    AccessFlags access_flags;
    u32 resource_info_idx;
  };

  struct BufferExecInfo {
    PassNodeID first_pass;
    PassNodeID last_pass;
    BufferUsageFlags usage_flags;
    QueueFlags queue_flags;
    BufferID buffer_id;

    VkEvent pending_event = VK_NULL_HANDLE;
    Semaphore pending_semaphore = Semaphore::From(TimelineSemaphore::null());
    ResourceCacheState cache_state;

    Vector<PassNodeID> passes;
    u32 pass_counter = 0;
  };

  struct TextureViewExecInfo {
    VkEvent pending_event = VK_NULL_HANDLE;
    Semaphore pending_semaphore = Semaphore::From(TimelineSemaphore::null());
    ResourceCacheState cache_state;
    Vector<PassNodeID> passes;
    u32 pass_counter = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
  };

  struct TextureExecInfo {
    PassNodeID first_pass;
    PassNodeID last_pass;
    TextureUsageFlags usage_flags;
    QueueFlags queue_flags;
    TextureID texture_id;
    TextureViewExecInfo* view = nullptr;
    u32 mip_levels = 0;
    u32 layers = 0;

    [[nodiscard]]
    auto get_view_count() const -> usize
    {
      return soul::cast<usize>(mip_levels) * layers;
    }

    [[nodiscard]]
    auto get_view_index(const SubresourceIndex index) const -> usize
    {
      return index.get_layer() * mip_levels + index.get_level();
    }

    [[nodiscard]]
    auto get_view(const SubresourceIndex index) const -> TextureViewExecInfo*
    {
      return view + get_view_index(index);
    }
  };

  struct ResourceExecInfo {
    PassNodeID first_pass;
    PassNodeID last_pass;
    QueueFlags queue_flags;

    VkEvent pending_event = VK_NULL_HANDLE;
    Semaphore pending_semaphore = Semaphore::From(TimelineSemaphore::null());
    ResourceCacheState cache_state;

    Vector<PassNodeID> passes;
    u32 pass_counter = 0;
  };

  class PassDependencyGraph
  {
  public:
    enum class DependencyType : u8 { READ_AFTER_WRITE, WRITE_AFTER_WRITE, WRITE_AFTER_READ, COUNT };

    using DependencyFlags = FlagSet<DependencyType>;
    static constexpr DependencyFlags OP_AFTER_WRITE_DEPENDENCY = {
      DependencyType::READ_AFTER_WRITE, DependencyType::WRITE_AFTER_WRITE};

    using NodeList = Vector<PassNodeID>;
    using NodeDependencyMatrix = FlagMap<DependencyType, BitVector<>>;

    PassDependencyGraph(usize pass_node_count, std::span<const ResourceNode> resource_nodes);
    [[nodiscard]]
    auto get_dependency_flags(PassNodeID src_node_id, PassNodeID dst_node_id) const
      -> DependencyFlags;

    [[nodiscard]]
    auto get_dependencies(const PassNodeID node_id) const -> std::span<const PassNodeID>
    {
      return dependencies_[node_id.id];
    }

    [[nodiscard]]
    auto get_dependants(const PassNodeID node_id) const -> std::span<const PassNodeID>
    {
      return dependants_[node_id.id];
    }

    [[nodiscard]]
    auto get_dependency_level(PassNodeID node_id) const -> usize
    {
      return dependency_levels_[node_id.id];
    }

    auto set_dependency(
      PassNodeID src_node_id, PassNodeID dst_node_id, DependencyType dependency_type) -> void;

  private:
    usize pass_node_count_;
    NodeDependencyMatrix dependency_matrixes_;
    Vector<NodeList> dependencies_;
    Vector<NodeList> dependants_;
    Vector<usize> dependency_levels_;

    static constexpr auto UNINITIALIZED_DEPENDENCY_LEVEL = ~0u;
    [[nodiscard]]
    auto get_pass_node_count() const -> usize;
    [[nodiscard]]
    auto get_dependency_matrix_index(PassNodeID src_node_id, PassNodeID dst_node_id) const -> usize;

    auto calculate_dependency_level(PassNodeID pass_node_id) -> usize;
  };

  struct PassExecInfo {
    PassBaseNode* pass_node = nullptr;
    Vector<BufferAccess> buffer_accesses;
    Vector<TextureAccess> texture_accesses;
    Vector<ResourceAccess> resource_accesses;
  };

  class RenderGraphExecution
  {
  public:
    RenderGraphExecution(
      NotNull<const RenderGraph*> render_graph,
      NotNull<System*> system,
      NotNull<memory::Allocator*> allocator,
      NotNull<CommandQueues*> command_queues,
      NotNull<CommandPools*> command_pools)
        : render_graph_(render_graph),
          gpu_system_(system),
          command_queues_(command_queues),
          command_pools_(command_pools),
          buffer_infos_(allocator),
          texture_infos_(allocator),
          pass_infos_(allocator),
          pass_dependency_graph_(
            render_graph->get_pass_nodes().size(), render_graph->get_resource_nodes())
    {
    }

    RenderGraphExecution() = delete;
    RenderGraphExecution(const RenderGraphExecution& other) = delete;
    auto operator=(const RenderGraphExecution& other) -> RenderGraphExecution& = delete;

    RenderGraphExecution(RenderGraphExecution&& other) = delete;
    auto operator=(RenderGraphExecution&& other) -> RenderGraphExecution& = delete;

    ~RenderGraphExecution() = default;

    auto init() -> void;

    auto run() -> void;

    auto cleanup() -> void;

    [[nodiscard]]
    auto is_external(const BufferExecInfo& info) const -> b8;

    [[nodiscard]]
    auto is_external(const TextureExecInfo& info) const -> b8;

    [[nodiscard]]
    auto get_buffer_id(BufferNodeID node_id) const -> BufferID;

    [[nodiscard]]
    auto get_texture_id(TextureNodeID node_id) const -> TextureID;

    [[nodiscard]]
    auto get_tlas_id(TlasNodeID node_id) const -> TlasID;

    [[nodiscard]]
    auto get_buffer(BufferNodeID node_id) const -> Buffer&;

    [[nodiscard]]
    auto get_texture(TextureNodeID node_id) const -> Texture&;

    [[nodiscard]]
    auto get_buffer_info_index(BufferNodeID node_id) const -> u32;

    [[nodiscard]]
    auto get_texture_info_index(TextureNodeID nodeID) const -> u32;

    [[nodiscard]]
    auto get_tlas_resource_info_index(TlasNodeID node_id) const -> u32;

    [[nodiscard]]
    auto get_blas_group_resource_info_index(BlasGroupNodeID node_id) const -> u32;

  private:
    NotNull<const RenderGraph*> render_graph_;
    NotNull<System*> gpu_system_;

    FlagMap<QueueType, VkEvent> external_events_;
    FlagMap<QueueType, PipelineStageFlags> external_events_stage_flags_;
    NotNull<CommandQueues*> command_queues_;
    NotNull<CommandPools*> command_pools_;

    Vector<BufferExecInfo> buffer_infos_;
    Slice<BufferExecInfo> internal_buffer_infos_;
    Slice<BufferExecInfo> external_buffer_infos_;

    Vector<TextureExecInfo> texture_infos_;
    Slice<TextureExecInfo> internal_texture_infos_;
    Slice<TextureExecInfo> external_texture_infos_;
    Vector<TextureViewExecInfo> texture_view_infos_;

    Vector<ResourceExecInfo> resource_infos_;
    Slice<ResourceExecInfo> external_tlas_resource_infos_;
    Slice<ResourceExecInfo> external_blas_group_resource_infos_;

    Vector<PassExecInfo> pass_infos_;

    PassDependencyGraph pass_dependency_graph_;
    BitVector<> active_passes_;
    Vector<PassNodeID> pass_order_;

    auto compute_active_passes() -> void;

    auto compute_pass_order() -> void;

    [[nodiscard]]
    auto create_render_pass(u32 pass_index) -> VkRenderPass;

    [[nodiscard]]
    auto create_framebuffer(u32 pass_index, VkRenderPass render_pass) -> VkFramebuffer;

    auto sync_external() -> void;

    auto execute_pass(u32 pass_index, PrimaryCommandBuffer command_buffer) -> void;

    auto init_shader_buffers(
      std::span<const ShaderBufferReadAccess> access_list,
      PassNodeID pass_node_id,
      QueueType queue_type) -> void;

    auto init_shader_buffers(
      std::span<const ShaderBufferWriteAccess> access_list,
      PassNodeID pass_node_id,
      QueueType queue_type) -> void;

    auto init_shader_textures(
      std::span<const ShaderTextureReadAccess> access_list,
      PassNodeID pass_node_id,
      QueueType queue_type) -> void;

    auto init_shader_textures(
      std::span<const ShaderTextureWriteAccess> access_list,
      PassNodeID pass_node_id,
      QueueType queue_type) -> void;

    auto init_shader_tlas_accesses(
      std::span<const ShaderTlasReadAccess> access_list,
      PassNodeID pass_node_id,
      QueueType queue_type) -> void;

    auto init_shader_blas_group_accesses(
      std::span<const ShaderBlasGroupReadAccess> access_list,
      PassNodeID pass_node_id,
      QueueType queue_type) -> void;
  };

} // namespace soul::gpu::impl
