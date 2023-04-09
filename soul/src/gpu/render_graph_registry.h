#pragma once

#include "gpu/render_graph.h"
#include "gpu/type.h"

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

    RenderGraphRegistry(
      System& system,
      const impl::RenderGraphExecution& execution,
      const VkRenderPass render_pass = VK_NULL_HANDLE,
      const TextureSampleCount sample_count = TextureSampleCount::COUNT_1)
        : system_(system),
          execution_(execution),
          render_pass_(render_pass),
          sample_count_(sample_count)
    {
    }

    RenderGraphRegistry(const RenderGraphRegistry& other) = delete;
    auto operator=(const RenderGraphRegistry& other) -> RenderGraphRegistry& = delete;

    RenderGraphRegistry(RenderGraphRegistry&& other) = delete;
    auto operator=(RenderGraphRegistry&& other) -> RenderGraphRegistry = delete;

    ~RenderGraphRegistry() = default;

    [[nodiscard]] auto get_buffer(BufferNodeID buffer_node_id) const -> BufferID;
    [[nodiscard]] auto get_texture(TextureNodeID texture_node_id) const -> TextureID;
    [[nodiscard]] auto get_tlas(TlasNodeID tlas_node_id) const -> TlasID;
    auto get_pipeline_state(const GraphicPipelineStateDesc& desc) -> PipelineStateID;
    auto get_pipeline_state(const ComputePipelineStateDesc& desc) -> PipelineStateID;

  private:
    System& system_;
    const impl::RenderGraphExecution& execution_;
    VkRenderPass render_pass_;
    const TextureSampleCount sample_count_;
  };

} // namespace soul::gpu
