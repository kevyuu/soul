#pragma once

#include "gpu/sl_type.h"

#include "math/aabb.h"
#include "render_node.h"

using namespace soul;

namespace soul::gpu
{
  class RenderGraph;
}

namespace soul::app
{
  class Gui;
}

namespace renderlab
{

  class ToneMapNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;
    gpu::ProgramID program_id_;

  public:
    static constexpr auto INPUT  = "input"_str;
    static constexpr auto OUTPUT = "output"_str;

    static constexpr auto INPUT_FIELDS = Array{
      RenderNodeField::Texture2D(INPUT),
    };

    static constexpr auto OUTPUT_FIELDS = Array{
      RenderNodeField::Texture2D(OUTPUT),
    };

    explicit ToneMapNode(NotNull<gpu::System*> gpu_system);

    ToneMapNode(const ToneMapNode&) = delete;

    ToneMapNode(ToneMapNode&&) = delete;

    auto operator=(const ToneMapNode&) -> ToneMapNode& = delete;

    auto operator=(ToneMapNode&&) -> ToneMapNode& = delete;

    ~ToneMapNode() override;

    [[nodiscard]]
    inline auto get_input_fields() const -> Span<const RenderNodeField*> override
    {
      return INPUT_FIELDS.cspan();
    }

    [[nodiscard]]
    inline auto get_output_fields() const -> Span<const RenderNodeField*> override
    {
      return OUTPUT_FIELDS.cspan();
    }

    auto submit_pass(
      const Scene& scene,
      const RenderConstant& constant,
      const RenderData& inputs,
      NotNull<gpu::RenderGraph*> render_graph) -> RenderData override;

    void on_gui_render(NotNull<app::Gui*> gui) override;

    [[nodiscard]]
    auto get_gui_label() const -> CompStr override;
  };
} // namespace renderlab
