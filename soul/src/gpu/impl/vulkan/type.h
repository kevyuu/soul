#pragma once

#include "core/sbo_vector.h"

#include "gpu/type.h"

#include "gpu/impl/vulkan/bindless_descriptor_allocator.h"

namespace soul::gpu::impl
{

  class PrimaryCommandBuffer;

  struct GraphicPipelineStateKey
  {
    GraphicPipelineStateDesc desc;
    TextureSampleCount sample_count                               = TextureSampleCount::COUNT_1;
    auto operator==(const GraphicPipelineStateKey&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const GraphicPipelineStateKey& key)
    {
      hasher.combine(key.desc, key.sample_count);
    }
  };

  struct ComputePipelineStateKey
  {
    ComputePipelineStateDesc desc;
    auto operator==(const ComputePipelineStateKey&) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const ComputePipelineStateKey& key)
    {
      hasher.combine(key.desc);
    }
  };

  struct PipelineStateKey
  {

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

  struct PipelineState
  {
    VkPipeline vk_handle           = VK_NULL_HANDLE;
    VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    ProgramID program_id;
  };

  using PipelineStateCache =
    ConcurrentObjectCache<PipelineStateKey, PipelineState, PipelineStateID>;

  struct ProgramDescriptorBinding
  {
    u8 count                                  = 0;
    u8 attachment_index                       = 0;
    VkShaderStageFlags shader_stage_flags     = 0;
    VkPipelineStageFlags pipeline_stage_flags = 0;
  };

  struct RenderPassKey
  {
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

  struct QueueData
  {
    u32 count      = 0;
    u32 indices[3] = {};
  };

  struct Swapchain
  {
    WSI* wsi                  = nullptr;
    VkSwapchainKHR vk_handle  = VK_NULL_HANDLE;
    VkSurfaceFormatKHR format = {};
    VkExtent2D extent         = {};
    u32 image_count           = 0;
    SBOVector<TextureID> textures;
    SBOVector<VkImage> images;
    SBOVector<VkImageView> image_views;
  };

  struct DescriptorSetLayoutBinding
  {
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

  struct DescriptorSetLayoutKey
  {
    Array<DescriptorSetLayoutBinding, MAX_BINDING_PER_SET> bindings;

    auto operator==(const DescriptorSetLayoutKey& other) const -> bool = default;

    friend void soul_op_hash_combine(auto& hasher, const DescriptorSetLayoutKey& key) {}
  };

  static_assert(soul::impl_soul_op_hash_combine_v<gpu::impl::DescriptorSetLayoutKey>);

  struct ShaderDescriptorBinding
  {
    u8 count           = 0;
    u8 attachmentIndex = 0;
  };

  struct ShaderInput
  {
    VkFormat format = VK_FORMAT_UNDEFINED;
    u32 offset      = 0;
  };

  using VisibleAccessMatrix                 = FlagMap<PipelineStage, AccessFlags>;
  constexpr auto VISIBLE_ACCESS_MATRIX_ALL  = VisibleAccessMatrix::Fill(ACCESS_FLAGS_ALL);
  constexpr auto VISIBLE_ACCESS_MATRIX_NONE = VisibleAccessMatrix::Fill(AccessFlags());

  struct ResourceCacheState
  {
    QueueType queue_owner = QueueType::COUNT;
    PipelineStageFlags unavailable_pipeline_stages;
    AccessFlags unavailable_accesses;
    PipelineStageFlags sync_stages            = PIPELINE_STAGE_FLAGS_ALL;
    VisibleAccessMatrix visible_access_matrix = VISIBLE_ACCESS_MATRIX_ALL;

    auto commit_acquire_swapchain() -> void
    {
      queue_owner                 = QueueType::GRAPHIC;
      unavailable_pipeline_stages = {};
      unavailable_accesses        = {};
      sync_stages                 = {};
      visible_access_matrix       = VISIBLE_ACCESS_MATRIX_NONE;
    }

    auto commit_wait_semaphore(
      QueueType src_queue_type, QueueType dst_queue_type, PipelineStageFlags dst_stages) -> void
    {
      if (queue_owner != QueueType::COUNT && queue_owner != src_queue_type)
      {
        return;
      }
      queue_owner                 = dst_queue_type;
      sync_stages                 = dst_stages;
      unavailable_pipeline_stages = {};
      unavailable_accesses        = {};
      dst_stages.for_each(
        [this](const PipelineStage dst_stage)
        {
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
      if (queue_owner != QueueType::COUNT && queue_owner != queue_type)
      {
        SOUL_LOG_INFO("Queue owner mismatch");
        return;
      }
      if ((sync_stages & src_stages).none())
      {
        return;
      }
      if ((unavailable_pipeline_stages & src_stages) != unavailable_pipeline_stages)
      {
        return;
      }
      if ((unavailable_accesses & src_accesses) != unavailable_accesses)
      {
        return;
      }
      queue_owner = queue_type;
      sync_stages |= dst_stages;
      unavailable_pipeline_stages = {};
      unavailable_accesses        = {};
      if (layout_change)
      {
        visible_access_matrix = VISIBLE_ACCESS_MATRIX_NONE;
      }
      dst_stages.for_each(
        [this, dst_accesses](const PipelineStage dst_stage)
        {
          visible_access_matrix[dst_stage] |= dst_accesses;
        });
    }

    auto commit_access(QueueType queue, PipelineStageFlags stages, AccessFlags accesses) -> void
    {
      SOUL_ASSERT(0, (sync_stages & stages) == stages);
      SOUL_ASSERT(0, unavailable_accesses.none());
      queue_owner = queue;
      unavailable_pipeline_stages |= stages;
      const auto write_accesses = (accesses & ACCESS_FLAGS_WRITE);
      if (write_accesses.any())
      {
        unavailable_accesses |= write_accesses;
        visible_access_matrix = VISIBLE_ACCESS_MATRIX_NONE;
      }
    }

    [[nodiscard]]
    auto need_invalidate(PipelineStageFlags stages, AccessFlags accesses) -> b8
    {
      return stages
        .find_if(
          [this, accesses](const PipelineStage pipeline_stage)
          {
            return accesses.test_any(~visible_access_matrix[pipeline_stage]);
          })
        .is_some();
    }

    auto join(const ResourceCacheState& other) -> void
    {
      unavailable_pipeline_stages |= other.unavailable_pipeline_stages;
      unavailable_accesses |= other.unavailable_accesses;
      for (const auto stage_flag : FlagIter<PipelineStage>())
      {
        visible_access_matrix[stage_flag] &= other.visible_access_matrix[stage_flag];
      }
    }
  };

  struct Buffer
  {
    String name;
    BufferDesc desc;
    VkBuffer vk_handle = VK_NULL_HANDLE;
    VmaAllocation allocation{};
    ResourceCacheState cache_state;
    DescriptorID storage_buffer_gpu_handle      = DescriptorID::null();
    VkMemoryPropertyFlags memory_property_flags = 0;
  };

  using BufferPool = ChunkedSparsePool<Buffer, BufferID>;

  struct TextureView
  {
    VkImageView vk_handle                 = VK_NULL_HANDLE;
    DescriptorID storage_image_gpu_handle = DescriptorID::null();
    DescriptorID sampled_image_gpu_handle = DescriptorID::null();
  };

  struct Texture
  {
    String name;
    TextureDesc desc;
    VkImage vk_handle        = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    TextureView view;
    TextureView* views         = nullptr;
    VkImageLayout layout       = VK_IMAGE_LAYOUT_UNDEFINED;
    VkSharingMode sharing_mode = {};
    ResourceCacheState cache_state;
  };

  using TexturePool = ChunkedSparsePool<Texture, TextureID>;

  struct Blas
  {
    struct BlasGroupData
    {
      BlasGroupID group_id = BlasGroupID::Null();
      usize index          = 0;
    };

    String name;
    BlasDesc desc;
    VkAccelerationStructureKHR vk_handle = VK_NULL_HANDLE;
    BufferID buffer;
    ResourceCacheState cache_state;
    BlasGroupData group_data;
  };

  using BlasPool = ChunkedSparsePool<Blas, BlasID>;

  struct BlasGroup
  {
    String name;
    Vector<BlasID> blas_list       = Vector<BlasID>();
    ResourceCacheState cache_state = ResourceCacheState();
  };

  using BlasGroupPool = ChunkedSparsePool<BlasGroup, BlasGroupID>;

  struct Tlas
  {
    String name;
    TlasDesc desc;
    VkAccelerationStructureKHR vk_handle = VK_NULL_HANDLE;
    BufferID buffer;
    DescriptorID descriptor_id;
    ResourceCacheState cache_state;
  };

  using TlasPool = ChunkedSparsePool<Tlas, TlasID>;

  struct Shader
  {
    ShaderStage stage;
    VkShaderModule vk_handle = VK_NULL_HANDLE;
    String entry_point;
  };

  using ShaderPool = ChunkedSparsePool<Shader, ShaderID>;

  struct ShaderTable
  {
    using Buffers = FlagMap<ShaderGroupKind, BufferID>;
    using Regions = FlagMap<ShaderGroupKind, VkStridedDeviceAddressRegionKHR>;

    String name;
    VkPipeline pipeline = VK_NULL_HANDLE;
    Buffers buffers     = Buffers::Fill(BufferID::Null());
    Regions vk_regions  = Regions();
  };

  using ShaderTablePool = ChunkedSparsePool<ShaderTable, ShaderTableID>;

  struct Program
  {
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    SBOVector<Shader> shaders;
  };

  using ProgramPool = ChunkedSparsePool<Program, ProgramID>;

  struct BinarySemaphore
  {
    enum class State : u8
    {
      INIT,
      SIGNALLED,
      WAITED,
      COUNT
    };

    VkSemaphore vk_handle = VK_NULL_HANDLE;
    State state           = State::INIT;

    static auto null() -> BinarySemaphore
    {
      return {VK_NULL_HANDLE};
    }

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

  struct TimelineSemaphore
  {
    u32 queue_family_index;
    VkSemaphore vk_handle;
    u64 counter;

    static auto null() -> TimelineSemaphore
    {
      return {};
    }

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
  inline auto is_semaphore_valid(const Semaphore& semaphore) -> b8
  {
    return semaphore.visit(VisitorSet{
      [](BinarySemaphore* semaphore)
      {
        return semaphore->is_valid();
      },
      [](const TimelineSemaphore& semaphore)
      {
        return semaphore.is_valid();
      },
    });
  }

  [[nodiscard]]
  inline auto is_semaphore_null(const Semaphore& semaphore) -> b8
  {
    return semaphore.visit(VisitorSet{
      [](BinarySemaphore* semaphore)
      {
        return semaphore->is_null();
      },
      [](const TimelineSemaphore& semaphore)
      {
        return semaphore.is_null();
      },
    });
  }

  class CommandQueue
  {
  public:
    void init(VkDevice device, u32 family_index, u32 queue_index);

    void wait(Semaphore semaphore, VkPipelineStageFlags wait_stages);

    void wait(BinarySemaphore* semaphore, VkPipelineStageFlags wait_stages);

    void wait(TimelineSemaphore semaphore, VkPipelineStageFlags wait_stages);

    [[nodiscard]]
    auto get_vk_handle() const -> VkQueue
    {
      return vk_handle_;
    }

    auto get_timeline_semaphore() -> TimelineSemaphore;

    void submit(PrimaryCommandBuffer command_buffer, BinarySemaphore* = nullptr);

    void flush(BinarySemaphore* binary_semaphore = nullptr);

    void present(VkSwapchainKHR swapchain, u32 swapchain_index, BinarySemaphore* semaphore);

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
    void init_timeline_semaphore();

    VkDevice device_   = VK_NULL_HANDLE;
    VkQueue vk_handle_ = VK_NULL_HANDLE;
    u32 family_index_  = 0;
    SBOVector<VkSemaphore> wait_semaphores_;
    SBOVector<VkPipelineStageFlags> wait_stages_;
    SBOVector<u64> wait_timeline_values_;
    SBOVector<VkCommandBuffer> commands_;

    VkSemaphore timeline_semaphore_ = VK_NULL_HANDLE;
    u64 current_timeline_values_    = 0;
  };

  class SecondaryCommandBuffer
  {
  private:
    VkCommandBuffer vk_handle_ = VK_NULL_HANDLE;

  public:
    constexpr SecondaryCommandBuffer() noexcept = default;

    explicit constexpr SecondaryCommandBuffer(VkCommandBuffer vk_handle) : vk_handle_(vk_handle) {}

    [[nodiscard]]
    constexpr auto get_vk_handle() const noexcept -> VkCommandBuffer
    {
      return vk_handle_;
    }

    void end();
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

    void init(VkDevice device, VkCommandBufferLevel level, u32 queue_family_index);
    void reset();
    auto request() -> VkCommandBuffer;
    void shutdown();

  private:
    runtime::AllocatorInitializer allocator_initializer_;
    VkDevice device_         = VK_NULL_HANDLE;
    VkCommandPool vk_handle_ = VK_NULL_HANDLE;
    Vector<VkCommandBuffer> allocated_buffers_;
    VkCommandBufferLevel level_ = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
    u16 count_                  = 0;
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
    void shutdown();
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
    struct StagingBuffer
    {
      VkBuffer vk_handle;
      VmaAllocation allocation;
    };

    // TODO(kevinyu): This should be alignas(SOUL_CACHELINE_SIZE), but malloc does not
    // allocate memory with alignment more than sizeof(void*) causing problem, should
    // add alignas(SOUL_CACHELINE_SIZE) back, once we have an alternative memory allocator_
    // that could allocate memory with better alignment support
    struct ThreadContext
    {
      PrimaryCommandBuffer transfer_command_buffer;
      PrimaryCommandBuffer clear_command_buffer;
      PrimaryCommandBuffer mipmap_gen_command_buffer;
      PrimaryCommandBuffer as_command_buffer;
      Vector<StagingBuffer> staging_buffers;
    };

    VmaAllocator gpu_allocator_  = nullptr;
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
    // TODO(kevinyu): This should be alignas(SOUL_CACHELINE_SIZE), but malloc does not
    // allocate memory with alignment more than sizeof(void*) causing problem, should
    // add alignas(SOUL_CACHELINE_SIZE) back, once we have an alternative memory allocator_
    // that could allocate memory with better alignment support
    struct ThreadContext
    {
      FlagMap<QueueType, Vector<VkImageMemoryBarrier>> image_barriers_;
      FlagMap<QueueType, QueueFlags> sync_dst_queues_;

      // TODO(kevinyu): Investigate why we cannot use explicitly or implicitly default constructor
      // here.
      // - alignas seems to be relevant
      // - only happen on release mode
      ThreadContext() {} // NOLINT

      ThreadContext(const ThreadContext&)                    = delete;
      auto operator=(const ThreadContext&) -> ThreadContext& = delete;
      ThreadContext(ThreadContext&&)                         = default;
      auto operator=(ThreadContext&&) -> ThreadContext&      = default;

      ~ThreadContext() {} // NOLINT
    };

    // static_assert(alignof(ThreadContext) == SOUL_CACHELINE_SIZE);

    Vector<ThreadContext> thread_contexts_;
  };

  struct FrameContext
  {
    runtime::AllocatorInitializer allocator_initializer;
    CommandPools command_pools;

    TimelineSemaphore frame_end_semaphore = TimelineSemaphore::null();
    BinarySemaphore image_available_semaphore;
    BinarySemaphore render_finished_semaphore;

    u32 swapchain_index = 0;

    struct Garbages
    {
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

      struct SwapchainGarbage
      {
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

  struct Database
  {
    using CPUAllocatorProxy = memory::MultiProxy<memory::ProfileProxy, memory::CounterProxy>;
    using CPUAllocator      = memory::ProxyAllocator<memory::Allocator, CPUAllocatorProxy>;
    CPUAllocator cpu_allocator;

    memory::MallocAllocator vulkan_cpu_backing_allocator{"Vulkan CPU Backing Allocator"_str};
    using VulkanCPUAllocatorProxy = memory::MultiProxy<memory::MutexProxy, memory::ProfileProxy>;
    using VulkanCPUAllocator =
      memory::ProxyAllocator<memory::MallocAllocator, VulkanCPUAllocatorProxy>;
    VulkanCPUAllocator vulkan_cpu_allocator;

    runtime::AllocatorInitializer allocator_initializer;

    VkInstance instance                      = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    VkDevice device                                                        = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device                                       = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physical_device_properties                  = {};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
      .pNext = nullptr,
    };
    VkPhysicalDeviceAccelerationStructurePropertiesKHR as_properties = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
      .pNext = &ray_tracing_properties,
    };
    GPUProperties gpu_properties                                       = {};
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};
    VkPhysicalDeviceFeatures physical_device_features                  = {};
    FlagMap<QueueType, u32> queue_family_indices                       = {};

    CommandQueues queues;

    VkSurfaceKHR surface                  = VK_NULL_HANDLE;
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

    PipelineStateCache pipeline_state_cache;

    ProgramPool program_pool;
    ShaderTablePool shader_table_pool;

    HashMap<RenderPassKey, VkRenderPass, HashOp<RenderPassKey>> render_pass_maps;

    UInt64HashMap<SamplerID> sampler_map;
    BindlessDescriptorAllocator descriptor_allocator;

    explicit Database(memory::Allocator* backing_allocator)
        : cpu_allocator(
            "GPU System allocator"_str,
            backing_allocator,
            CPUAllocatorProxy::Config{
              memory::ProfileProxy::Config(), memory::CounterProxy::Config()}),
          vulkan_cpu_allocator(
            "Vulkan allocator"_str,
            &vulkan_cpu_backing_allocator,
            VulkanCPUAllocatorProxy::Config{
              memory::MutexProxy::Config(), memory::ProfileProxy::Config()}),
          allocator_initializer(&cpu_allocator)
    {
      allocator_initializer.end();
    }
  };

} // namespace soul::gpu::impl
