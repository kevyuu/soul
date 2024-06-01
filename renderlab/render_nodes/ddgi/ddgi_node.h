#pragma once

#include "gpu/sl_type.h"

#include "ddgi.shared.hlsl"
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

  class DdgiNode : public RenderNode
  {
  private:
    NotNull<gpu::System*> gpu_system_;

    DdgiVolume volume_;

    gpu::ProgramID ray_trace_program_id_;
    gpu::ProgramID probe_update_program_id_;
    gpu::ProgramID probe_border_update_program_id_;
    gpu::ProgramID sample_irradiance_program_id_;
    gpu::ProgramID probe_overlay_program_id_;
    gpu::ProgramID ray_overlay_program_id_;
    gpu::ShaderTableID shader_table_id_;

    gpu::TextureID history_depth_probe_texture_;
    gpu::TextureID history_radiance_probe_texture_;

    std::random_device random_device_;
    std::mt19937 random_generator_;
    std::uniform_real_distribution<float> random_distribution_zo_;
    std::uniform_real_distribution<float> random_distribution_no_;

    math::AABB scene_aabb_;
    u32 frame_counter_;

    b8 enable_random_probe_ray_rotation_ = true;
    vec3f32 grid_step_;
    vec3i32 probe_count_;
    vec3f32 grid_start_position_;

    b8 show_overlay_             = false;
    f32 probe_overlay_radius_    = 0.2f;
    b8 show_ray_overlay_         = false;
    i32 ray_overlay_probe_index_ = 0;

    enum class ProbePlacementUpdateMode : u32
    {
      GRID_STEP_AND_SCENE_AABB,
      PROBE_COUNT_AND_SCENE_AABB,
      MANUAL,
      COUNT,
    };
    ProbePlacementUpdateMode probe_placement_update_mode_;
    b8 probe_placement_dirty_;

  public:
    static constexpr auto NORMAL_ROUGHNESS_INPUT = "normal_roughness"_str;
    static constexpr auto DEPTH_INPUT            = "depth"_str;

    static constexpr auto OUTPUT = "output"_str;

    static constexpr auto INPUT_FIELDS = Array{
      RenderNodeField::Texture2D(NORMAL_ROUGHNESS_INPUT),
      RenderNodeField::Texture2D(DEPTH_INPUT),
    };

    static constexpr auto OUTPUT_FIELDS = Array{
      RenderNodeField::Texture2D(OUTPUT),
    };

    explicit DdgiNode(NotNull<gpu::System*> gpu_system);

    DdgiNode(const DdgiNode&) = delete;

    DdgiNode(DdgiNode&&) = delete;

    auto operator=(const DdgiNode&) -> DdgiNode& = delete;

    auto operator=(DdgiNode&&) -> DdgiNode& = delete;

    ~DdgiNode() override;

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

    void reset_probe_grids();
  };
} // namespace renderlab
