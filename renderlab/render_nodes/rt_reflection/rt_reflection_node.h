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
  class RtReflectionNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;

    gpu::ProgramID ray_trace_program_id_;
    gpu::ProgramID init_dispatch_args_program_id_;
    gpu::ProgramID temporal_accumulation_program_id_;
    gpu::ProgramID filter_tile_program_id_;
    gpu::ProgramID copy_tile_program_id_;
    gpu::ShaderTableID shader_table_id_;

    Array<gpu::TextureID, 2> moment_textures_;
    gpu::TextureID temporal_accumulation_output_texture_;
    gpu::TextureID atrous_feedback_texture_;
    vec2u32 viewport_ = {0, 0};

    f32 alpha              = 0.1f;
    f32 moments_alpha      = 0.2f;
    f32 phi_color          = 2.0f;
    f32 phi_normal         = 4.0f;
    f32 sigma_depth        = 1.0f;
    i32 radius             = 2;
    i32 filter_iterations  = 4;
    i32 feedback_iteration = 1;
    f32 trace_normal_bias  = 0.1f;
    f32 lobe_trim          = 0.8f;

  public:
    static constexpr auto PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT = "prev_normal_roughness"_str;
    static constexpr auto PREV_GBUFFER_MOTION_CURVE_INPUT     = "prev_motion_curve"_str;
    static constexpr auto PREV_GBUFFER_MESHID_INPUT           = "prev_meshid"_str;
    static constexpr auto PREV_GBUFFER_DEPTH_INPUT            = "prev_depth"_str;
    static constexpr auto GBUFFER_NORMAL_ROUGHNESS_INPUT      = "normal_roughness"_str;
    static constexpr auto GBUFFER_MOTION_CURVE_INPUT          = "motion_curve"_str;
    static constexpr auto GBUFFER_MESHID_INPUT                = "meshid"_str;
    static constexpr auto GBUFFER_DEPTH_INPUT                 = "depth"_str;

    static constexpr auto OUTPUT                       = "output"_str;
    static constexpr auto RAY_TRACE_OUTPUT             = "ray_trace_output"_str;
    static constexpr auto TEMPORAL_ACCUMULATION_OUTPUT = "temporal_accumulation_output"_str;
    static constexpr auto TEMPORAL_ACCUMULATION_MOMENT_OUTPUT =
      "temporal_accumulation_moment_output"_str;

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
      RenderNodeField::Texture2D(RAY_TRACE_OUTPUT),
      RenderNodeField::Texture2D(TEMPORAL_ACCUMULATION_OUTPUT),
      RenderNodeField::Texture2D(TEMPORAL_ACCUMULATION_MOMENT_OUTPUT),
    };

    explicit RtReflectionNode(NotNull<gpu::System*> gpu_system);

    RtReflectionNode(const RtReflectionNode&) = delete;

    RtReflectionNode(RtReflectionNode&&) = delete;

    auto operator=(const RtReflectionNode&) -> RtReflectionNode& = delete;

    auto operator=(RtReflectionNode&&) -> RtReflectionNode& = delete;

    ~RtReflectionNode() override;

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
