#include "core/type.h"
#include "gpu/type.h"
#include "math/math.h"

#include <app.h>

#include "gpu/gpu.h"
#include "gpu/id.h"
#include "gpu/render_graph.h"

using namespace soul;

class IndexBufferSampleApp final : public App
{
  gpu::ProgramID program_id_;

  struct Vertex {
    vec2f32 position;
    vec3f32 color;
  };

  static constexpr Vertex VERTICES[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

  using Index = u16;
  static constexpr Index INDICES[] = {0, 1, 2, 2, 3, 0};
  static_assert(std::same_as<Index, u16> || std::same_as<Index, u32>);
  static constexpr gpu::IndexType INDEX_TYPE =
    std::same_as<Index, u16> ? gpu::IndexType::UINT16 : gpu::IndexType::UINT32;

  gpu::BufferID vertex_buffer_id_;
  gpu::BufferID index_buffer_id_;

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    const gpu::RGColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear = true,
    };

    const vec2u32 viewport = gpu_system_->get_swapchain_extent();

    struct PassParameter {
    };
    const auto& raster_node = render_graph.add_raster_pass<PassParameter>(
      "Triangle Test",
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [](auto& parameter, auto& builder) {

      },
      [viewport, this](const auto& /* parameter */, auto& registry, auto& command_list) {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = program_id_,
          .input_bindings =
            {
              .list =
                {
                  {.stride = sizeof(Vertex)},
                },
            },
          .input_attributes =
            {
              .list =
                {
                  {
                    .binding = 0,
                    .offset = offsetof(Vertex, position),
                    .type = gpu::VertexElementType::FLOAT2,
                  },
                  {
                    .binding = 0,
                    .offset = offsetof(Vertex, color),
                    .type = gpu::VertexElementType::FLOAT3,
                  },
                },
            },
          .viewport =
            {.width = static_cast<float>(viewport.x), .height = static_cast<float>(viewport.y)},
          .scissor = {.extent = viewport},
          .color_attachment_count = 1,
        };

        using Command = gpu::RenderCommandDrawIndex;
        const Command command = {
          .pipeline_state_id = registry.get_pipeline_state(pipeline_desc),
          .vertex_buffer_ids = {vertex_buffer_id_},
          .index_buffer_id = index_buffer_id_,
          .index_type = INDEX_TYPE,
          .first_index = 0,
          .index_count = std::size(INDICES),
        };
        command_list.push(command);
      });

    return raster_node.get_color_attachment_node_id();
  }

public:
  explicit IndexBufferSampleApp(const AppConfig& app_config) : App(app_config)
  {
    const auto shader_source =
      gpu::ShaderSource::From(gpu::ShaderFile{.path = Path::From("index_buffer_sample.hlsl"_str)});
    const auto search_path = Path::From("shaders/"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "ps_main"_str},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system_->create_program(program_desc);
    if (result.is_err()) {
      SOUL_ASSERT(0, "Cannot create shader program");
    }
    program_id_ = result.ok_ref();
    vertex_buffer_id_ = gpu_system_->create_buffer(
      {.size = std::size(VERTICES) * sizeof(Vertex),
       .usage_flags = {gpu::BufferUsage::VERTEX},
       .queue_flags = {gpu::QueueType::GRAPHIC},
       .name = "Vertex buffer"},
      VERTICES);
    gpu_system_->flush_buffer(vertex_buffer_id_);

    index_buffer_id_ = gpu_system_->create_buffer(
      {.size = std::size(INDICES) * sizeof(Index),
       .usage_flags = {gpu::BufferUsage::INDEX},
       .queue_flags = {gpu::QueueType::GRAPHIC},
       .name = "Index buffer"},
      INDICES);
    gpu_system_->flush_buffer(index_buffer_id_);
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  const ScreenDimension screen_dimension = {.width = 800, .height = 600};
  IndexBufferSampleApp app(
    {.screen_dimension = soul::Option<ScreenDimension>::Some(screen_dimension)});
  app.run();

  return 0;
}
