#include "rt_reflection_node.h"
#include "gpu/sl_type.h"
#include "gpu/type.h"
#include "render_nodes/render_constant_name.h"
#include "rt_reflection.shared.hlsl"
#include "utils/util.h"

#include "app/gui.h"
#include "core/not_null.h"
#include "core/type.h"
#include "gpu/render_graph.h"

namespace renderlab
{
  RtReflectionNode::RtReflectionNode(NotNull<gpu::System*> gpu_system) : gpu_system_(gpu_system)
  {
    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("render_nodes/rt_reflection/ray_trace_main.hlsl"_str)});
    const auto search_path  = Path::From("shaders"_str);
    const auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::RAYGEN, "rgen_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::MISS, "rmiss_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::CLOSEST_HIT, "rchit_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources      = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system_->create_program(program_desc);
    if (result.is_err())
    {
      SOUL_PANIC("Fail to create program");
    }
    ray_trace_program_id_ = result.ok_ref();

    const auto miss_groups = soul::Array{
      gpu::RTGeneralShaderGroup{.entry_point = 1},
    };

    const gpu::RTTriangleHitGroup hit_group = {
      .closest_hit_entry_point = 2,
    };

    const gpu::ShaderTableDesc shader_table_desc = {
      .program_id   = ray_trace_program_id_,
      .raygen_group = {.entry_point = 0},
      .miss_groups  = u32cspan(miss_groups.data(), miss_groups.size()),
      .hit_groups   = u32cspan(&hit_group, 1),
      .name         = "Shader Table",
    };
    shader_table_id_ = gpu_system_->create_shader_table(shader_table_desc);

    init_dispatch_args_program_id_ = util::create_compute_program(
      gpu_system_, "render_nodes/rt_reflection/init_dispatch_args_main.hlsl"_str);
    temporal_accumulation_program_id_ = util::create_compute_program(
      gpu_system_, "render_nodes/rt_reflection/temporal_accumulation_main.hlsl"_str);
    filter_tile_program_id_ = util::create_compute_program(
      gpu_system_, "render_nodes/rt_reflection/filter_tile_main.hlsl"_str);
    copy_tile_program_id_ = util::create_compute_program(
      gpu_system_, "render_nodes/rt_reflection/copy_tile_main.hlsl"_str);
  }

  auto RtReflectionNode::submit_pass(
    const Scene& scene,
    const RenderConstant& constant,
    const RenderData& inputs,
    NotNull<gpu::RenderGraph*> render_graph) -> RenderData
  {
    const auto viewport = scene.get_viewport();
    const auto frame_id = scene.render_data_cref().num_frames;
    setup_images(viewport);

    gpu::TextureNodeID ray_trace_result_texture_node = render_graph->create_texture(
      "RayTraced Relection Output",
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, {viewport.x, viewport.y}));

    // RayQuery Pass
    struct RayTraceParameter
    {
      gpu::BlasGroupNodeID blas_group;
      gpu::TlasNodeID tlas;
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID normal_roughness_texture;
      gpu::TextureNodeID depth_texture;
      gpu::TextureNodeID output_texture;
    };

    const auto ray_trace_setup_fn = [&](auto& parameter, auto& builder)
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
      parameter.output_texture = builder.add_uav(ray_trace_result_texture_node);
    };

    const auto ray_trace_execute_fn = [this, viewport, frame_id, &scene, &constant](
                                        const auto& parameter, auto& registry, auto& command_list)
    {
      RayTracePC push_constant = {
        .gpu_scene_id =
          gpu_system_->get_ssbo_descriptor_id(registry.get_buffer(parameter.scene_buffer)),
        .sobol_texture =
          gpu_system_->get_srv_descriptor_id(constant.textures[RenderConstantName::SOBOL_TEXTURE]),
        .scrambling_ranking_texture = gpu_system_->get_srv_descriptor_id(
          constant.textures[RenderConstantName::SCRAMBLE_TEXTURE]),
        .depth_gbuffer =
          gpu_system_->get_srv_descriptor_id(registry.get_texture(parameter.depth_texture)),
        .normal_roughness_gbuffer = gpu_system_->get_srv_descriptor_id(
          registry.get_texture(parameter.normal_roughness_texture)),
        .output_texture =
          gpu_system_->get_uav_descriptor_id(registry.get_texture(parameter.output_texture)),
        .num_frames        = u32(frame_id),
        .trace_normal_bias = trace_normal_bias,
        .lobe_trim         = lobe_trim,
      };

      using Command = gpu::RenderCommandRayTrace;
      command_list.template push<Command>({
        .shader_table_id    = shader_table_id_,
        .push_constant_data = &push_constant,
        .push_constant_size = sizeof(RayTracePC),
        .dimension          = vec3u32(viewport, 1.0),
      });
    };

    const auto& ray_trace_node = render_graph->add_ray_tracing_pass<RayTraceParameter>(
      "Reflection Ray Tracing Pass", ray_trace_setup_fn, ray_trace_execute_fn);

    ray_trace_result_texture_node = ray_trace_node.get_parameter().output_texture;

    // Init Dispatch Args Pass
    auto filter_dispatch_arg_buffer_node = render_graph->create_buffer(
      "Filter Dispatch Args", {.size = sizeof(gpu::DispatchIndirectCommand)});

    auto copy_dispatch_arg_buffer_node = render_graph->create_buffer(
      "Copy Dispatch Args", {.size = sizeof(gpu::DispatchIndirectCommand)});

    struct InitDispatchArgsParameter
    {
      gpu::BufferNodeID filter_dispatch_arg_buffer;
      gpu::BufferNodeID copy_dispatch_arg_buffer;
    };

    const auto init_dispatch_args_setup_fn =
      [&](InitDispatchArgsParameter& parameter, auto& builder)
    {
      parameter.filter_dispatch_arg_buffer = builder.add_shader_buffer(
        filter_dispatch_arg_buffer_node,
        {gpu::ShaderStage::COMPUTE},
        gpu::ShaderBufferWriteUsage::STORAGE);
      parameter.copy_dispatch_arg_buffer = builder.add_shader_buffer(
        copy_dispatch_arg_buffer_node,
        {gpu::ShaderStage::COMPUTE},
        gpu::ShaderBufferWriteUsage::STORAGE);
    };

    const auto init_dispatch_args_execute_fn =
      [this](const InitDispatchArgsParameter& parameter, auto& registry, auto& command_list)
    {
      const gpu::ComputePipelineStateDesc desc = {
        .program_id = init_dispatch_args_program_id_,
      };

      InitDispatchArgsPC push_constant = {
        .filter_dispatch_arg_buffer = gpu_system_->get_ssbo_descriptor_id(
          registry.get_buffer(parameter.filter_dispatch_arg_buffer)),
        .copy_dispatch_arg_buffer = gpu_system_->get_ssbo_descriptor_id(
          registry.get_buffer(parameter.copy_dispatch_arg_buffer)),
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
      "Init Dispatch Args", init_dispatch_args_setup_fn, init_dispatch_args_execute_fn);

    filter_dispatch_arg_buffer_node =
      init_dispatch_args_pass.get_parameter().filter_dispatch_arg_buffer;

    copy_dispatch_arg_buffer_node =
      init_dispatch_args_pass.get_parameter().copy_dispatch_arg_buffer;

    // Temporal Acummulation pass
    auto temporal_accumulation_output_texture_node = render_graph->import_texture(
      "Temporal Acummulation Output", temporal_accumulation_output_texture_);
    auto atrous_feedback_texture_node =
      render_graph->import_texture("History Temporal Accumulation", atrous_feedback_texture_);
    auto moment_length_ouptut_texture_node =
      render_graph->import_texture("Moment Length Ouptut", moment_textures_[frame_id % 2]);
    auto moment_length_history_texture_node =
      render_graph->import_texture("Moment Length History", moment_textures_[(frame_id + 1) % 2]);

    const vec2u32 temporal_dispatch_count = {
      math::ceil(f32(viewport.x) / TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X),
      math::ceil(f32(viewport.y) / TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y),
    };

    const auto max_coords = temporal_dispatch_count.x * temporal_dispatch_count.y;
    auto filter_coords_buffer_node =
      render_graph->create_buffer("Filter Texcoords", {.size = sizeof(vec2u32) * max_coords});
    auto copy_coords_buffer_node =
      render_graph->create_buffer("Copy Texcoords", {.size = sizeof(vec2u32) * max_coords});

    struct TemporalAcummulationParameter
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

      gpu::TextureNodeID ray_trace_result_texture;

      gpu::TextureNodeID output_color_variance_texture;
      gpu::TextureNodeID output_moment_length_texture;

      gpu::TextureNodeID prev_color_variance_texture;
      gpu::TextureNodeID prev_moment_length_texture;

      gpu::BufferNodeID filter_dispatch_arg_buffer;
      gpu::BufferNodeID copy_dispatch_arg_buffer;
      gpu::BufferNodeID filter_coords_buffer;
      gpu::BufferNodeID copy_coords_buffer;
    };

    const auto temporal_accumulation_setup_fn =
      [&](TemporalAcummulationParameter& parameter, auto& builder)
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

      parameter.ray_trace_result_texture = builder.add_srv(ray_trace_result_texture_node);

      parameter.output_color_variance_texture =
        builder.add_uav(temporal_accumulation_output_texture_node);
      parameter.output_moment_length_texture = builder.add_uav(moment_length_ouptut_texture_node);

      parameter.prev_color_variance_texture = builder.add_srv(atrous_feedback_texture_node);
      parameter.prev_moment_length_texture  = builder.add_srv(moment_length_history_texture_node);

      parameter.filter_dispatch_arg_buffer =
        builder.add_write_ssbo(filter_dispatch_arg_buffer_node);
      parameter.copy_dispatch_arg_buffer = builder.add_write_ssbo(copy_dispatch_arg_buffer_node);
      parameter.filter_coords_buffer     = builder.add_write_ssbo(filter_coords_buffer_node);
      parameter.copy_coords_buffer       = builder.add_write_ssbo(copy_coords_buffer_node);
    };

    const auto temporal_accumulation_execute_fn =
      [this, temporal_dispatch_count, &scene](
        const TemporalAcummulationParameter& parameter, auto& registry, auto& command_list)
    {
      const gpu::ComputePipelineStateDesc desc = {.program_id = temporal_accumulation_program_id_};

      TemporalAccumulationPC push_constant = {
        .gpu_scene_id =
          gpu_system_->get_ssbo_descriptor_id(registry.get_buffer(parameter.scene_buffer)),

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

        .ray_trace_result_texture =
          registry.get_srv_descriptor_id(parameter.ray_trace_result_texture),

        .output_color_variance_texture =
          registry.get_uav_descriptor_id(parameter.output_color_variance_texture),
        .output_moments_texture =
          registry.get_uav_descriptor_id(parameter.output_moment_length_texture),

        .prev_color_variance_texture =
          registry.get_srv_descriptor_id(parameter.prev_color_variance_texture),
        .prev_moments_texture =
          registry.get_srv_descriptor_id(parameter.prev_moment_length_texture),

        .filter_dispatch_arg_buffer =
          registry.get_ssbo_descriptor_id(parameter.filter_dispatch_arg_buffer),
        .copy_dispatch_arg_buffer =
          registry.get_ssbo_descriptor_id(parameter.copy_dispatch_arg_buffer),
        .filter_coords_buffer = registry.get_ssbo_descriptor_id(parameter.filter_coords_buffer),
        .copy_coords_buffer   = registry.get_ssbo_descriptor_id(parameter.copy_coords_buffer),

        .alpha         = alpha,
        .moments_alpha = moments_alpha,
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
      render_graph->add_compute_pass<TemporalAcummulationParameter>(
        "Temporal Acummulation", temporal_accumulation_setup_fn, temporal_accumulation_execute_fn);
    temporal_accumulation_output_texture_node =
      temporal_accumulation_pass.get_parameter().output_color_variance_texture;
    filter_dispatch_arg_buffer_node =
      temporal_accumulation_pass.get_parameter().filter_dispatch_arg_buffer;
    copy_dispatch_arg_buffer_node =
      temporal_accumulation_pass.get_parameter().copy_dispatch_arg_buffer;
    filter_coords_buffer_node = temporal_accumulation_pass.get_parameter().filter_coords_buffer;
    copy_coords_buffer_node   = temporal_accumulation_pass.get_parameter().copy_coords_buffer;

    // Atrous Filter
    Array<gpu::TextureNodeID, 2> atrous_ping_pong_texture_nodes = {
      render_graph->create_texture(
        "Atrous Ping Pong Texture 0",
        gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, viewport)),
      render_graph->create_texture(
        "Atrous Ping Pong Texture 1",
        gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, viewport)),
    };

    struct FilterParameter
    {
      gpu::TextureNodeID output_texture;
      gpu::TextureNodeID input_texture;

      gpu::TextureNodeID gbuffer_normal_roughness;
      gpu::TextureNodeID gbuffer_depth;

      gpu::BufferNodeID filter_dispatch_arg_buffer;
      gpu::BufferNodeID filter_coords_buffer;
      gpu::BufferNodeID copy_dispatch_arg_buffer;
      gpu::BufferNodeID copy_coords_buffer;
    };

    gpu::TextureNodeID atrous_input  = temporal_accumulation_output_texture_node;
    gpu::TextureNodeID atrous_output = atrous_ping_pong_texture_nodes[0];
    for (u32 filter_i = 0; filter_i < filter_iterations; filter_i++)
    {
      atrous_output = filter_i == feedback_iteration ? atrous_feedback_texture_node
                                                     : atrous_ping_pong_texture_nodes[filter_i % 2];

      auto filter_setup_fn = [&](FilterParameter& parameter, auto& builder)
      {
        parameter.input_texture  = builder.add_srv(atrous_input);
        parameter.output_texture = builder.add_uav(atrous_output);

        parameter.gbuffer_normal_roughness =
          builder.add_srv(inputs.textures[GBUFFER_NORMAL_ROUGHNESS_INPUT]);
        parameter.gbuffer_depth = builder.add_srv(inputs.textures[GBUFFER_DEPTH_INPUT]);

        parameter.filter_dispatch_arg_buffer =
          builder.add_indirect_command_buffer(filter_dispatch_arg_buffer_node);
        parameter.copy_dispatch_arg_buffer =
          builder.add_indirect_command_buffer(copy_dispatch_arg_buffer_node);
        parameter.filter_coords_buffer = builder.add_read_ssbo(filter_coords_buffer_node);
        parameter.copy_coords_buffer   = builder.add_read_ssbo(copy_coords_buffer_node);
      };

      auto filter_execute_fn =
        [this, filter_i](const FilterParameter& parameter, auto& registry, auto& command_list)
      {
        CopyTilePC copy_tile_pc = {
          .input_texture      = registry.get_srv_descriptor_id(parameter.input_texture),
          .output_texture     = registry.get_uav_descriptor_id(parameter.output_texture),
          .copy_coords_buffer = registry.get_ssbo_descriptor_id(parameter.copy_coords_buffer),
        };

        using Command = gpu::RenderCommandDispatchIndirect;
        command_list.template push<Command>({
          .pipeline_state_id = registry.get_pipeline_state(
            gpu::ComputePipelineStateDesc{.program_id = copy_tile_program_id_}),
          .push_constant = {cast<byte*>(&copy_tile_pc), sizeof(copy_tile_pc)},
          .buffer        = registry.get_buffer(parameter.copy_dispatch_arg_buffer),
        });

        FilterTilePC filter_tile_pc = {
          .output_texture       = registry.get_uav_descriptor_id(parameter.output_texture),
          .filter_coords_buffer = registry.get_ssbo_descriptor_id(parameter.filter_coords_buffer),
          .color_texture        = registry.get_srv_descriptor_id(parameter.input_texture),
          .gbuffer_normal_roughness =
            registry.get_srv_descriptor_id(parameter.gbuffer_normal_roughness),
          .gbuffer_depth = registry.get_srv_descriptor_id(parameter.gbuffer_depth),
          .radius        = radius,
          .step_size     = (1 << filter_i),
          .phi_color     = phi_color,
          .phi_normal    = phi_normal,
          .sigma_depth   = sigma_depth,
        };

        command_list.template push<Command>({
          .pipeline_state_id = registry.get_pipeline_state(
            gpu::ComputePipelineStateDesc{.program_id = filter_tile_program_id_}),
          .push_constant = {cast<byte*>(&filter_tile_pc), sizeof(filter_tile_pc)},
          .buffer        = registry.get_buffer(parameter.filter_dispatch_arg_buffer),
        });
      };

      const auto& filter_node = render_graph->add_compute_pass<FilterParameter>(
        "Filter pass", filter_setup_fn, filter_execute_fn);

      atrous_input = filter_node.get_parameter().output_texture;
      if (filter_i != feedback_iteration)
      {
        atrous_ping_pong_texture_nodes[filter_i % 2] = atrous_input;
      }
    }

    RenderData outputs;
    outputs.textures.insert(String::From(OUTPUT), atrous_input);
    outputs.textures.insert(
      String::From(RAY_TRACE_OUTPUT), ray_trace_node.get_parameter().output_texture);
    outputs.textures.insert(
      String::From(TEMPORAL_ACCUMULATION_OUTPUT),
      temporal_accumulation_pass.get_parameter().output_color_variance_texture);
    outputs.textures.insert(
      String::From(TEMPORAL_ACCUMULATION_MOMENT_OUTPUT),
      temporal_accumulation_pass.get_parameter().output_moment_length_texture);
    return outputs;
  }

  void RtReflectionNode::on_gui_render(NotNull<app::Gui*> gui)
  {
    gui->input_f32("Alpha"_str, &alpha);
    gui->input_f32("Alpha Moments"_str, &moments_alpha);
    gui->input_f32("Phi Color"_str, &phi_color);
    gui->input_f32("Phi Normal"_str, &phi_normal);
    gui->input_f32("Sigma Depth"_str, &sigma_depth);
    gui->input_i32("Filter Radius"_str, &radius);
    gui->slider_i32("Filter Iterations"_str, &filter_iterations, 1, 5);
    gui->input_f32("Trace Normal Bias"_str, &trace_normal_bias);
    gui->input_f32("Lobe Trim"_str, &lobe_trim);
  }

  auto RtReflectionNode::get_gui_label() const -> CompStr
  {
    return "Ray Trace Reflection Node"_str;
  }

  RtReflectionNode::~RtReflectionNode()
  {
    gpu_system_->destroy_program(ray_trace_program_id_);
    gpu_system_->destroy_program(init_dispatch_args_program_id_);
    gpu_system_->destroy_program(temporal_accumulation_program_id_);
    gpu_system_->destroy_program(filter_tile_program_id_);
    gpu_system_->destroy_program(copy_tile_program_id_);
  }

  void RtReflectionNode::setup_images(vec2u32 viewport)
  {
    if (all(viewport_ == viewport))
    {
      return;
    }

    viewport_ = viewport;
    for (gpu::TextureID& texture_id : moment_textures_)
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

    gpu_system_->destroy_texture(temporal_accumulation_output_texture_);
    temporal_accumulation_output_texture_ = gpu_system_->create_texture(gpu::TextureDesc::d2(
      "Reprojection Output Texture",
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

    gpu_system_->destroy_texture(atrous_feedback_texture_);
    atrous_feedback_texture_ = gpu_system_->create_texture(gpu::TextureDesc::d2(
      "Filter Output Texture",
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
} // namespace renderlab
