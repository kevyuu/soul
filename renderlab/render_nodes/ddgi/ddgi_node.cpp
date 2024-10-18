#include "ddgi_node.h"
#include "core/panic.h"
#include "gpu/sl_type.h"
#include "gpu/type.h"

#include "app/gui.h"
#include "core/not_null.h"
#include "core/type.h"
#include "gpu/render_graph.h"
#include "misc/string_util.h"

#include "ddgi.shared.hlsl"
#include "math/matrix.h"

#include "render_graph_util.h"
#include "render_nodes/render_constant_name.h"
#include "utils/util.h"

namespace renderlab
{
  DdgiNode::DdgiNode(NotNull<gpu::System*> gpu_system)
      : gpu_system_(gpu_system),
        random_generator_(std::mt19937(random_device_())),
        probe_placement_update_mode_(ProbePlacementUpdateMode::GRID_STEP_AND_SCENE_AABB),
        probe_placement_dirty_(false)
  {
    random_distribution_zo_ = std::uniform_real_distribution<float>(0.0f, 1.0f);
    random_distribution_no_ = std::uniform_real_distribution<float>(-1.0f, 1.0f);

    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("render_nodes/ddgi/ray_trace_main.hlsl"_str)});
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
    };
    shader_table_id_ =
      gpu_system_->create_shader_table("DDGI Ray Trace Shader Table"_str, shader_table_desc);

    probe_update_program_id_ =
      util::create_compute_program(gpu_system_, "render_nodes/ddgi/probe_update_main.hlsl"_str);
    probe_border_update_program_id_ = util::create_compute_program(
      gpu_system_, "render_nodes/ddgi/probe_border_update_main.hlsl"_str);
    sample_irradiance_program_id_ = util::create_compute_program(
      gpu_system_, "render_nodes/ddgi/sample_irradiance_main.hlsl"_str);
    probe_overlay_program_id_ =
      util::create_raster_program(gpu_system_, "render_nodes/ddgi/probe_overlay_main.hlsl"_str);
    ray_overlay_program_id_ =
      util::create_raster_program(gpu_system_, "render_nodes/ddgi/ray_overlay_main.hlsl"_str);

    probe_count_         = vec3i32(24, 14, 20);
    grid_step_           = vec3f32(1.0f);
    grid_start_position_ = -(vec3f32(probe_count_) * grid_step_ / 2.0f);

    const auto probe_map_texture_width  = (PROBE_OCT_SIZE + 2) * probe_count_.x * probe_count_.y;
    const auto probe_map_texture_height = probe_count_.z;

    volume_ = {
      .grid_start_position      = grid_start_position_,
      .grid_step                = grid_step_,
      .probe_counts             = probe_count_,
      .max_depth                = 1.0f * 1.5f,
      .depth_sharpness          = 50,
      .hysteresis               = 0.98,
      .crush_threshold          = 0.2f,
      .sample_normal_bias       = 0.3f,
      .visibility_normal_bias   = 0.01f,
      .bounce_intensity         = 1.2f,
      .energy_preservation      = 0.8f,
      .probe_map_texture_width  = probe_map_texture_width,
      .probe_map_texture_height = probe_map_texture_height,
      .rays_per_probe           = 256,
    };
    frame_counter_ = 0;
    reset_probe_grids();
  }

  auto DdgiNode::submit_pass(
    const Scene& scene,
    const RenderConstant& constant,
    const RenderData& inputs,
    NotNull<gpu::RenderGraph*> render_graph) -> RenderData
  {
    const auto viewport  = scene.get_viewport();
    const auto frame_idx = scene.render_data_cref().num_frames;

    if (probe_placement_dirty_)
    {
      const auto scene_aabb   = scene.render_data_cref().scene_aabb;
      const auto scene_extent = scene_aabb.max - scene_aabb.min;
      switch (probe_placement_update_mode_)
      {
      case ProbePlacementUpdateMode::GRID_STEP_AND_SCENE_AABB:
      {
        probe_count_         = vec3i32(scene_extent / grid_step_) + vec3i32(2);
        grid_start_position_ = scene_aabb.min - grid_step_ * 0.5f;
        break;
      }
      case ProbePlacementUpdateMode::PROBE_COUNT_AND_SCENE_AABB:
      {
        grid_step_           = scene_extent / vec3f32(probe_count_ - vec3i32(2));
        grid_start_position_ = scene_aabb.min - grid_step_ * 0.5f;
        break;
      }
      case ProbePlacementUpdateMode::MANUAL:
      {
        break;
      }
      default:
      {
        SOUL_NOT_IMPLEMENTED();
      }
      }
      volume_.grid_step           = grid_step_;
      volume_.probe_counts        = probe_count_;
      volume_.grid_start_position = grid_start_position_;
      reset_probe_grids();
      probe_placement_dirty_ = false;
    }

    const u32 probe_count =
      volume_.probe_counts.x * volume_.probe_counts.y * volume_.probe_counts.z;

    const vec2u32 rt_dimension = {volume_.rays_per_probe, probe_count};

    const gpu::TextureNodeID ray_radiance_texture = render_graph->create_texture(
      "Ray Tracing Radiance Texture"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, rt_dimension));

    const gpu::TextureNodeID ray_dir_dist_texture = render_graph->create_texture(
      "Ray Tracing Direction Distance Texture"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, rt_dimension));

    const auto history_irradiance_probe_texture = render_graph->import_texture(
      "History Irradiance Probe Texture"_str, history_radiance_probe_texture_);
    const auto history_depth_probe_texture =
      render_graph->import_texture("History Depth Probe Texture"_str, history_depth_probe_texture_);

    vec2u32 probe_dimension = {volume_.probe_map_texture_width, volume_.probe_map_texture_height};
    const auto radiance_probe_texture = render_graph->create_texture(
      "Radiance Probe Texture"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, probe_dimension));
    const auto depth_probe_texture = render_graph->create_texture(
      "Depth Probe Texture"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RG16F, 1, probe_dimension));

    struct RayTracingPassParameter
    {
      gpu::BufferNodeID scene_buffer;

      gpu::TextureNodeID irradiance_output_texture;
      gpu::TextureNodeID direction_depth_output_texture;
      gpu::TextureNodeID history_irradiance_probe_texture;
      gpu::TextureNodeID history_depth_probe_texture;
      gpu::TlasNodeID tlas;
      gpu::BlasGroupNodeID blas_group;
    };

    const auto& ray_trace_node = render_graph->add_ray_tracing_pass<RayTracingPassParameter>(
      "DDGI Ray Tracing Pass"_str,
      [&, ray_radiance_texture, ray_dir_dist_texture](auto& parameter, auto& builder)
      {
        const auto& render_data                  = scene.render_data_cref();
        parameter.scene_buffer                   = scene.build_scene_dependencies(&builder);
        parameter.irradiance_output_texture      = builder.add_uav(ray_radiance_texture);
        parameter.direction_depth_output_texture = builder.add_uav(ray_dir_dist_texture);
        parameter.history_irradiance_probe_texture =
          builder.add_srv(history_irradiance_probe_texture);
        parameter.history_depth_probe_texture = builder.add_srv(history_depth_probe_texture);
        parameter.tlas =
          builder.add_shader_tlas(render_data.tlas_node_id, {gpu::ShaderStage::COMPUTE});
        parameter.blas_group = builder.add_shader_blas_group(
          render_data.blas_group_node_id, {gpu::ShaderStage::COMPUTE});
      },
      [this, &scene, rt_dimension, frame_counter = this->frame_counter_](
        const auto& parameter, auto& registry, auto& command_list)
      {
        const vec3f32 random_axis = vec3f32(
          random_distribution_no_(random_generator_),
          random_distribution_no_(random_generator_),
          random_distribution_no_(random_generator_));

        const f32 random_angle =
          random_distribution_zo_(random_generator_) * (2 * math::f64const::PI);

        const mat4f32 random_rotation =
          math::rotate(mat4f32::Identity(), random_angle, math::normalize(random_axis));

        RayTracingPC push_constant = {
          .rotation     = enable_random_probe_ray_rotation_ ? random_rotation : mat4f32::Identity(),
          .ddgi_volume  = volume_,
          .frame_idx    = frame_counter,
          .gpu_scene_id = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
          .irradiance_output_texture =
            registry.get_uav_descriptor_id(parameter.irradiance_output_texture),
          .direction_depth_output_texture =
            registry.get_uav_descriptor_id(parameter.direction_depth_output_texture),
          .history_irradiance_probe_texture =
            registry.get_srv_descriptor_id(parameter.history_irradiance_probe_texture),
          .history_depth_probe_texture =
            registry.get_srv_descriptor_id(parameter.history_depth_probe_texture),
        };
        using Command = gpu::RenderCommandRayTrace;
        command_list.template push<Command>({
          .shader_table_id    = shader_table_id_,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(RayTracingPC),
          .dimension          = vec3u32(rt_dimension, 1),
        });
      });

    struct ProbeUpdateParameter
    {
      gpu::BlasGroupNodeID blas_group;
      gpu::TlasNodeID tlas;
      gpu::TextureNodeID irradiance_texture;
      gpu::TextureNodeID ray_dir_dist_texture;
      gpu::TextureNodeID irradiance_probe_texture;
      gpu::TextureNodeID depth_probe_texture;
      gpu::TextureNodeID history_irradiance_probe_texture;
      gpu::TextureNodeID history_depth_probe_texture;
    };

    const auto& probe_update_node = render_graph->add_compute_pass<ProbeUpdateParameter>(
      "Probe Update Pass"_str,
      [&](auto& parameter, auto& builder)
      {
        parameter.irradiance_texture =
          builder.add_srv(ray_trace_node.get_parameter().irradiance_output_texture);
        parameter.ray_dir_dist_texture =
          builder.add_srv(ray_trace_node.get_parameter().direction_depth_output_texture);
        parameter.irradiance_probe_texture = builder.add_uav(radiance_probe_texture);
        parameter.depth_probe_texture      = builder.add_uav(depth_probe_texture);
        parameter.history_irradiance_probe_texture =
          builder.add_srv(history_irradiance_probe_texture);
        parameter.history_depth_probe_texture = builder.add_srv(history_depth_probe_texture);
      },
      [this, frame_counter = this->frame_counter_](
        const auto& parameter, auto& registry, auto& command_list)
      {
        ProbeUpdatePC push_constant = {
          .ddgi_volume          = volume_,
          .frame_counter        = frame_counter,
          .ray_radiance_texture = registry.get_srv_descriptor_id(parameter.irradiance_texture),
          .ray_dir_dist_texture = registry.get_srv_descriptor_id(parameter.ray_dir_dist_texture),
          .history_irradiance_probe_texture =
            registry.get_srv_descriptor_id(parameter.history_irradiance_probe_texture),
          .history_depth_probe_texture =
            registry.get_srv_descriptor_id(parameter.history_depth_probe_texture),
          .irradiance_probe_texture =
            registry.get_uav_descriptor_id(parameter.irradiance_probe_texture),
          .depth_probe_texture = registry.get_uav_descriptor_id(parameter.depth_probe_texture),
        };

        const gpu::ComputePipelineStateDesc desc = {.program_id = probe_update_program_id_};
        const auto pipeline_state_id             = registry.get_pipeline_state(desc);
        using Command                            = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(ProbeUpdatePC),
          .group_count =
            {volume_.probe_counts.x * volume_.probe_counts.y, volume_.probe_counts.z, 1},
        });
      });

    struct ProbeBorderUpdateParameter
    {
      gpu::TextureNodeID irradiance_probe_texture;
      gpu::TextureNodeID depth_probe_texture;
    };

    const auto& border_update_node = render_graph->add_compute_pass<ProbeBorderUpdateParameter>(
      "Probe Border Update Pass"_str,
      [&](auto& parameter, auto& builder)
      {
        parameter.irradiance_probe_texture =
          builder.add_uav(probe_update_node.get_parameter().irradiance_probe_texture);
        parameter.depth_probe_texture =
          builder.add_uav(probe_update_node.get_parameter().depth_probe_texture);
      },
      [&](const auto& parameter, auto& registry, auto& command_list)
      {
        ProbeBorderUpdatePC push_constant = {
          .irradiance_probe_texture =
            registry.get_uav_descriptor_id(parameter.irradiance_probe_texture),
          .depth_probe_texture = registry.get_uav_descriptor_id(parameter.depth_probe_texture),
        };

        const gpu::ComputePipelineStateDesc desc = {.program_id = probe_border_update_program_id_};
        const auto pipeline_state_id             = registry.get_pipeline_state(desc);
        using Command                            = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(ProbeBorderUpdatePC),
          .group_count =
            {volume_.probe_counts.x * volume_.probe_counts.y, volume_.probe_counts.z, 1},
        });
      });

    const auto sample_irradiance_texture = render_graph->create_texture(
      "Sample Irradiance Texture"_str,
      gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA16F, 1, viewport));

    struct SampleIrradianceParameter
    {
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID depth_texture;
      gpu::TextureNodeID normal_roughness_texture;
      gpu::TextureNodeID irradiance_probe_texture;
      gpu::TextureNodeID depth_probe_texture;
      gpu::TextureNodeID output_texture;
    };

    const auto& sample_irradiance_node = render_graph->add_compute_pass<SampleIrradianceParameter>(
      "Sample Irradiance Pass"_str,
      [&](auto& parameter, auto& builder)
      {
        parameter.scene_buffer  = scene.build_scene_dependencies(&builder);
        parameter.depth_texture = builder.add_srv(inputs.textures[DEPTH_INPUT]);
        parameter.normal_roughness_texture =
          builder.add_srv(inputs.textures[NORMAL_ROUGHNESS_INPUT]);
        parameter.irradiance_probe_texture =
          builder.add_srv(border_update_node.get_parameter().irradiance_probe_texture);
        parameter.depth_probe_texture =
          builder.add_srv(border_update_node.get_parameter().depth_probe_texture);
        parameter.output_texture = builder.add_uav(sample_irradiance_texture);
      },
      [this, viewport](const auto& parameter, auto& registry, auto& command_list)
      {
        SampleIrradiancePC push_constant = {
          .ddgi_volume   = volume_,
          .gpu_scene_id  = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
          .depth_texture = registry.get_srv_descriptor_id(parameter.depth_texture),
          .normal_roughness_texture =
            registry.get_srv_descriptor_id(parameter.normal_roughness_texture),
          .irradiance_probe_texture =
            registry.get_srv_descriptor_id(parameter.irradiance_probe_texture),
          .depth_probe_texture = registry.get_srv_descriptor_id(parameter.depth_probe_texture),
          .output_texture      = registry.get_uav_descriptor_id(parameter.output_texture),
        };

        const gpu::ComputePipelineStateDesc desc = {.program_id = sample_irradiance_program_id_};
        const auto pipeline_state_id             = registry.get_pipeline_state(desc);
        using Command                            = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(SampleIrradiancePC),
          .group_count        = vec3u32(
            viewport.x / SAMPLE_IRRADIANCE_WORK_GROUP_SIZE_X,
            viewport.y / SAMPLE_IRRADIANCE_WORK_GROUP_SIZE_Y,
            1),
        });
      });

    const auto copy_to_history_params = Array<RenderGraphUtil::CopyTexturePassParameter, 2>{
      .list = {
        {
          .src_node_id = border_update_node.get_parameter().irradiance_probe_texture,
          .dst_node_id = history_irradiance_probe_texture,
          .region_copy = gpu::TextureRegionCopy::Texture2D(probe_dimension),
        },
        {
          .src_node_id = border_update_node.get_parameter().depth_probe_texture,
          .dst_node_id = history_depth_probe_texture,
          .region_copy = gpu::TextureRegionCopy::Texture2D(probe_dimension),
        },
      }};

    RenderGraphUtil::AddBatchCopyTexturePass(
      render_graph, "Save Probe To History Textures Pass"_str, copy_to_history_params.cspan());

    frame_counter_++;

    RenderData outputs;
    outputs.textures.insert(
      String::From(OUTPUT), sample_irradiance_node.get_parameter().output_texture);

    if (!show_overlay_)
    {
      return outputs;
    }

    const auto depth_texture_node = RenderGraphUtil::CreateDuplicateTexture(
      render_graph, *gpu_system_, "Probe Overlay Depth"_str, inputs.textures[DEPTH_INPUT]);

    struct ProbeOverlayParameter
    {
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID irradiance_probe_texture;
      gpu::TextureNodeID depth_probe_texture;
    };

    SOUL_ASSERT(!inputs.overlay_texture.is_null(), "");

    const gpu::RGColorAttachmentDesc color_attachment_desc = {
      .node_id = inputs.overlay_texture,
    };

    const gpu::RGDepthStencilAttachmentDesc depth_stencil_desc = {.node_id = depth_texture_node};

    const auto& probe_overlay_pass = render_graph->add_raster_pass<ProbeOverlayParameter>(
      "Probe Overlay Render Pass"_str,
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc, depth_stencil_desc),
      [&, this](auto& parameter, auto& builder)
      {
        parameter.scene_buffer = scene.build_scene_dependencies(&builder);
        parameter.irradiance_probe_texture =
          builder.add_srv(border_update_node.get_parameter().irradiance_probe_texture);
        parameter.depth_probe_texture =
          builder.add_srv(border_update_node.get_parameter().depth_probe_texture);
      },
      [this, viewport, probe_count, &constant](
        const auto& parameter, auto& registry, auto& command_list)
      {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = probe_overlay_program_id_,
          .input_bindings =
            {
              .list =
                {
                  {.stride = sizeof(vec2f32)},
                },
            },
          .input_attributes =
            {
              .list =
                {
                  {
                    .binding = 0,
                    .offset  = 0,
                    .type    = gpu::VertexElementType::FLOAT2,
                  },
                },
            },
          .viewport =
            {
              .width  = static_cast<float>(viewport.x),
              .height = static_cast<float>(viewport.y),
            },
          .scissor                = {.extent = viewport},
          .color_attachment_count = 1,
          .depth_stencil_attachment =
            {
              .depth_test_enable  = true,
              .depth_write_enable = true,
              .depth_compare_op   = gpu::CompareOp::LESS,
            },
        };
        const auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

        using Command = gpu::RenderCommandDrawIndex;

        auto push_constant = ProbeOverlayPC{
          .ddgi_volume  = volume_,
          .probe_radius = probe_overlay_radius_,
          .gpu_scene_id = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
          .irradiance_probe_texture =
            registry.get_srv_descriptor_id(parameter.irradiance_probe_texture),
          .depth_probe_texture = registry.get_srv_descriptor_id(parameter.depth_probe_texture),
        };

        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = soul::cast<void*>(&push_constant),
          .push_constant_size = sizeof(ProbeOverlayPC),
          .vertex_buffer_ids  = {constant.buffers[RenderConstantName::QUAD_VERTEX_BUFFER]},
          .index_buffer_id    = {constant.buffers[RenderConstantName::QUAD_INDEX_BUFFER]},
          .first_index        = 0,
          .index_count        = 6,
          .instance_count     = probe_count,
          .first_instance     = 0,
        });
      });

    const auto overlay_texture = probe_overlay_pass.get_color_attachment_node_id(0);

    if (!show_ray_overlay_)
    {
      outputs.overlay_texture = overlay_texture;
      return outputs;
    }

    struct RayOverlayParameter
    {
      gpu::BufferNodeID scene_buffer;
      gpu::TextureNodeID irradiance_texture;
      gpu::TextureNodeID ray_dir_dist_texture;
    };

    const auto& ray_overlay_pass = render_graph->add_raster_pass<RayOverlayParameter>(
      "Ray Overlay Render Pass"_str,
      gpu::RGRenderTargetDesc(
        viewport,
        {.node_id = overlay_texture},
        {.node_id = probe_overlay_pass.get_depth_stencil_attachment_node_id()}),
      [&, this](auto& parameter, auto& builder)
      {
        parameter.scene_buffer = scene.build_scene_dependencies(&builder);
        parameter.irradiance_texture =
          builder.add_srv(ray_trace_node.get_parameter().irradiance_output_texture);
        parameter.ray_dir_dist_texture =
          builder.add_srv(ray_trace_node.get_parameter().direction_depth_output_texture);
      },
      [this, viewport, probe_count, &constant](
        const auto& parameter, auto& registry, auto& command_list)
      {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = ray_overlay_program_id_,
          .input_bindings =
            {
              .list =
                {
                  {.stride = sizeof(vec3f32)},
                },
            },
          .input_attributes =
            {
              .list =
                {
                  {
                    .binding = 0,
                    .offset  = 0,
                    .type    = gpu::VertexElementType::FLOAT3,
                  },
                },
            },
          .viewport =
            {
              .width  = static_cast<float>(viewport.x),
              .height = static_cast<float>(viewport.y),
            },
          .scissor                = {.extent = viewport},
          .color_attachment_count = 1,
          .depth_stencil_attachment =
            {
              .depth_test_enable  = true,
              .depth_write_enable = true,
              .depth_compare_op   = gpu::CompareOp::LESS,
            },
        };
        const auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

        using Command = gpu::RenderCommandDrawIndex;

        auto push_constant = RayOverlayPC{
          .ddgi_volume          = volume_,
          .probe_index          = cast<u32>(ray_overlay_probe_index_),
          .gpu_scene_id         = registry.get_ssbo_descriptor_id(parameter.scene_buffer),
          .irradiance_texture   = registry.get_srv_descriptor_id(parameter.irradiance_texture),
          .ray_dir_dist_texture = registry.get_srv_descriptor_id(parameter.ray_dir_dist_texture),
        };

        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = soul::cast<void*>(&push_constant),
          .push_constant_size = sizeof(RayOverlayPC),
          .vertex_buffer_ids  = {constant.buffers[RenderConstantName::UNIT_CUBE_VERTEX_BUFFER]},
          .index_buffer_id    = {constant.buffers[RenderConstantName::UNIT_CUBE_INDEX_BUFFER]},
          .first_index        = 0,
          .index_count        = 36,
          .instance_count     = cast<u32>(volume_.rays_per_probe),
          .first_instance     = 0,
        });
      });

    outputs.textures.insert(
      String::From(OUTPUT), sample_irradiance_node.get_parameter().output_texture);
    outputs.overlay_texture = ray_overlay_pass.get_color_attachment_node_id(0);
    return outputs;
  } // namespace renderlab

  void DdgiNode::on_gui_render(NotNull<app::Gui*> gui)
  {
    gui->checkbox("Enable Random Probe Ray Rotation"_str, &enable_random_probe_ray_rotation_);
    gui->input_f32("Sample Normal Bias"_str, &volume_.sample_normal_bias);
    gui->input_f32("Visibility Normal Bias"_str, &volume_.visibility_normal_bias);
    gui->input_f32("Depth Sharpness"_str, &volume_.depth_sharpness);
    gui->input_f32("Hysteresis"_str, &volume_.hysteresis);
    gui->input_f32("Crush Threshold"_str, &volume_.crush_threshold);
    gui->input_f32("Bounce Intensity"_str, &volume_.bounce_intensity);
    gui->input_f32("Energy Preservation"_str, &volume_.energy_preservation);
    gui->separator_text("Update probe placement"_str);

    static constexpr FlagMap<ProbePlacementUpdateMode, CompStr> PROBE_PLACEMENT_MODE_STR = {
      "Grid Step and Scene AABB"_str,
      "Probe Count and Scene AABB"_str,
      "Manual"_str,
    };

    if (gui->begin_combo(
          "Probe Placement"_str, PROBE_PLACEMENT_MODE_STR[probe_placement_update_mode_]))
    {
      for (const auto option : FlagIter<ProbePlacementUpdateMode>())
      {
        const b8 is_selected = probe_placement_update_mode_ == option;
        if (gui->selectable(PROBE_PLACEMENT_MODE_STR[option], is_selected))
        {
          probe_placement_update_mode_ = option;
        }
        if (is_selected)
        {
          gui->set_item_default_focus();
        }
      }
      gui->end_combo();
    }

    switch (probe_placement_update_mode_)
    {
    case ProbePlacementUpdateMode::GRID_STEP_AND_SCENE_AABB:
    {
      gui->input_vec3f32("Grid Step"_str, &grid_step_);
      break;
    }
    case ProbePlacementUpdateMode::PROBE_COUNT_AND_SCENE_AABB:
    {
      gui->input_vec3i32("Probe Count"_str, &probe_count_);
      break;
    }
    case ProbePlacementUpdateMode::MANUAL:
    {
      gui->input_vec3f32("Grid Step"_str, &grid_step_);
      gui->input_vec3i32("Probe Count"_str, &probe_count_);
      gui->input_vec3f32("Start Position"_str, &grid_start_position_);
      break;
    }
    default:
    {
      SOUL_NOT_IMPLEMENTED();
    }
    }
    if (gui->button("Reset probe grids"_str))
    {
      probe_placement_dirty_ = true;
    }
    gui->separator_text("Probe dimension : "_str);
    gui->text(String::Format("Grid Step : {}", volume_.grid_step).cspan());
    gui->text(String::Format("Probe Counts : {}", volume_.probe_counts).cspan());
    gui->text(String::Format("Grid Start Position : {}", volume_.grid_start_position).cspan());

    gui->separator_text("Overlay Setting : "_str);
    gui->checkbox("Show Overlay"_str, &show_overlay_);
    if (show_overlay_)
    {
      gui->checkbox("Show Probe Rays"_str, &show_ray_overlay_);
      gui->slider_f32("Probe Radius"_str, &probe_overlay_radius_, 0, 1);
      i32 probe_count = volume_.probe_counts.x * volume_.probe_counts.y * volume_.probe_counts.z;
      gui->slider_i32("Probe Index"_str, &ray_overlay_probe_index_, 0, probe_count - 1);
    }
  }

  auto DdgiNode::get_gui_label() const -> CompStr
  {
    return "Ddgi Node"_str;
  }

  DdgiNode::~DdgiNode()
  {
    gpu_system_->destroy_shader_table(shader_table_id_);
    gpu_system_->destroy_program(ray_trace_program_id_);
    gpu_system_->destroy_program(probe_update_program_id_);
    gpu_system_->destroy_program(probe_border_update_program_id_);
    gpu_system_->destroy_program(sample_irradiance_program_id_);
    gpu_system_->destroy_program(probe_overlay_program_id_);
    gpu_system_->destroy_program(ray_overlay_program_id_);

    gpu_system_->destroy_texture(history_radiance_probe_texture_);
    gpu_system_->destroy_texture(history_depth_probe_texture_);
  }

  void DdgiNode::reset_probe_grids()
  {
    SOUL_LOG_INFO("Reset Probe Grids");
    frame_counter_ = 0;

    const auto probe_map_texture_width =
      PROBE_OCT_SIZE_WITH_BORDER * volume_.probe_counts.x * volume_.probe_counts.y + 2;
    const auto probe_map_texture_height = PROBE_OCT_SIZE_WITH_BORDER * volume_.probe_counts.z + 2;

    volume_.probe_map_texture_width  = probe_map_texture_width;
    volume_.probe_map_texture_height = probe_map_texture_height;

    volume_.max_depth = 1.5f * math::max(grid_step_.x, math::max(grid_step_.y, grid_step_.z));

    vec2u32 probe_dimension = {volume_.probe_map_texture_width, volume_.probe_map_texture_height};

    if (!history_radiance_probe_texture_.is_null())
    {
      gpu_system_->destroy_texture(history_radiance_probe_texture_);
    }
    history_radiance_probe_texture_ = gpu_system_->create_texture(
      "History Radiance Probe Texture"_str,
      gpu::TextureDesc::d2(

        gpu::TextureFormat::RGBA16F,
        1,
        {gpu::TextureUsage::STORAGE, gpu::TextureUsage::SAMPLED, gpu::TextureUsage::TRANSFER_DST},
        {gpu::QueueType::GRAPHIC},
        probe_dimension));

    if (!history_depth_probe_texture_.is_null())
    {
      gpu_system_->destroy_texture(history_depth_probe_texture_);
    }
    history_depth_probe_texture_ = gpu_system_->create_texture(
      "History Depth Probe Texture"_str,
      gpu::TextureDesc::d2(
        gpu::TextureFormat::RG16F,
        1,
        {gpu::TextureUsage::STORAGE, gpu::TextureUsage::SAMPLED, gpu::TextureUsage::TRANSFER_DST},
        {gpu::QueueType::GRAPHIC},
        probe_dimension));
  }
} // namespace renderlab
