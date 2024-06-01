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

  class TaaNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;
    gpu::ProgramID program_id_;

    Array<gpu::TextureID, 2> color_textures_;
    vec2u32 viewport_ = {0, 0};

    b8 enable_pass_       = true;
    f32 feedback_min_     = 0.88f;
    f32 feedback_max_     = 0.97f;
    bool sharpen_enable_  = false;
    bool dilation_enable_ = true;

  public:
    static constexpr auto COLOR_INPUT                = "color"_str;
    static constexpr auto DEPTH_INPUT                = "depth"_str;
    static constexpr auto GBUFFER_MOTION_CURVE_INPUT = "motion_curve"_str;
    static constexpr auto GBUFFER_DEPTH_INPUT        = "depth"_str;

    static constexpr auto OUTPUT         = "output"_str;
    static constexpr auto HISTORY_OUTPUT = "history_output"_str;

    static constexpr auto INPUT_FIELDS = Array{
      RenderNodeField::Texture2D(COLOR_INPUT),
      RenderNodeField::Texture2D(DEPTH_INPUT),
      RenderNodeField::Texture2D(GBUFFER_MOTION_CURVE_INPUT),
      RenderNodeField::Texture2D(GBUFFER_DEPTH_INPUT),
    };

    static constexpr auto OUTPUT_FIELDS = Array{
      RenderNodeField::Texture2D(OUTPUT),
      RenderNodeField::Texture2D(HISTORY_OUTPUT),
    };

    explicit TaaNode(NotNull<gpu::System*> gpu_system);

    TaaNode(const TaaNode&) = delete;

    TaaNode(TaaNode&&) = delete;

    auto operator=(const TaaNode&) -> TaaNode& = delete;

    auto operator=(TaaNode&&) -> TaaNode& = delete;

    ~TaaNode() override;

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

    void setup_images(vec2u32 viewport);
  };
} // namespace renderlab
