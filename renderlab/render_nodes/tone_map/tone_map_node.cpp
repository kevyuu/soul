#include "tone_map_node.h"

#include "tone_map.shared.hlsl"

#include "app/gui.h"
#include "core/not_null.h"
#include "core/type.h"
#include "gpu/render_graph.h"

namespace renderlab
{
  ToneMapNode::ToneMapNode(NotNull<gpu::System*> gpu_system) : gpu_system_(gpu_system)
  {
    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("render_nodes/tone_map/tone_map_main.hlsl"_str)});
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

  auto ToneMapNode::submit_pass(
    const Scene& scene,
    const RenderConstant& constant,
    const RenderData& inputs,
    NotNull<gpu::RenderGraph*> render_graph) -> RenderData
  {
    struct Parameter
    {
      gpu::BufferNodeID scene_buffer;
    };

    const auto viewport = scene.get_viewport();

    const gpu::TextureNodeID output_texture = render_graph->create_texture(
      "Tone Map Output Texture",
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, viewport));

    struct ComputePassParameter
    {
      gpu::TextureNodeID input_texture;
      gpu::TextureNodeID output_texture;
    };

    const auto& compute_pass = render_graph->add_compute_pass<ComputePassParameter>(
      "Tone Map Pass",
      [&scene, &inputs, output_texture](ComputePassParameter& parameter, auto& builder)
      {
        parameter.input_texture  = builder.add_srv(inputs.textures[INPUT]);
        parameter.output_texture = builder.add_uav(output_texture);
      },
      [this, viewport, &scene](
        const ComputePassParameter& parameter, auto& registry, auto& command_list)
      {
        const gpu::ComputePipelineStateDesc desc = {.program_id = program_id_};

        ToneMapPC push_constant = {
          .input_texture  = registry.get_srv_descriptor_id(parameter.input_texture),
          .output_texture = registry.get_uav_descriptor_id(parameter.output_texture),
        };

        const auto pipeline_state_id = registry.get_pipeline_state(desc);
        using Command                = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(ToneMapPC),
          .group_count        = {viewport.x / WORK_GROUP_SIZE_X, viewport.y / WORK_GROUP_SIZE_Y, 1},
        });
      });

    RenderData outputs;
    outputs.textures.insert(String::From(OUTPUT), compute_pass.get_parameter().output_texture);
    return outputs;
  }

  void ToneMapNode::on_gui_render(NotNull<app::Gui*> gui) {}

  auto ToneMapNode::get_gui_label() const -> CompStr
  {
    return "Tone Map"_str;
  }

  ToneMapNode::~ToneMapNode()
  {
    gpu_system_->destroy_program(program_id_);
  }
} // namespace renderlab
