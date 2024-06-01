#pragma once

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
  class RtaoNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;
    gpu::ProgramID ray_query_program_id_;
    gpu::ProgramID init_dispatch_arg_program_id_;
    gpu::ProgramID temporal_accumulation_program_id_;
    gpu::ProgramID bilateral_blur_program_id_;

    Array<gpu::TextureID, 2> history_length_texture_ids_;
    gpu::TextureID feedback_ao_texture_id_;

    vec2u32 viewport_;

    f32 bias_    = 0.1f;
    f32 ray_min_ = 0.001f;
    f32 ray_max_ = 0.7f;
    i32 radius_  = 4;
    f32 alpha_   = 0.01f;

  public:
    static constexpr auto PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT = "prev_normal_roughness"_str;
    static constexpr auto PREV_GBUFFER_MOTION_CURVE_INPUT     = "prev_motion_curve"_str;
    static constexpr auto PREV_GBUFFER_MESHID_INPUT           = "prev_meshid"_str;
    static constexpr auto PREV_GBUFFER_DEPTH_INPUT            = "prev_depth"_str;
    static constexpr auto GBUFFER_NORMAL_ROUGHNESS_INPUT      = "normal_roughness"_str;
    static constexpr auto GBUFFER_MOTION_CURVE_INPUT          = "motion_curve"_str;
    static constexpr auto GBUFFER_MESHID_INPUT                = "meshid"_str;
    static constexpr auto GBUFFER_DEPTH_INPUT                 = "depth"_str;

    static constexpr auto OUTPUT                = "output"_str;
    static constexpr auto HISTORY_LENGTH_OUTPUT = "history_length_output"_str;

    static constexpr auto INPUT_FIELDS = Array{
      RenderNodeField::Texture2D(PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT),
      RenderNodeField::Texture2D(PREV_GBUFFER_MOTION_CURVE_INPUT),
      RenderNodeField::Texture2D(PREV_GBUFFER_MESHID_INPUT),
      RenderNodeField::Texture2D(PREV_GBUFFER_DEPTH_INPUT),
      RenderNodeField::Texture2D(GBUFFER_NORMAL_ROUGHNESS_INPUT),
      RenderNodeField::Texture2D(GBUFFER_MOTION_CURVE_INPUT),
      RenderNodeField::Texture2D(GBUFFER_MESHID_INPUT),
      RenderNodeField::Texture2D(GBUFFER_DEPTH_INPUT),
    };

    static constexpr auto OUTPUT_FIELDS = Array{
      RenderNodeField::Texture2D(OUTPUT),
      RenderNodeField::Texture2D(HISTORY_LENGTH_OUTPUT),
    };

    explicit RtaoNode(NotNull<gpu::System*> gpu_system);

    RtaoNode(const RtaoNode&) = delete;

    RtaoNode(RtaoNode&&) = delete;

    auto operator=(const RtaoNode&) -> RtaoNode& = delete;

    auto operator=(RtaoNode&&) -> RtaoNode& = delete;

    ~RtaoNode() override;

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
