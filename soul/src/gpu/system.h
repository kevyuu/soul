#pragma once

#include "core/result.h"
#include "core/string.h"

#include "gpu/impl/vulkan/bindless_descriptor_allocator.h"
#include "gpu/impl/vulkan/type.h"

namespace soul::gpu::impl
{
  class RenderCompiler;
  class RenderGraphExecution;
} // namespace soul::gpu::impl

namespace soul::gpu
{

  class RenderGraph;

  class System
  {
  public:
    explicit System(memory::Allocator* allocator) : _db(allocator) {}

    struct Config
    {
      WSI* wsi                  = nullptr;
      b8 use_srgb_swapchain     = false;
      u16 max_frame_in_flight   = 0;
      u16 thread_count          = 0;
      usize transient_pool_size = 50 * ONE_MEGABYTE;
    };

    void init(const Config& config);
    void init_frame_context(const Config& config);

    auto get_gpu_properties() const -> const GPUProperties&;

    void shutdown();

    auto create_buffer(String&& name, const BufferDesc& desc) -> BufferID;
    auto create_buffer(String&& name, const BufferDesc& desc, const void* data) -> BufferID;
    auto create_transient_buffer(String&& name, const BufferDesc& desc) -> BufferID;
    void flush_buffer(BufferID buffer_id);
    void destroy_buffer_descriptor(BufferID buffer_id);
    void destroy_buffer(BufferID buffer_id);
    auto buffer_ref(BufferID buffer_id) -> impl::Buffer&;
    auto buffer_cref(BufferID buffer_id) const -> const impl::Buffer&;
    auto get_gpu_address(BufferID buffer_id, usize offset = 0) const -> GPUAddress;
    auto buffer_desc_cref(BufferID buffer_id) const -> const BufferDesc&;

    auto create_texture(String&& name, const TextureDesc& desc) -> TextureID;
    auto create_texture(String&& name, const TextureDesc& desc, const TextureLoadDesc& load_desc)
      -> TextureID;
    auto create_texture(String&& name, const TextureDesc& desc, ClearValue clear_value)
      -> TextureID;
    void flush_texture(TextureID texture_id, TextureUsageFlags usage_flags);
    auto get_texture_mip_levels(TextureID texture_id) const -> u32;
    void destroy_texture_descriptor(TextureID texture_id);
    void destroy_texture(TextureID texture_id);
    auto texture_ref(TextureID texture_id) -> impl::Texture&;
    auto texture_cref(TextureID texture_id) const -> const impl::Texture&;
    auto get_texture_view(TextureID texture_id, u32 level, u32 layer = 0) -> impl::TextureView;
    auto get_texture_view(TextureID texture_id, SubresourceIndex subresource_index)
      -> impl::TextureView;
    auto get_texture_view(TextureID texture_id, Option<SubresourceIndex> subresource)
      -> impl::TextureView;
    auto texture_desc_cref(TextureID texture_id) const -> const TextureDesc&;
    auto texture_name_view(TextureID texture_id) const -> StringView;

    auto get_blas_size_requirement(const BlasBuildDesc& build_desc) -> usize;
    auto create_blas(
      String&& name, const BlasDesc& desc, BlasGroupID blas_group_id = BlasGroupID::Null())
      -> BlasID;
    void destroy_blas(BlasID blas_id);
    auto blas_cref(BlasID blas_id) const -> const impl::Blas&;
    auto blas_ref(BlasID blas_id) -> impl::Blas&;
    auto get_gpu_address(BlasID blas_id) const -> GPUAddress;

    auto create_blas_group(String&& name) -> BlasGroupID;
    void destroy_blas_group(BlasGroupID blas_group_id);
    auto blas_group_cref(BlasGroupID blas_group_id) const -> const impl::BlasGroup&;
    auto blas_group_ref(BlasGroupID blas_group_id) -> impl::BlasGroup&;

    auto get_tlas_size_requirement(const TlasBuildDesc& build_desc) -> usize;
    auto create_tlas(String&& name, const TlasDesc& desc) -> TlasID;
    void destroy_tlas(TlasID tlas_id);
    auto tlas_cref(TlasID tlas_id) const -> const impl::Tlas&;
    auto tlas_ref(TlasID tlas_id) -> impl::Tlas&;

