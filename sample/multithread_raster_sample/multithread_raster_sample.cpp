#include <random>
#include "core/type.h"
#include "gpu/gpu.h"
#include "math/math.h"

#include "shaders/multithread_raster_type.hlsl"

#include <app.h>

using namespace soul;

class MultiThreadRasterSample final : public App
{
  static constexpr usize ROW_COUNT = 80;
  static constexpr usize COL_COUNT = 30;

  struct Vertex {
    vec2f position = {};
  };

  static constexpr Vertex VERTICES[4] = {
    {{-0.5f, -0.5f}}, {{0.5f, -0.5f}}, {{0.5f, 0.5f}}, {{-0.5f, 0.5f}}};

  using Index = ui16;
  static constexpr Index INDICES[] = {0, 1, 2, 2, 3, 0};

  gpu::ProgramID program_id_ = gpu::ProgramID();
  gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
  gpu::BufferID index_buffer_id_ = gpu::BufferID();

  Vector<MultithreadRasterPushConstant> push_constants_;

  static auto fill_push_constant_vector(
    Vector<MultithreadRasterPushConstant>& vector,
    float x_start,
    float y_start,
    float x_end,
    float y_end,
    ui32 row_count,
    ui32 col_count) -> void
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0, 1.0f);

    for (ui32 col_idx = 0; col_idx < col_count; col_idx++) {
      for (ui32 row_idx = 0; row_idx < row_count; row_idx++) {
        const auto x_offset = x_start + ((x_end - x_start) / static_cast<float>(col_count)) *
                                          (static_cast<float>(col_idx) + 0.5f);
        const auto y_offset = y_start + ((y_end - y_start) / static_cast<float>(row_count)) *
                                          (static_cast<float>(row_idx) + 0.5f);

        const auto translate_vec = vec3f(x_offset, y_offset, 0.0f);
        const auto scale_vec = vec3f(2.0f / col_count, 2.0f / row_count, 0.0f);
        const auto color = vec3f(dist(gen), dist(gen), dist(gen));

        vector.emplace_back(MultithreadRasterPushConstant{
          .transform = math::scale(math::translate(mat4f::identity(), translate_vec), scale_vec),
          .color = color,
        });
      }
    }
  }

  static auto get_rotation(const float elapsed_seconds) -> mat4f
  {
    return math::rotate(
      mat4f::identity(), math::radians(elapsed_seconds * 10.0f), vec3f(0.0f, 0.0f, 1.0f));
  }

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    const gpu::ColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear = true,
    };

    const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

    struct RenderPassParameter {
    };
    const auto& raster_node = render_graph.add_raster_pass<RenderPassParameter>(
      "Render Pass",
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [](auto& parameter, auto& builder) {

      },
      [viewport, this](const auto& /* parameter */, auto& registry, auto& command_list) {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = program_id_,
          .input_bindings = {{.stride = sizeof(Vertex)}},
          .input_attributes =
            {
              {.binding = 0,
               .offset = offsetof(Vertex, position),
               .type = gpu::VertexElementType::FLOAT2},
            },
          .viewport =
            {.width = static_cast<float>(viewport.x), .height = static_cast<float>(viewport.y)},
          .scissor = {.extent = viewport},
          .color_attachment_count = 1,
        };

        using Command = gpu::RenderCommandDrawIndex;

        auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

        command_list.template push<Command>(
          push_constants_.size(), [=, this](const usize index) -> Command {
            return {
              .pipeline_state_id = pipeline_state_id,
              .push_constant_data = &push_constants_[index],
              .push_constant_size = sizeof(MultithreadRasterPushConstant),
              .vertex_buffer_ids = {vertex_buffer_id_},
              .index_buffer_id = index_buffer_id_,
              .first_index = 0,
              .index_count = std::size(INDICES),
            };
          });
      });

    return raster_node.get_color_attachment_node_id();
  }

public:
  explicit MultiThreadRasterSample(const AppConfig& app_config) : App(app_config)
  {
    gpu::ShaderSource shader_source = gpu::ShaderFile("multithread_raster_sample.hlsl");
    std::filesystem::path search_path = "shaders/";
    constexpr auto entry_points = std::to_array<gpu::ShaderEntryPoint>(
      {{gpu::ShaderStage::VERTEX, "vs_main"}, {gpu::ShaderStage::FRAGMENT, "ps_main"}});
    const gpu::ProgramDesc program_desc = {
      .search_path_count = 1,
      .search_paths = &search_path,
      .source_count = 1,
      .sources = &shader_source,
      .entry_point_count = entry_points.size(),
      .entry_points = entry_points.data(),
    };
    auto result = gpu_system_->create_program(program_desc);
    if (!result) {
      SOUL_PANIC("Fail to create program");
    }
    program_id_ = result.value();

    vertex_buffer_id_ = gpu_system_->create_buffer(
      {
        .size = sizeof(Vertex) * std::size(VERTICES),
        .usage_flags = {gpu::BufferUsage::VERTEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Vertex buffer",
      },
      VERTICES);
    gpu_system_->flush_buffer(vertex_buffer_id_);

    index_buffer_id_ = gpu_system_->create_buffer(
      {
        .size = sizeof(Index) * std::size(INDICES),
        .usage_flags = {gpu::BufferUsage::INDEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Index buffer",
      },
      INDICES);
    gpu_system_->flush_buffer(index_buffer_id_);

    fill_push_constant_vector(push_constants_, -1.0f, -1.0f, 1.0f, 1.0f, ROW_COUNT, COL_COUNT);
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  MultiThreadRasterSample app({});
  app.run();

  return 0;
}
