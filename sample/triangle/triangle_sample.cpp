#include "core/panic.h"
#include "core/type.h"
#include "gpu/gpu.h"
#include "gpu/type.h"

#include <app.h>

using namespace soul;

class TriangleSampleApp final : public App
{
  gpu::ProgramID program_id_;

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    const gpu::ColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear = true,
    };

    const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

    struct PassParameter {
    };
    const auto& raster_node = render_graph.add_raster_pass<PassParameter>(
      "Triangle Test",
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [](auto& parameter, auto& builder) -> void {
        // We leave this empty, because there is no any shader dependency.
        // We will hardcode the triangle vertex on the shader in this example.
      },
      [viewport, this](const auto& parameter, auto& registry, auto& command_list) -> void {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = program_id_,
          .viewport =
            {.width = static_cast<float>(viewport.x), .height = static_cast<float>(viewport.y)},
          .scissor = {.extent = viewport},
          .color_attachment_count = 1,
        };

        using Command = gpu::RenderCommandDraw;
        const Command command = {
          .pipeline_state_id = registry.get_pipeline_state(pipeline_desc),
          .vertex_count = 3,
          .instance_count = 1,
        };
        command_list.push(command);
      });

    return raster_node.get_color_attachment_node_id();
  }

public:
  explicit TriangleSampleApp(const AppConfig& app_config) : App(app_config)
  {
    const auto shader_source =
      gpu::ShaderSource::From(gpu::ShaderFile{.path = Path::From("triangle_sample.hlsl"_str)});
    const auto search_path = Path::From("shaders/"_str);
    const auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "fs_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    program_id_ = gpu_system_->create_program(program_desc).ok_ref();
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  TriangleSampleApp app({});
  app.run();

  return 0;
}
