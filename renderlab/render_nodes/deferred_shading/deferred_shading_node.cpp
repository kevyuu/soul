#include "deferred_shading_node.h"

#include "app/gui.h"
#include "core/not_null.h"
#include "core/type.h"
#include "gpu/render_graph.h"
#include "render_nodes/render_constant_name.h"

namespace renderlab
{
  DeferredShadingNode::DeferredShadingNode(NotNull<gpu::System*> gpu_system)
      : gpu_system_(gpu_system)
  {
    const auto shader_source    = gpu::ShaderSource::From(gpu::ShaderFile{
         .path = Path::From("render_nodes/deferred_shading/deferred_shading_main.hlsl"_str)});
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

  auto DeferredShadingNode::submit_pass(
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
      "Deferred Shading Output Texture",
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, viewport));

    struct ComputePassParameter
    {
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID light_visibility_texture;
      gpu::TextureNodeID ao_texture;
      gpu::TextureNodeID albedo_metallic_texture;
      gpu::TextureNodeID motion_curve_texture;
      gpu::TextureNodeID normal_roughness_texture;
      gpu::TextureNodeID depth_texture;
      gpu::TextureNodeID indirect_diffuse_texture;
      gpu::TextureNodeID indirect_specular_texture;
      gpu::TextureNodeID output_texture;
    };

    const auto& compute_pass = render_graph->add_compute_pass<ComputePassParameter>(
      "Deferred Shading Pass",
      [&scene, &inputs, output_texture](ComputePassParameter& parameter, auto& builder)
      {
        parameter.scene_buffer = scene.build_scene_dependencies(&builder);
        parameter.light_visibility_texture =
          builder.add_srv(inputs.textures[LIGHT_VISIBILITY_INPUT]);
        parameter.ao_texture              = builder.add_srv(inputs.textures[AO_INPUT]);
        parameter.albedo_metallic_texture = builder.add_srv(inputs.textures[ALBEDO_METALLIC_INPUT]);
        parameter.motion_curve_texture    = builder.add_srv(inputs.textures[MOTION_CURVE_INPUT]);
        parameter.normal_roughness_texture =
          builder.add_srv(inputs.textures[NORMAL_ROUGHNESS_INPUT]);
        parameter.depth_texture = builder.add_srv(inputs.textures[DEPTH_INPUT]);
        parameter.indirect_diffuse_texture =
          builder.add_srv(inputs.textures[INDIRECT_DIFFUSE_INPUT]);
        parameter.indirect_specular_texture =
          builder.add_srv(inputs.textures[INDIRECT_SPECULAR_INPUT]);
        parameter.output_texture = builder.add_uav(output_texture);
      },
      [this, viewport, &constant](
        const ComputePassParameter& parameter, auto& registry, auto& command_list)
      {
        const gpu::ComputePipelineStateDesc desc = {.program_id = program_id_};

        DeferredShadingPC push_constant = {
          .gpu_scene_id = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
          .light_visibility_texture =
            registry.get_srv_descriptor_id(parameter.light_visibility_texture),
          .ao_texture = registry.get_srv_descriptor_id(parameter.ao_texture),
          .albedo_metallic_texture =
            registry.get_srv_descriptor_id(parameter.albedo_metallic_texture),
          .motion_curve_texture = registry.get_srv_descriptor_id(parameter.motion_curve_texture),
          .normal_roughness_texture =
            registry.get_srv_descriptor_id(parameter.normal_roughness_texture),
          .depth_texture = registry.get_srv_descriptor_id(parameter.depth_texture),
          .indirect_diffuse_texture =
            registry.get_srv_descriptor_id(parameter.indirect_diffuse_texture),
          .indirect_specular_texture =
            registry.get_srv_descriptor_id(parameter.indirect_specular_texture),
          .brdf_lut_texture = gpu_system_->get_srv_descriptor_id(
            constant.textures[RenderConstantName::BRDF_LUT_TEXTURE]),
          .output_texture              = registry.get_uav_descriptor_id(parameter.output_texture),
          .indirect_diffuse_intensity  = indirect_diffuse_intensity_,
          .indirect_specular_intensity = indirect_specular_intensity_,
        };

        const auto pipeline_state_id = registry.get_pipeline_state(desc);
        using Command                = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(DeferredShadingPC),
          .group_count        = {viewport.x / WORK_GROUP_SIZE_X, viewport.y / WORK_GROUP_SIZE_Y, 1},
        });
      });

    RenderData outputs;
    outputs.textures.insert(String::From(OUTPUT), compute_pass.get_parameter().output_texture);
    return outputs;
  }

  void DeferredShadingNode::on_gui_render(NotNull<app::Gui*> gui)
  {
    gui->input_f32("Indirect Diffuse Intensity"_str, &indirect_diffuse_intensity_);
    gui->input_f32("Indirect Specular Intensity"_str, &indirect_specular_intensity_);
  }

  auto DeferredShadingNode::get_gui_label() const -> CompStr
  {
    return "Deferred Shading"_str;
  }

  DeferredShadingNode::~DeferredShadingNode()
  {
    gpu_system_->destroy_program(program_id_);
  }
} // namespace renderlab
