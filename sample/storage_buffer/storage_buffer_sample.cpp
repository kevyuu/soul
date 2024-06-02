#include "core/type.h"
#include "gpu/gpu.h"
#include "gpu/type.h"
#include "math/math.h"

#include <app.h>

#include "shaders/transform.hlsl"

using namespace soul;

class StorageBufferSampleApp final : public App
{
  static constexpr usize ROW_COUNT       = 4;
  static constexpr usize COL_COUNT       = 5;
  static constexpr usize TRANSFORM_COUNT = ROW_COUNT * COL_COUNT;

  struct Vertex
  {
    vec2f32 position = {};
    vec3f32 color    = {};
  };

  static constexpr Vertex VERTICES[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

  using Index                      = u16;
  static constexpr Index INDICES[] = {0, 1, 2, 2, 3, 0};

  gpu::ProgramID program_id_         = gpu::ProgramID();
  gpu::BufferID vertex_buffer_id_    = gpu::BufferID();
  gpu::BufferID index_buffer_id_     = gpu::BufferID();
  gpu::BufferID transform_buffer_id_ = gpu::BufferID();

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    const gpu::RGColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear   = true,
    };

    const vec2u32 viewport = gpu_system_->get_swapchain_extent();

    struct PassParameter
    {
    };

    const auto& raster_node = render_graph.add_raster_pass<PassParameter>(
      "Storage Buffer Test"_str,
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [](auto& parameter, auto& builder) {

      },
      [viewport, this](const auto& /* parameter */, auto& registry, auto& command_list)
      {
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
                    .offset  = offsetof(Vertex, position),
                    .type    = gpu::VertexElementType::FLOAT2,
                  },
                  {
                    .binding = 0,
                    .offset  = offsetof(Vertex, color),
                    .type    = gpu::VertexElementType::FLOAT3,
                  },
                },
            },
          .viewport =
            {.width = static_cast<float>(viewport.x), .height = static_cast<float>(viewport.y)},
          .scissor                = {.extent = viewport},
          .color_attachment_count = 1,
        };

        struct PushConstant
        {
          gpu::DescriptorID transform_descriptor_id = gpu::DescriptorID::null();
          u32 offset                                = 0;
        };

        using Command = gpu::RenderCommandDrawIndex;

        auto transform_descriptor_id = gpu_system_->get_ssbo_descriptor_id(transform_buffer_id_);
        auto pipeline_state_id       = registry.get_pipeline_state(pipeline_desc);
        auto push_constants          = Vector<PushConstant>::WithSize(TRANSFORM_COUNT);
        command_list.template push<Command>(
          TRANSFORM_COUNT,
          [=, this, &push_constants](const usize index) -> Command
          {
            push_constants[index] = {
              .transform_descriptor_id = transform_descriptor_id,
              .offset                  = soul::cast<u32>(index * sizeof(Transform)),
            };
            return {
              .pipeline_state_id  = pipeline_state_id,
              .push_constant_data = &push_constants[index],
              .push_constant_size = sizeof(PushConstant),
              .vertex_buffer_ids  = {this->vertex_buffer_id_},
              .index_buffer_id    = index_buffer_id_,
              .first_index        = 0,
              .index_count        = std::size(INDICES),
            };
          });
      });

    return raster_node.get_color_attachment_node_id();
  }

public:
  explicit StorageBufferSampleApp(const AppConfig& app_config) : App(app_config)
  {
    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("storage_buffer_sample.hlsl"_str)});
    const auto search_path      = Path::From("shaders/"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vs_main"_str},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "ps_main"_str},
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
    program_id_ = result.ok_ref();

    vertex_buffer_id_ = gpu_system_->create_buffer(
      "Vertex buffer"_str,
      {
        .size        = sizeof(Vertex) * std::size(VERTICES),
        .usage_flags = {gpu::BufferUsage::VERTEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
      },
      VERTICES);
    gpu_system_->flush_buffer(vertex_buffer_id_);

    index_buffer_id_ = gpu_system_->create_buffer(
      "Index buffer"_str,
      {
        .size        = sizeof(Index) * std::size(INDICES),
        .usage_flags = {gpu::BufferUsage::INDEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
      },
      INDICES);
    gpu_system_->flush_buffer(index_buffer_id_);

    auto transforms = Vector<Transform>::WithSize(TRANSFORM_COUNT);
    for (usize transform_idx = 0; transform_idx < transforms.size(); transform_idx++)
    {
      const auto col_idx        = transform_idx % COL_COUNT;
      const auto row_idx        = transform_idx / COL_COUNT;
      const auto x_offset       = -1.0f + (2.0f / COL_COUNT) * (static_cast<float>(col_idx) + 0.5f);
      const auto y_offset       = -1.0f + (2.0f / ROW_COUNT) * (static_cast<float>(row_idx) + 0.5f);
      transforms[transform_idx] = {
        .color       = {1.0f, 0.0f, 0.0f},
        .scale       = math::scale(mat4f32::Identity(), vec3f32(0.25f, 0.25, 1.0f)),
        .translation = math::translate(mat4f32::Identity(), vec3f32(x_offset, y_offset, 0.0f)),
        .rotation =
          math::rotate(mat4f32::Identity(), math::radians(45.0f), vec3f32(0.0f, 0.0f, 1.0f)),
      };
    }

    transform_buffer_id_ = gpu_system_->create_buffer(
      "Transform buffer"_str,
      {
        .size        = TRANSFORM_COUNT * sizeof(Transform),
        .usage_flags = {gpu::BufferUsage::STORAGE},
        .queue_flags = {gpu::QueueType::GRAPHIC},
      },
      transforms.data());
    gpu_system_->flush_buffer(transform_buffer_id_);
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  const ScreenDimension screen_dimension = {.width = 800, .height = 600};
  StorageBufferSampleApp app(
    {.screen_dimension = soul::Option<ScreenDimension>::Some(screen_dimension)});
  app.run();

  return 0;
}
