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
  class GBufferGenerateNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;
    gpu::ProgramID program_id_;
    gpu::TextureID albedo_metal_gbuffer_;
    Array<gpu::TextureID, 2> normal_roughness_gbuffers_;
    Array<gpu::TextureID, 2> motion_curve_gbuffers_;
    Array<gpu::TextureID, 2> meshid_gbuffers_;

    gpu::TextureID prev_depth_texture_;

    vec2u32 viewport_ = {0, 0};

  public:
    static constexpr auto PREV_GBUFFER_NORMAL_ROUGHNESS = "prev_gbuffer_normal_roughness"_str;
    static constexpr auto PREV_GBUFFER_MOTION_CURVE     = "prev_gbuffer_motion_curve"_str;
    static constexpr auto PREV_GBUFFER_MESHID           = "prev_gbuffer_mesh_id"_str;
    static constexpr auto PREV_GBUFFER_DEPTH            = "prev_gbuffer_depth"_str;

    static constexpr auto GBUFFER_ALBEDO_METAL     = "gbuffer_albedo_metal"_str;
    static constexpr auto GBUFFER_EMISSIVE         = "gbuffer_emissive"_str;
    static constexpr auto GBUFFER_NORMAL_ROUGHNESS = "gbuffer_normal_roughness"_str;
    static constexpr auto GBUFFER_MOTION_CURVE     = "gbuffer_motion_curve"_str;
    static constexpr auto GBUFFER_MESHID           = "gbuffer_mesh_id"_str;
    static constexpr auto GBUFFER_DEPTH            = "gbuffer_depth"_str;

    static constexpr auto OUTPUT_FIELDS = Array{
      RenderNodeField::Texture2D(PREV_GBUFFER_NORMAL_ROUGHNESS),
      RenderNodeField::Texture2D(PREV_GBUFFER_MOTION_CURVE),
      RenderNodeField::Texture2D(PREV_GBUFFER_MESHID),
      RenderNodeField::Texture2D(PREV_GBUFFER_DEPTH),

      RenderNodeField::Texture2D(GBUFFER_ALBEDO_METAL),
      RenderNodeField::Texture2D(GBUFFER_EMISSIVE),
      RenderNodeField::Texture2D(GBUFFER_NORMAL_ROUGHNESS),
      RenderNodeField::Texture2D(GBUFFER_MOTION_CURVE),
      RenderNodeField::Texture2D(GBUFFER_MESHID),
      RenderNodeField::Texture2D(GBUFFER_DEPTH),
    };

    explicit GBufferGenerateNode(NotNull<gpu::System*> gpu_system);

    GBufferGenerateNode(const GBufferGenerateNode&) = delete;

    GBufferGenerateNode(GBufferGenerateNode&&) = delete;

    auto operator=(const GBufferGenerateNode&) -> GBufferGenerateNode& = delete;

    auto operator=(GBufferGenerateNode&&) -> GBufferGenerateNode& = delete;

    ~GBufferGenerateNode() override;

    [[nodiscard]]
    inline auto get_input_fields() const -> Span<const RenderNodeField*> override
    {
      return Span<const RenderNodeField*>(nullptr, 0);
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

    void setup_gbuffers(vec2u32 viewport);
  };
} // namespace renderlab
