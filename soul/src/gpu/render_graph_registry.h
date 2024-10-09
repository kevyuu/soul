#pragma once

#include "gpu/render_graph.h"
#include "gpu/sl_type.h"

#include "gpu/impl/vulkan/type.h"

namespace soul::gpu
{

  namespace impl
  {
    class RenderGraphExecution;
  }

  class RenderGraphRegistry
  {
  public:
    RenderGraphRegistry() = delete;

    [[nodiscard]]
    static auto New(
      NotNull<System*> system,
      NotNull<const impl::RenderGraphExecution*> execution,
      VkRenderPass render_pass              = VK_NULL_HANDLE,
      const TextureSampleCount sample_count = TextureSampleCount::COUNT_1)
    {
      return RenderGraphRegistry(system, execution, render_pass, sample_count);
    }

    RenderGraphRegistry(const RenderGraphRegistry& other)                    = delete;
    auto operator=(const RenderGraphRegistry& other) -> RenderGraphRegistry& = delete;

    RenderGraphRegistry(RenderGraphRegistry&& other)                   = delete;
    auto operator=(RenderGraphRegistry&& other) -> RenderGraphRegistry = delete;

    ~RenderGraphRegistry() = default;

    [[nodiscard]]
    auto get_buffer(BufferNodeID buffer_node_id) const -> BufferID;
    [[nodiscard]]
    auto get_texture(TextureNodeID texture_node_id) const -> TextureID;
    [[nodiscard]]
    auto get_tlas(TlasNodeID tlas_node_id) const -> TlasID;
    auto get_pipeline_state(const GraphicPipelineStateDesc& desc) -> PipelineStateID;
    auto get_pipeline_state(const ComputePipelineStateDesc& desc) -> PipelineStateID;

    auto get_srv_descriptor_id(gpu::TextureNodeID node_id) -> soulsl::DescriptorID;
    auto get_uav_descriptor_id(gpu::TextureNodeID node_id) -> soulsl::DescriptorID;
    auto get_ssbo_descriptor_id(gpu::BufferNodeID node_id) -> soulsl::DescriptorID;
    auto get_tlas_descriptor_id(gpu::TlasNodeID node_id) -> soulsl::DescriptorID;

  private:
    NotNull<System*> system_;
    NotNull<const impl::RenderGraphExecution*> execution_;
    VkRenderPass render_pass_;
    TextureSampleCount sample_count_;

    RenderGraphRegistry(
      NotNull<System*> system,
      NotNull<const impl::RenderGraphExecution*> execution,
      VkRenderPass render_pass,
      const TextureSampleCount sample_count)
        : system_(system),
          execution_(execution),
          render_pass_(render_pass),
          sample_count_(sample_count)
    {
    }
  };

} // namespace soul::gpu
