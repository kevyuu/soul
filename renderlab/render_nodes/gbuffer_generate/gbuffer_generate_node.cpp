#include "gbuffer_generate_node.h"
#include "gbuffer_generate_type.hlsl"

#include "render_graph_util.h"

#include "core/type.h"
#include "gpu/render_graph.h"
#include "gpu/type.h"

namespace renderlab
{
  GBufferGenerateNode::GBufferGenerateNode(NotNull<gpu::System*> gpu_system)
      : gpu_system_(gpu_system)
  {
    const auto shader_source    = gpu::ShaderSource::From(gpu::ShaderFile{
         .path = Path::From("render_nodes/gbuffer_generate/gbuffer_generate_main.hlsl"_str)});
    const auto search_path      = Path::From("shaders"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "ps_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources      = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    program_id_ = gpu_system_->create_program(program_desc).ok_ref();
  }

  auto GBufferGenerateNode::submit_pass(
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
    setup_gbuffers(viewport);

    const auto frame_index = scene.render_data_cref().num_frames;

    const auto current_normal_roughness_gbuffers = normal_roughness_gbuffers_[frame_index % 2];
    const auto current_motion_curve_gbuffers     = motion_curve_gbuffers_[frame_index % 2];
    const auto current_meshid_gbuffers           = meshid_gbuffers_[frame_index % 2];

    const auto prev_normal_roughness_gbuffers = normal_roughness_gbuffers_[(frame_index + 1) % 2];
    const auto prev_motion_curve_gbuffers     = motion_curve_gbuffers_[(frame_index + 1) % 2];
    const auto prev_meshid_gbuffers           = meshid_gbuffers_[(frame_index + 1) % 2];

    const auto prev_normal_roughness_gbuffers_node = render_graph->import_texture(
      "Prev Normal Roughness GBuffers", prev_normal_roughness_gbuffers);
    const auto prev_motion_curve_gbuffers_node =
      render_graph->import_texture("Prev Motion LinearZ GBuffers", prev_motion_curve_gbuffers);
    const auto prev_meshid_gbuffers_node =
      render_graph->import_texture("Prev MeshID GBuffers", prev_meshid_gbuffers);

    const auto current_depth_gbuffer_node = render_graph->create_texture(
      "Depth Texture", gpu::RGTextureDesc::create_d2(gpu::TextureFormat::DEPTH32F, 1, viewport));
    const auto prev_depth_gbuffers_node =
      render_graph->import_texture("Prev Depth GBuffers", prev_depth_texture_);

    const gpu::RGColorAttachmentDesc albedo_metal_texture = {
      .node_id = render_graph->import_texture("Albedo Metal GBuffers", albedo_metal_gbuffer_),
      .clear   = true,
    };
    const gpu::RGColorAttachmentDesc normal_roughness_texture = {
      .node_id =
        render_graph->import_texture("GBuffer_Normal_Roughness", current_normal_roughness_gbuffers),
      .clear = true,
    };
    const gpu::RGColorAttachmentDesc motion_curve_texture = {
      .node_id =
        render_graph->import_texture("GBuffer_motion_curve", current_motion_curve_gbuffers),
      .clear = true,
    };
    const gpu::RGColorAttachmentDesc mesh_id_texture = {
      .node_id = render_graph->import_texture("GBuffer_MeshID", current_meshid_gbuffers),
      .clear   = true,
    };

    auto clear_value                                              = gpu::ClearValue();
    clear_value.depth_stencil.depth                               = 1.0f;
    const gpu::RGDepthStencilAttachmentDesc depth_attachment_desc = {
      .node_id     = current_depth_gbuffer_node,
      .clear       = true,
      .clear_value = clear_value,
    };

    gpu::RGRenderTargetDesc render_target_desc;
    render_target_desc.dimension = viewport;
    render_target_desc.color_attachments.reserve(4);
    render_target_desc.color_attachments.push_back(albedo_metal_texture);
    render_target_desc.color_attachments.push_back(normal_roughness_texture);
    render_target_desc.color_attachments.push_back(motion_curve_texture);
    render_target_desc.color_attachments.push_back(mesh_id_texture);
    render_target_desc.depth_stencil_attachment = depth_attachment_desc;

    const auto& pass = render_graph->add_raster_pass<Parameter>(
      "GBuffer Generate Pass",
      render_target_desc,
      [&scene](auto& parameter, auto& builder)
      {
        parameter.scene_buffer = scene.build_scene_dependencies(&builder);
        scene.build_rasterize_dependencies(&builder);
      },
      [this, &scene, &render_target_desc](const auto& parameter, auto& registry, auto& command_list)
      {
        const GBufferGeneratePushConstant push_constant = {
          .gpu_scene_id =
            gpu_system_->get_ssbo_descriptor_id(registry.get_buffer(parameter.scene_buffer)),
        };
        const auto viewport                    = scene.get_viewport();
        const Scene::RasterizeDesc raster_desc = {
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(GBufferGeneratePushConstant),
          .program_id         = program_id_,
          .viewport =
            {
              .width  = static_cast<float>(viewport.x),
              .height = static_cast<float>(viewport.y),
            },
          .scissor                = {.extent = viewport},
          .color_attachment_count = 4,
          .depth_stencil_attachment =
            {
              .depth_test_enable  = true,
              .depth_write_enable = true,
              .depth_compare_op   = gpu::CompareOp::LESS,
            },
        };
        scene.rasterize(raster_desc, &registry, &command_list);
      });

    const auto store_previous_pass = Array<RenderGraphUtil::CopyTexturePassParameter, 1>{
      .list =
        {
          {
            .src_node_id = pass.get_depth_stencil_attachment_node_id(),
            .dst_node_id = prev_depth_gbuffers_node,
            .region_copy = gpu::TextureRegionCopy::Texture2D(viewport),
          },
        },
    };
    RenderGraphUtil::AddBatchCopyTexturePass(
      render_graph, "GBuffer Store Previous GBuffer Pass"_str, store_previous_pass.cspan());

    RenderData outputs;
    outputs.textures.insert(String(GBUFFER_ALBEDO_METAL), pass.get_color_attachment_node_id(0));
    outputs.textures.insert(String(GBUFFER_NORMAL_ROUGHNESS), pass.get_color_attachment_node_id(1));
    outputs.textures.insert(String(GBUFFER_MOTION_CURVE), pass.get_color_attachment_node_id(2));
    outputs.textures.insert(String(GBUFFER_MESHID), pass.get_color_attachment_node_id(3));
    outputs.textures.insert(String(GBUFFER_DEPTH), pass.get_depth_stencil_attachment_node_id());

    outputs.textures.insert(
      String(PREV_GBUFFER_NORMAL_ROUGHNESS), prev_normal_roughness_gbuffers_node);
    outputs.textures.insert(String(PREV_GBUFFER_MOTION_CURVE), prev_motion_curve_gbuffers_node);
    outputs.textures.insert(String(PREV_GBUFFER_MESHID), prev_meshid_gbuffers_node);
    outputs.textures.insert(String(PREV_GBUFFER_DEPTH), prev_depth_gbuffers_node);
    return outputs;
  }

  void GBufferGenerateNode::on_gui_render(NotNull<app::Gui*> gui) {}

  [[nodiscard]]
  auto GBufferGenerateNode::get_gui_label() const -> CompStr
  {
    return "GBuffer Generation"_str;
  }

  GBufferGenerateNode::~GBufferGenerateNode()
  {
    gpu_system_->destroy_program(program_id_);
  }

  void GBufferGenerateNode::setup_gbuffers(vec2u32 viewport)
  {
    if (all(viewport_ == viewport))
    {
      return;
    }

    viewport_ = viewport;

    albedo_metal_gbuffer_ = gpu_system_->create_texture(gpu::TextureDesc::d2(
      "albedo_metal_gbuffers",
      gpu::TextureFormat::RGBA8,
      1,
      {
        gpu::TextureUsage::COLOR_ATTACHMENT,
        gpu::TextureUsage::SAMPLED,
      },
      {
        gpu::QueueType::GRAPHIC,
        gpu::QueueType::COMPUTE,
      },
      viewport));

    for (gpu::TextureID& texture_id : normal_roughness_gbuffers_)
    {
      texture_id = gpu_system_->create_texture(gpu::TextureDesc::d2(
        "normal_rougness_gbuffers",
        gpu::TextureFormat::RGBA8,
        1,
        {
          gpu::TextureUsage::COLOR_ATTACHMENT,
          gpu::TextureUsage::SAMPLED,
        },
        {
          gpu::QueueType::GRAPHIC,
          gpu::QueueType::COMPUTE,
        },
        viewport));
    }

    for (gpu::TextureID& texture_id : motion_curve_gbuffers_)
    {
      texture_id = gpu_system_->create_texture(gpu::TextureDesc::d2(
        "motion_curve_gbuffers",
        gpu::TextureFormat::RGBA16F,
        1,
        {
          gpu::TextureUsage::COLOR_ATTACHMENT,
          gpu::TextureUsage::SAMPLED,
        },
        {
          gpu::QueueType::GRAPHIC,
          gpu::QueueType::COMPUTE,
        },
        viewport));
    }

    for (gpu::TextureID& texture_id : meshid_gbuffers_)
    {
      texture_id = gpu_system_->create_texture(gpu::TextureDesc::d2(
        "meshid_gbuffers",
        gpu::TextureFormat::R32UI,
        1,
        {
          gpu::TextureUsage::COLOR_ATTACHMENT,
          gpu::TextureUsage::SAMPLED,
        },
        {
          gpu::QueueType::GRAPHIC,
          gpu::QueueType::COMPUTE,
        },
        viewport));
    }

    prev_depth_texture_ = gpu_system_->create_texture(gpu::TextureDesc::d2(
      "prev depth gbuffer",
      gpu::TextureFormat::DEPTH32F,
      1,
      {
        gpu::TextureUsage::DEPTH_STENCIL_ATTACHMENT,
        gpu::TextureUsage::SAMPLED,
        gpu::TextureUsage::TRANSFER_DST,
      },
      {
        gpu::QueueType::GRAPHIC,
        gpu::QueueType::COMPUTE,
        gpu::QueueType::TRANSFER,
      },
      viewport));
  }
} // namespace renderlab
