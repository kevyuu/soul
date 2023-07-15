#pragma once

#include "core/cstring.h"
#include "core/panic.h"
#include "gpu/intern/bindless_descriptor_allocator.h"
#include "gpu/type.h"

#if defined(SOUL_ASSERT_ENABLE)
#  define SOUL_VK_CHECK(result, ...) SOUL_ASSERT(0, result == VK_SUCCESS, ##__VA_ARGS__)
#else
#  include "core/log.h"
#  define SOUL_VK_CHECK(expr, message)                                                             \
    do {                                                                                           \
      VkResult _result = expr;                                                                     \
      if (_result != VK_SUCCESS) {                                                                 \
        SOUL_LOG_ERROR("Vulkan error| expr = {}, result = {} ", #expr, usize(_result));        \
        SOUL_LOG_ERROR("Message = {}", message);                                                   \
      }                                                                                            \
    } while (0)
#endif

#include <span>

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

    struct Config {
      WSI* wsi = nullptr;
      ui16 max_frame_in_flight = 0;
      ui16 thread_count = 0;
      usize transient_pool_size = 16 * ONE_MEGABYTE;
    };

    auto init(const Config& config) -> void;
    auto init_frame_context(const Config& config) -> void;

    auto get_gpu_properties() const -> const GPUProperties&;

    auto shutdown() -> void;

    auto create_buffer(const BufferDesc& desc) -> BufferID;
    auto create_buffer(const BufferDesc& desc, const void* data) -> BufferID;
    auto create_transient_buffer(const BufferDesc& desc) -> BufferID;
    auto flush_buffer(BufferID buffer_id) -> void;
    auto destroy_buffer_descriptor(BufferID buffer_id) -> void;
    auto destroy_buffer(BufferID buffer_id) -> void;
    auto get_buffer(BufferID buffer_id) -> impl::Buffer&;
    auto get_buffer(BufferID buffer_id) const -> const impl::Buffer&;
    auto get_gpu_address(BufferID buffer_id) const -> GPUAddress;

    auto create_texture(const TextureDesc& desc) -> TextureID;
    auto create_texture(const TextureDesc& desc, const TextureLoadDesc& load_desc) -> TextureID;
    auto create_texture(const TextureDesc& desc, ClearValue clear_value) -> TextureID;
    auto flush_texture(TextureID texture_id, TextureUsageFlags usage_flags) -> void;
    auto get_texture_mip_levels(TextureID texture_id) const -> ui32;
    auto get_texture_desc(TextureID texture_id) const -> const TextureDesc&;
    auto destroy_texture_descriptor(TextureID texture_id) -> void;
    auto destroy_texture(TextureID texture_id) -> void;
    auto get_texture(TextureID texture_id) -> impl::Texture&;
    auto get_texture(TextureID texture_id) const -> const impl::Texture&;
    auto get_texture_view(TextureID texture_id, ui32 level, ui32 layer = 0)
      -> impl::TextureView;
    auto get_texture_view(TextureID texture_id, SubresourceIndex subresource_index)
      -> impl::TextureView;
    auto get_texture_view(TextureID texture_id, std::optional<SubresourceIndex> subresource)
      -> impl::TextureView;

    auto get_blas_size_requirement(const BlasBuildDesc& build_desc) -> usize;
    auto create_blas(const BlasDesc& desc, BlasGroupID blas_group_id = BlasGroupID::null())
      -> BlasID;
    auto destroy_blas(BlasID blas_id) -> void;
    auto get_blas(BlasID blas_id) const -> const impl::Blas&;
    auto get_blas(BlasID blas_id) -> impl::Blas&;
    auto get_gpu_address(BlasID blas_id) const -> GPUAddress;

    auto create_blas_group(const char* name) -> BlasGroupID;
    auto destroy_blas_group(BlasGroupID blas_group_id) -> void;
    auto get_blas_group(BlasGroupID blas_group_id) const -> const impl::BlasGroup&;
    auto get_blas_group(BlasGroupID blas_group_id) -> impl::BlasGroup&;

    auto get_tlas_size_requirement(const TlasBuildDesc& build_desc) -> usize;
    auto create_tlas(const TlasDesc& desc) -> TlasID;
    auto destroy_tlas(TlasID tlas_id) -> void;
    auto get_tlas(TlasID tlas_id) const -> const impl::Tlas&;
    auto get_tlas(TlasID tlas_id) -> impl::Tlas&;

    auto create_program(const ProgramDesc& program_desc) -> expected<ProgramID, Error>;
    auto get_program_ptr(ProgramID program_id) -> impl::Program*;
    auto get_program(ProgramID program_id) -> const impl::Program&;

    auto create_shader_table(const ShaderTableDesc& shader_table_desc) -> ShaderTableID;
    auto destroy_shader_table(ShaderTableID shader_table_id) -> void;
    auto get_shader_table(ShaderTableID shader_table_id) const -> const impl::ShaderTable&;
    auto get_shader_table(ShaderTableID shader_table_id) -> impl::ShaderTable&;

    auto request_pipeline_state(
      const GraphicPipelineStateDesc& key,
      VkRenderPass render_pass,
      TextureSampleCount sample_count) -> PipelineStateID;
    auto request_pipeline_state(const ComputePipelineStateDesc& key) -> PipelineStateID;
    auto get_pipeline_state(PipelineStateID pipeline_state_id) -> const impl::PipelineState&;
    auto get_bindless_pipeline_layout() const -> VkPipelineLayout;
    auto get_bindless_descriptor_sets() const -> impl::BindlessDescriptorSets;

    auto request_sampler(const SamplerDesc& desc) -> SamplerID;

    auto get_ssbo_descriptor_id(BufferID buffer_id) const -> DescriptorID;
    auto get_srv_descriptor_id(
      TextureID texture_id, std::optional<SubresourceIndex> subresource_index = std::nullopt)
      -> DescriptorID;
    auto get_uav_descriptor_id(
      TextureID texture_id, std::optional<SubresourceIndex> subresource_index = std::nullopt)
      -> DescriptorID;
    auto get_sampler_descriptor_id(SamplerID sampler_id) const -> DescriptorID;
    auto get_as_descriptor_id(TlasID tlas_id) const -> DescriptorID;

    auto create_binary_semaphore() -> impl::BinarySemaphore;
    auto destroy_binary_semaphore(impl::BinarySemaphore id) -> void;

    auto create_event() -> VkEvent;
    auto destroy_event(VkEvent event) -> void;

    auto execute(const RenderGraph& render_graph) -> void;

    auto flush() -> void;
    auto flush_frame() -> void;
    auto begin_frame() -> void;
    auto end_frame() -> void;

    auto recreate_swapchain() -> void;
    auto get_swapchain_extent() -> vec2ui32;
    auto get_swapchain_texture() -> TextureID;

    auto get_frame_context() -> impl::FrameContext&;

    auto request_render_pass(const impl::RenderPassKey& key) -> VkRenderPass;

    auto create_framebuffer(const VkFramebufferCreateInfo& info) -> VkFramebuffer;
    auto destroy_framebuffer(VkFramebuffer framebuffer) -> void;

    auto get_queue_data_from_queue_flags(QueueFlags flags) const -> impl::QueueData;

    impl::Database _db;

    friend class impl::RenderCompiler;
    friend class impl::RenderGraphExecution;

  private:
    auto is_owned_by_presentation_engine(TextureID texture_id) -> bool;
    auto create_buffer(const BufferDesc& desc, bool use_linear_pool) -> BufferID;
    auto create_staging_buffer(usize size) -> BufferID;
    auto get_gpu_allocator() -> VmaAllocator;

    auto acquire_swapchain() -> VkResult;

    auto wait_sync_counter(impl::TimelineSemaphore sync_counter) -> void;

    auto calculate_gpu_properties() -> void;

    auto get_as_build_size_info(const TlasBuildDesc& build_desc)
      -> VkAccelerationStructureBuildSizesInfoKHR;
    auto get_as_build_size_info(const BlasBuildDesc& build_desc)
      -> VkAccelerationStructureBuildSizesInfoKHR;
    auto get_as_build_size_info(
      const VkAccelerationStructureBuildGeometryInfoKHR& build_info,
      const ui32* max_primitives_counts) -> VkAccelerationStructureBuildSizesInfoKHR;

    auto add_to_blas_group(BlasID blas_id, BlasGroupID blas_group_id) -> void;
    auto remove_from_blas_group(BlasID blas_id) -> void;

    Config config_;
  };

} // namespace soul::gpu
