#include "taa_node.h"

#include "taa.shared.hlsl"

#include "app/gui.h"
#include "core/not_null.h"
#include "core/type.h"
#include "gpu/render_graph.h"

namespace renderlab
{
  TaaNode::TaaNode(NotNull<gpu::System*> gpu_system) : gpu_system_(gpu_system)
  {
    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("render_nodes/taa/taa_main.hlsl"_str)});
    const auto search_path      = Path::From("shaders"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::COMPUTE, "cs_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources      = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    program_id_ = gpu_system_->create_program(program_desc).ok_ref();
  }

  auto TaaNode::submit_pass(
    const Scene& scene,
    const RenderConstant& constant,
    const RenderData& inputs,
    NotNull<gpu::RenderGraph*> render_graph) -> RenderData
  {
    if (!enable_pass_)
    {
      RenderData outputs;
      outputs.textures.insert(String::From(OUTPUT), inputs.textures[COLOR_INPUT]);
      return outputs;
    }
    const auto viewport = scene.get_viewport();
    setup_images(viewport);

    const auto frame_id = scene.render_data_cref().num_frames;

    const auto output_texture_node =
      render_graph->import_texture("Output Texture", color_textures_[frame_id % 2]);

    const auto history_color_texture_node =
      render_graph->import_texture("History Color Texture", color_textures_[(frame_id + 1) % 2]);

    struct ComputePassParameter
    {
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID current_color_texture;
      gpu::TextureNodeID history_color_texture;
      gpu::TextureNodeID motion_curve_gbuffer;
      gpu::TextureNodeID depth_gbuffer;
      gpu::TextureNodeID output_texture;
    };

    const auto& compute_pass = render_graph->add_compute_pass<ComputePassParameter>(
      "TAA Pass",
      [&](ComputePassParameter& parameter, auto& builder)
      {
        parameter.scene_buffer          = scene.build_scene_dependencies(&builder);
        parameter.current_color_texture = builder.add_srv(inputs.textures[COLOR_INPUT]);
        parameter.history_color_texture = builder.add_srv(history_color_texture_node);
        parameter.motion_curve_gbuffer =
          builder.add_srv(inputs.textures[GBUFFER_MOTION_CURVE_INPUT]);
        parameter.depth_gbuffer  = builder.add_srv(inputs.textures[GBUFFER_DEPTH_INPUT]);
        parameter.output_texture = builder.add_uav(output_texture_node);
      },
      [this, viewport, &scene](
        const ComputePassParameter& parameter, auto& registry, auto& command_list)
      {
        const gpu::ComputePipelineStateDesc desc = {.program_id = program_id_};

        TaaPC push_constant = {
          .gpu_scene_buffer      = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
          .current_color_texture = registry.get_srv_descriptor_id(parameter.current_color_texture),
          .history_color_texture = registry.get_srv_descriptor_id(parameter.history_color_texture),
          .motion_curve_gbuffer  = registry.get_srv_descriptor_id(parameter.motion_curve_gbuffer),
          .depth_gbuffer         = registry.get_srv_descriptor_id(parameter.depth_gbuffer),
          .output_texture        = registry.get_uav_descriptor_id(parameter.output_texture),
          .feedback_min          = feedback_min_,
          .feedback_max          = feedback_max_,
          .sharpen_enable        = sharpen_enable_,
          .dilation_enable       = dilation_enable_,
        };

        const auto pipeline_state_id = registry.get_pipeline_state(desc);
        using Command                = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(TaaPC),
          .group_count        = {viewport.x / WORK_GROUP_SIZE_X, viewport.y / WORK_GROUP_SIZE_Y, 1},
        });
      });

    RenderData outputs;
    outputs.textures.insert(String::From(OUTPUT), compute_pass.get_parameter().output_texture);
    outputs.textures.insert(String::From(HISTORY_OUTPUT), history_color_texture_node);
    return outputs;
  }

  void TaaNode::on_gui_render(NotNull<app::Gui*> gui)
  {
    gui->checkbox("Enable"_str, &enable_pass_);
    gui->slider_f32("Min Feedback"_str, &feedback_min_, 0.0f, 1.0f);
    gui->slider_f32("Max Feedback"_str, &feedback_max_, 0.0f, 1.0f);
    gui->checkbox("Sharpen Enable"_str, &sharpen_enable_);
    gui->checkbox("Dilation Enable"_str, &dilation_enable_);
  }

  auto TaaNode::get_gui_label() const -> CompStr
  {
    return "Temporal Anti-Aliasing"_str;
  }

  void TaaNode::setup_images(vec2u32 viewport)
  {
    if (all(viewport_ == viewport))
    {
      return;
    }

    viewport_ = viewport;
    for (gpu::TextureID& texture_id : color_textures_)
    {
      gpu_system_->destroy_texture(texture_id);
      texture_id = gpu_system_->create_texture(gpu::TextureDesc::d2(
        "Moment Texture",
        gpu::TextureFormat::RGBA16F,
        1,
        {
          gpu::TextureUsage::STORAGE,
          gpu::TextureUsage::SAMPLED,
        },
        {
          gpu::QueueType::COMPUTE,
        },
        viewport));
    }
  }

  TaaNode::~TaaNode()
  {
    gpu_system_->destroy_program(program_id_);
  }
} // namespace renderlab
