#pragma once

#include "gpu/sl_type.h"
using namespace soul;

#include "deferred_shading.shared.hlsl"
#include "render_node.h"

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
  class DeferredShadingNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;
    gpu::ProgramID program_id_;

    f32 indirect_diffuse_intensity_  = 1.0f;
    f32 indirect_specular_intensity_ = 1.0f;

  public:
    static constexpr auto OUTPUT = "output"_str;

    static constexpr auto LIGHT_VISIBILITY_INPUT  = "light_visibility"_str;
    static constexpr auto AO_INPUT                = "ao_input"_str;
    static constexpr auto ALBEDO_METALLIC_INPUT   = "albedo_metallic"_str;
    static constexpr auto NORMAL_ROUGHNESS_INPUT  = "normal_roughness"_str;
    static constexpr auto MOTION_CURVE_INPUT      = "motion_curve"_str;
    static constexpr auto EMISSIVE_INPUT          = "emissive_input"_str;
    static constexpr auto DEPTH_INPUT             = "depth"_str;
    static constexpr auto INDIRECT_DIFFUSE_INPUT  = "indirect_diffuse"_str;
    static constexpr auto INDIRECT_SPECULAR_INPUT = "indirect_specular"_str;

    static constexpr auto INPUT_FIELDS = Array{
      RenderNodeField::Texture2D(LIGHT_VISIBILITY_INPUT),
      RenderNodeField::Texture2D(AO_INPUT),
      RenderNodeField::Texture2D(ALBEDO_METALLIC_INPUT),
      RenderNodeField::Texture2D(NORMAL_ROUGHNESS_INPUT),
      RenderNodeField::Texture2D(MOTION_CURVE_INPUT),
      RenderNodeField::Texture2D(DEPTH_INPUT),
      RenderNodeField::Texture2D(INDIRECT_DIFFUSE_INPUT),
      RenderNodeField::Texture2D(INDIRECT_SPECULAR_INPUT),
    };

    static constexpr auto OUTPUT_FIELDS = Array{
      RenderNodeField::Texture2D(OUTPUT),
    };

    explicit DeferredShadingNode(NotNull<gpu::System*> gpu_system);

    DeferredShadingNode(const DeferredShadingNode&) = delete;

    DeferredShadingNode(DeferredShadingNode&&) = delete;

    auto operator=(const DeferredShadingNode&) -> DeferredShadingNode& = delete;

    auto operator=(DeferredShadingNode&&) -> DeferredShadingNode& = delete;

    ~DeferredShadingNode() override;

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