    auto create_program(const ProgramDesc& program_desc) -> Result<ProgramID, Error>;
    void destroy_program(ProgramID program_id);

    auto create_shader_table(String&& name, const ShaderTableDesc& shader_table_desc)
      -> ShaderTableID;
    void destroy_shader_table(ShaderTableID shader_table_id);
    auto shader_table_cref(ShaderTableID shader_table_id) const -> const impl::ShaderTable&;
    auto shader_table_ref(ShaderTableID shader_table_id) -> impl::ShaderTable&;

    auto request_pipeline_state(
      const GraphicPipelineStateDesc& desc,
      VkRenderPass render_pass,
      TextureSampleCount sample_count) -> PipelineStateID;
    auto request_pipeline_state(const ComputePipelineStateDesc& desc) -> PipelineStateID;
    auto get_pipeline_state(PipelineStateID pipeline_state_id) -> const impl::PipelineState&;
    auto get_bindless_pipeline_layout() const -> VkPipelineLayout;
    auto get_bindless_descriptor_sets() const -> impl::BindlessDescriptorSets;

    auto request_sampler(const SamplerDesc& desc) -> SamplerID;

    auto get_ssbo_descriptor_id(BufferID buffer_id) const -> DescriptorID;
    auto get_srv_descriptor_id(
      TextureID texture_id, Option<SubresourceIndex> subresource_index = nilopt) -> DescriptorID;
    auto get_uav_descriptor_id(
      TextureID texture_id, Option<SubresourceIndex> subresource_index = nilopt) -> DescriptorID;
    auto get_sampler_descriptor_id(SamplerID sampler_id) const -> DescriptorID;
    auto get_as_descriptor_id(TlasID tlas_id) const -> DescriptorID;

    auto create_binary_semaphore() -> impl::BinarySemaphore;
    void destroy_binary_semaphore(impl::BinarySemaphore semaphore);

    auto create_event() -> VkEvent;
    void destroy_event(VkEvent event);

    void execute(const RenderGraph& render_graph);

    void flush();
    void flush_frame();
    void begin_frame();
    void end_frame();

    void recreate_swapchain();
    auto get_swapchain_extent() -> vec2u32;
    auto get_swapchain_texture() -> TextureID;

    auto get_frame_context() -> impl::FrameContext&;

    auto request_render_pass(const impl::RenderPassKey& key) -> VkRenderPass;

    auto create_framebuffer(const VkFramebufferCreateInfo& info) -> VkFramebuffer;
    void destroy_framebuffer(VkFramebuffer framebuffer);

    auto get_queue_data_from_queue_flags(QueueFlags flags) const -> impl::QueueData;

    friend class impl::RenderCompiler;
    friend class impl::RenderGraphExecution;

  private:
    impl::Database _db;

    auto is_owned_by_presentation_engine(TextureID texture_id) -> b8;
    auto create_buffer(String&& name, const BufferDesc& desc, b8 use_linear_pool) -> BufferID;
    auto create_staging_buffer(usize size) -> BufferID;
    auto get_gpu_allocator() -> VmaAllocator;

    auto acquire_swapchain() -> VkResult;

    void wait_sync_counter(impl::TimelineSemaphore sync_counter);

    void calculate_gpu_properties();

    auto get_as_build_size_info(const TlasBuildDesc& build_desc)
      -> VkAccelerationStructureBuildSizesInfoKHR;
    auto get_as_build_size_info(const BlasBuildDesc& build_desc)
      -> VkAccelerationStructureBuildSizesInfoKHR;
    auto get_as_build_size_info(
      const VkAccelerationStructureBuildGeometryInfoKHR& build_info,
      const u32* max_primitives_counts) -> VkAccelerationStructureBuildSizesInfoKHR;

    void add_to_blas_group(BlasID blas_id, BlasGroupID blas_group_id);
    void remove_from_blas_group(BlasID blas_id);

    Config config_;
  };

} // namespace soul::gpu
