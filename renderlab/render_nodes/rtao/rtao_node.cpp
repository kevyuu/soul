#include "rtao_node.h"
#include "gpu/sl_type.h"
#include "gpu/type.h"
#include "render_nodes/render_constant_name.h"
#include "rtao.shared.hlsl"

#include "app/gui.h"
#include "core/not_null.h"
#include "core/type.h"
#include "gpu/render_graph.h"

namespace renderlab
{
  RtaoNode::RtaoNode(NotNull<gpu::System*> gpu_system) : gpu_system_(gpu_system)
  {
    const auto search_path      = Path::From("shaders"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::COMPUTE, "cs_main"_str},
    };
    const auto create_program_from_file = [&](const CompStr path_str)
    {
      const auto shader_source =
        gpu::ShaderSource::From(gpu::ShaderFile{.path = Path::From(path_str)});
      const gpu::ProgramDesc program_desc = {
        .search_paths = u32cspan(&search_path, 1),
        .sources      = u32cspan(&shader_source, 1),
        .entry_points = entry_points.cspan<u32>(),
      };
      return gpu_system_->create_program(program_desc).ok_ref();
    };
    ray_query_program_id_ = create_program_from_file("render_nodes/rtao/ray_query_main.hlsl"_str);
    init_dispatch_arg_program_id_ =
      create_program_from_file("render_nodes/rtao/init_dispatch_args_main.hlsl"_str);
    temporal_accumulation_program_id_ =
      create_program_from_file("render_nodes/rtao/temporal_accumulation_main.hlsl"_str);
    bilateral_blur_program_id_ =
      create_program_from_file("render_nodes/rtao/bilateral_blur_main.hlsl"_str);
  }

  auto RtaoNode::submit_pass(
    const Scene& scene,
    const RenderConstant& constant,
    const RenderData& inputs,
    NotNull<gpu::RenderGraph*> render_graph) -> RenderData
  {
    const auto viewport = scene.get_viewport();
    const auto frame_id = scene.render_data_cref().num_frames;
    setup_images(viewport);

    gpu::TextureNodeID ray_query_result_texture_node = render_graph->create_texture(
      "Rtao Ray Query Output"_str,
      gpu::RGTextureDesc::create_d2(
        gpu::TextureFormat::R32UI,
        1,
        {viewport.x / RAY_QUERY_WORK_GROUP_SIZE_X, viewport.y / RAY_QUERY_WORK_GROUP_SIZE_Y}));

    struct RayQueryParameter
    {
      gpu::BlasGroupNodeID blas_group;
      gpu::TlasNodeID tlas;
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID normal_roughness_texture;
      gpu::TextureNodeID depth_texture;
      gpu::TextureNodeID output_texture;
    };

    const auto ray_query_setup_fn =
      [&scene, &inputs, ray_query_result_texture_node](RayQueryParameter& parameter, auto& builder)
    {
      const auto& render_data = scene.render_data_cref();
      if (render_data.blas_group_node_id.is_valid())
      {
        parameter.blas_group = builder.add_shader_blas_group(
          render_data.blas_group_node_id, {gpu::ShaderStage::COMPUTE});
      }
      if (render_data.tlas_node_id.is_valid())
      {
        parameter.tlas =
          builder.add_shader_tlas(render_data.tlas_node_id, {gpu::ShaderStage::COMPUTE});
      }
      parameter.scene_buffer = scene.build_scene_dependencies(&builder);
      parameter.normal_roughness_texture =
        builder.add_srv(inputs.textures[GBUFFER_NORMAL_ROUGHNESS_INPUT]);
      parameter.depth_texture  = builder.add_srv(inputs.textures[GBUFFER_DEPTH_INPUT]);
      parameter.output_texture = builder.add_uav(ray_query_result_texture_node);
    };

    const auto ray_query_execute_fn =
      [this, viewport, &scene, &constant](
        const RayQueryParameter& parameter, auto& registry, auto& command_list)
    {
      const gpu::ComputePipelineStateDesc desc = {.program_id = ray_query_program_id_};

      RayQueryPC push_constant = {
        .gpu_scene_id = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
        .normal_roughness_texture =
          registry.get_srv_descriptor_id(parameter.normal_roughness_texture),
        .depth_texture  = registry.get_srv_descriptor_id(parameter.depth_texture),
        .output_texture = registry.get_uav_descriptor_id(parameter.output_texture),
        .sobol_texture =
          gpu_system_->get_srv_descriptor_id(constant.textures[RenderConstantName::SOBOL_TEXTURE]),
        .scrambling_ranking_texture = gpu_system_->get_srv_descriptor_id(
          constant.textures[RenderConstantName::SCRAMBLE_TEXTURE]),
        .bias       = bias_,
        .ray_min    = ray_min_,
        .ray_max    = ray_max_,
        .num_frames = scene.render_data_cref().num_frames,
      };

      const auto pipeline_state_id = registry.get_pipeline_state(desc);
      using Command                = gpu::RenderCommandDispatch;
      command_list.template push<Command>({
        .pipeline_state_id  = pipeline_state_id,
        .push_constant_data = &push_constant,
        .push_constant_size = sizeof(RayQueryPC),
        .group_count =
          {viewport.x / RAY_QUERY_WORK_GROUP_SIZE_X, viewport.y / RAY_QUERY_WORK_GROUP_SIZE_Y, 1},
      });
    };

    const auto& ray_query_node = render_graph->add_compute_pass<RayQueryParameter>(
      "AO Ray Query Pass"_str, ray_query_setup_fn, ray_query_execute_fn);

    ray_query_result_texture_node = ray_query_node.get_parameter().output_texture;

    // Init Dispatch Args Pass
    auto filter_dispatch_arg_buffer_node = render_graph->create_buffer(
      "Filter Dispatch Args"_str, {.size = sizeof(gpu::DispatchIndirectCommand)});

    struct InitDispatchArgsParameter
    {
      gpu::BufferNodeID filter_dispatch_arg_buffer;
    };

    const auto init_dispatch_args_setup_fn =
      [&](InitDispatchArgsParameter& parameter, auto& builder)
    {
      parameter.filter_dispatch_arg_buffer =
        builder.add_write_ssbo(filter_dispatch_arg_buffer_node);
    };

    const auto init_dispatch_args_execute_fn =
      [this](const InitDispatchArgsParameter& parameter, auto& registry, auto& command_list)
    {
      const gpu::ComputePipelineStateDesc desc = {.program_id = init_dispatch_arg_program_id_};
      InitDispatchArgsPC push_constant         = {
                .filter_dispatch_arg_buffer =
          registry.get_ssbo_descriptor_id(parameter.filter_dispatch_arg_buffer),
      };

      const auto pipeline_state_id = registry.get_pipeline_state(desc);
      using Command                = gpu::RenderCommandDispatch;
      command_list.template push<Command>({
        .pipeline_state_id  = pipeline_state_id,
        .push_constant_data = &push_constant,
        .push_constant_size = sizeof(InitDispatchArgsPC),
        .group_count        = {1, 1, 1},
      });
    };

    const auto& init_dispatch_args_pass = render_graph->add_compute_pass<InitDispatchArgsParameter>(
      "AO Init Dispatch Arg"_str, init_dispatch_args_setup_fn, init_dispatch_args_execute_fn);

    filter_dispatch_arg_buffer_node =
      init_dispatch_args_pass.get_parameter().filter_dispatch_arg_buffer;

    // Temporal Denoise pass
    auto temporal_accumulation_output_texture_node = render_graph->create_texture(
      "AO Temporal Accumulation Output"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::R16F, 1, viewport));

    auto feedback_ao_texture_node =
      render_graph->import_texture("Feedback AO"_str, feedback_ao_texture_id_);

    auto history_length_texture_node = render_graph->import_texture(
      "History Length Ouptut"_str, history_length_texture_ids_[frame_id % 2]);
    auto moment_length_history_texture_node = render_graph->import_texture(
      "Prev History Length"_str, history_length_texture_ids_[(frame_id + 1) % 2]);

    const vec2u32 temporal_dispatch_count = {
      math::ceil(f32(viewport.x) / TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X),
      math::ceil(f32(viewport.y) / TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y),
    };

    const auto max_coords = temporal_dispatch_count.x * temporal_dispatch_count.y;
    auto filter_coords_buffer_node =
      render_graph->create_buffer("Filter Texcoords"_str, {.size = sizeof(vec2u32) * max_coords});

    struct TemporalAccumulationParameter
    {
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID current_normal_roughness_gbuffer;
      gpu::TextureNodeID current_motion_curve_gbuffer;
      gpu::TextureNodeID current_meshid_gbuffer;
      gpu::TextureNodeID current_depth_gbuffer;

      gpu::TextureNodeID prev_normal_roughness_gbuffer;
      gpu::TextureNodeID prev_motion_curve_gbuffer;
      gpu::TextureNodeID prev_meshid_gbuffer;
      gpu::TextureNodeID prev_depth_gbuffer;

      gpu::TextureNodeID ray_query_result_texture;

      gpu::TextureNodeID output_val_texture;
      gpu::TextureNodeID output_history_length_texture;

      gpu::TextureNodeID prev_val_texture;
      gpu::TextureNodeID prev_history_length_texture;

      gpu::BufferNodeID filter_dispatch_arg_buffer;
      gpu::BufferNodeID filter_coords_buffer;
    };

    const auto temporal_denoise_setup_fn =
      [&](TemporalAccumulationParameter& parameter, auto& builder)
    {
      parameter.scene_buffer = scene.build_scene_dependencies(&builder);
      parameter.current_normal_roughness_gbuffer =
        builder.add_srv(inputs.textures[GBUFFER_NORMAL_ROUGHNESS_INPUT]);
      parameter.current_motion_curve_gbuffer =
        builder.add_srv(inputs.textures[GBUFFER_MOTION_CURVE_INPUT]);
      parameter.current_meshid_gbuffer = builder.add_srv(inputs.textures[GBUFFER_MESHID_INPUT]);
      parameter.current_depth_gbuffer  = builder.add_srv(inputs.textures[GBUFFER_DEPTH_INPUT]);

      parameter.prev_normal_roughness_gbuffer =
        builder.add_srv(inputs.textures[PREV_GBUFFER_NORMAL_ROUGHNESS_INPUT]);
      parameter.prev_motion_curve_gbuffer =
        builder.add_srv(inputs.textures[PREV_GBUFFER_MOTION_CURVE_INPUT]);
      parameter.prev_meshid_gbuffer = builder.add_srv(inputs.textures[PREV_GBUFFER_MESHID_INPUT]);
      parameter.prev_depth_gbuffer  = builder.add_srv(inputs.textures[PREV_GBUFFER_DEPTH_INPUT]);

      parameter.ray_query_result_texture = builder.add_srv(ray_query_result_texture_node);

      parameter.output_val_texture = builder.add_uav(temporal_accumulation_output_texture_node);
      parameter.output_history_length_texture = builder.add_uav(history_length_texture_node);

      parameter.prev_val_texture            = builder.add_srv(feedback_ao_texture_node);
      parameter.prev_history_length_texture = builder.add_srv(moment_length_history_texture_node);

      parameter.filter_dispatch_arg_buffer =
        builder.add_write_ssbo(filter_dispatch_arg_buffer_node);
      parameter.filter_coords_buffer = builder.add_write_ssbo(filter_coords_buffer_node);
    };

    const auto temporal_denoise_execute_fn =
      [this, temporal_dispatch_count, &scene](
        const TemporalAccumulationParameter& parameter, auto& registry, auto& command_list)
    {
      const gpu::ComputePipelineStateDesc desc = {.program_id = temporal_accumulation_program_id_};

      TemporalAccumulationPC push_constant = {
        .gpu_scene_id = registry.get_ssbo_descriptor_id(parameter.scene_buffer),

        .current_normal_roughness_gbuffer =
          registry.get_srv_descriptor_id(parameter.current_normal_roughness_gbuffer),
        .current_motion_curve_gbuffer =
          registry.get_srv_descriptor_id(parameter.current_motion_curve_gbuffer),
        .current_meshid_gbuffer = registry.get_srv_descriptor_id(parameter.current_meshid_gbuffer),
        .current_depth_gbuffer  = registry.get_srv_descriptor_id(parameter.current_depth_gbuffer),

        .prev_normal_roughness_gbuffer =
          registry.get_srv_descriptor_id(parameter.prev_normal_roughness_gbuffer),
        .prev_motion_curve_gbuffer =
          registry.get_srv_descriptor_id(parameter.prev_motion_curve_gbuffer),
        .prev_meshid_gbuffer = registry.get_srv_descriptor_id(parameter.prev_meshid_gbuffer),
        .prev_depth_gbuffer  = registry.get_srv_descriptor_id(parameter.prev_depth_gbuffer),

        .ray_query_result_texture =
          registry.get_srv_descriptor_id(parameter.ray_query_result_texture),

        .output_val_texture = registry.get_uav_descriptor_id(parameter.output_val_texture),
        .output_history_length_texture =
          registry.get_uav_descriptor_id(parameter.output_history_length_texture),

        .prev_val_texture = registry.get_srv_descriptor_id(parameter.prev_val_texture),
        .prev_history_length_texture =
          registry.get_srv_descriptor_id(parameter.prev_history_length_texture),

        .filter_dispatch_arg_buffer =
          registry.get_ssbo_descriptor_id(parameter.filter_dispatch_arg_buffer),
        .filter_coords_buffer = registry.get_ssbo_descriptor_id(parameter.filter_coords_buffer),
        .alpha                = alpha_,
      };

      const auto pipeline_state_id = registry.get_pipeline_state(desc);
      using Command                = gpu::RenderCommandDispatch;
      command_list.template push<Command>({
        .pipeline_state_id  = pipeline_state_id,
        .push_constant_data = &push_constant,
        .push_constant_size = sizeof(TemporalAccumulationPC),
        .group_count        = {temporal_dispatch_count, 1},
      });
    };

    const auto& temporal_accumulation_pass =
      render_graph->add_compute_pass<TemporalAccumulationParameter>(
        "AO Temporal Accumulation"_str, temporal_denoise_setup_fn, temporal_denoise_execute_fn);

    temporal_accumulation_output_texture_node =
      temporal_accumulation_pass.get_parameter().output_val_texture;

    filter_dispatch_arg_buffer_node =
      temporal_accumulation_pass.get_parameter().filter_dispatch_arg_buffer;

    filter_coords_buffer_node = temporal_accumulation_pass.get_parameter().filter_coords_buffer;

    // Bilateral blur pass
    struct BilateralBlurParameter
    {
      gpu::BufferNodeID filter_dispatch_arg_buffer;
      gpu::BufferNodeID filter_coords_buffer;
      gpu::TextureNodeID gbuffer_normal_roughness;
      gpu::TextureNodeID gbuffer_depth;
      gpu::TextureNodeID ao_input_texture;
      gpu::TextureNodeID output_texture;
    };

    gpu::TextureNodeID horizontal_blur_output_node = render_graph->create_texture(
      "Horizontal Bilateral Blur Output"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::R16F, 1, viewport));

    horizontal_blur_output_node = render_graph->clear_texture(
      gpu::QueueType::COMPUTE,
      horizontal_blur_output_node,
      gpu::ClearValue(vec4f32(1, 0, 0, 0), 1, 1));

    const auto& horizontal_blur_pass = render_graph->add_compute_pass<BilateralBlurParameter>(
      "AO Horizontal Blur Pass"_str,
      [&](BilateralBlurParameter& parameter, auto& builder)
      {
        parameter.filter_dispatch_arg_buffer =
          builder.add_indirect_command_buffer(filter_dispatch_arg_buffer_node);
        parameter.filter_coords_buffer = builder.add_read_ssbo(filter_coords_buffer_node);
        parameter.gbuffer_normal_roughness =
          builder.add_srv(inputs.textures[GBUFFER_NORMAL_ROUGHNESS_INPUT]);
        parameter.gbuffer_depth    = builder.add_srv(inputs.textures[GBUFFER_DEPTH_INPUT]);
        parameter.ao_input_texture = builder.add_srv(temporal_accumulation_output_texture_node);
        parameter.output_texture   = builder.add_uav(horizontal_blur_output_node);
      },
      [this](const BilateralBlurParameter& parameter, auto& registry, auto& command_list)
      {
        BilateralBlurPC blur_pc = {
          .filter_coords_buffer = registry.get_ssbo_descriptor_id(parameter.filter_coords_buffer),
          .gbuffer_normal_roughness =
            registry.get_srv_descriptor_id(parameter.gbuffer_normal_roughness),
          .gbuffer_depth    = registry.get_srv_descriptor_id(parameter.gbuffer_depth),
          .ao_input_texture = registry.get_srv_descriptor_id(parameter.ao_input_texture),
          .output_texture   = registry.get_uav_descriptor_id(parameter.output_texture),
          .radius           = radius_,
          .direction        = vec2i32(1, 0),
        };

        using Command = gpu::RenderCommandDispatchIndirect;
        command_list.template push<Command>({
          .pipeline_state_id = registry.get_pipeline_state(
            gpu::ComputePipelineStateDesc{.program_id = bilateral_blur_program_id_}),
          .push_constant = {cast<byte*>(&blur_pc), sizeof(blur_pc)},
          .buffer        = registry.get_buffer(parameter.filter_dispatch_arg_buffer),
        });
      });

    horizontal_blur_output_node = horizontal_blur_pass.get_parameter().output_texture;

    const auto& vertical_blur_pass = render_graph->add_compute_pass<BilateralBlurParameter>(
      "AO Vertical Blur Pass"_str,
      [&](BilateralBlurParameter& parameter, auto& builder)
      {
        parameter.filter_dispatch_arg_buffer =
          builder.add_indirect_command_buffer(filter_dispatch_arg_buffer_node);
        parameter.filter_coords_buffer = builder.add_read_ssbo(filter_coords_buffer_node);
        parameter.gbuffer_normal_roughness =
          builder.add_srv(inputs.textures[GBUFFER_NORMAL_ROUGHNESS_INPUT]);
        parameter.gbuffer_depth    = builder.add_srv(inputs.textures[GBUFFER_DEPTH_INPUT]);
        parameter.ao_input_texture = builder.add_srv(horizontal_blur_output_node);
        parameter.output_texture   = builder.add_uav(feedback_ao_texture_node);
      },
      [this](const BilateralBlurParameter& parameter, auto& registry, auto& command_list)
      {
        BilateralBlurPC blur_pc = {
          .filter_coords_buffer = registry.get_ssbo_descriptor_id(parameter.filter_coords_buffer),
          .gbuffer_normal_roughness =
            registry.get_srv_descriptor_id(parameter.gbuffer_normal_roughness),
          .gbuffer_depth    = registry.get_srv_descriptor_id(parameter.gbuffer_depth),
          .ao_input_texture = registry.get_srv_descriptor_id(parameter.ao_input_texture),
          .output_texture   = registry.get_uav_descriptor_id(parameter.output_texture),
          .radius           = radius_,
          .direction        = vec2i32(0, 1),
        };

        using Command = gpu::RenderCommandDispatchIndirect;
        command_list.template push<Command>({
          .pipeline_state_id = registry.get_pipeline_state(
            gpu::ComputePipelineStateDesc{.program_id = bilateral_blur_program_id_}),
          .push_constant = {cast<byte*>(&blur_pc), sizeof(blur_pc)},
          .buffer        = registry.get_buffer(parameter.filter_dispatch_arg_buffer),
        });
      });

    feedback_ao_texture_node = vertical_blur_pass.get_parameter().output_texture;

    RenderData outputs;
    outputs.textures.insert(String::From(OUTPUT), feedback_ao_texture_node);
    outputs.textures.insert(
      String::From(HISTORY_LENGTH_OUTPUT),
      temporal_accumulation_pass.get_parameter().output_history_length_texture);
    return outputs;
  }

  void RtaoNode::on_gui_render(NotNull<app::Gui*> gui)
  {
    gui->input_f32("Trace Normal Bias"_str, &bias_);
    gui->input_f32("Alpha"_str, &alpha_);
  }

  auto RtaoNode::get_gui_label() const -> CompStr
  {
    return "Rtao Node"_str;
  }

  RtaoNode::~RtaoNode()
  {
    gpu_system_->destroy_program(ray_query_program_id_);
    gpu_system_->destroy_program(init_dispatch_arg_program_id_);
    gpu_system_->destroy_program(temporal_accumulation_program_id_);
    gpu_system_->destroy_program(bilateral_blur_program_id_);
  }

  void RtaoNode::setup_images(vec2u32 viewport)
  {
    if (all(viewport_ == viewport))
    {
      return;
    }

    viewport_ = viewport;

    gpu_system_->destroy_texture(feedback_ao_texture_id_);
    feedback_ao_texture_id_ = gpu_system_->create_texture(
      "Moment Texture"_str,
      gpu::TextureDesc::d2(

        gpu::TextureFormat::R16F,
        1,
        {
          gpu::TextureUsage::STORAGE,
          gpu::TextureUsage::SAMPLED,
        },
        {
          gpu::QueueType::COMPUTE,
        },
        viewport));

    for (auto& texture_id : history_length_texture_ids_)
    {
      gpu_system_->destroy_texture(texture_id);
      texture_id = gpu_system_->create_texture(
        "Filter Output Texture"_str,
        gpu::TextureDesc::d2(

          gpu::TextureFormat::R16F,
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
} // namespace renderlab
