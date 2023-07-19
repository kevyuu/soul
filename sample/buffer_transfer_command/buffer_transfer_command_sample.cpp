#include "core/type.h"
#include "gpu/gpu.h"
#include "gpu/type.h"
#include "math/math.h"

#include "shaders/transform.hlsl"

#include <app.h>

#include <array>
#include <ranges>

using namespace soul;

class BufferTransferCommandSample final : public App
{
  static constexpr usize ROW_COUNT = 2;
  static constexpr usize COL_COUNT = 2;
  static constexpr usize TRANSFORM_COUNT = ROW_COUNT * COL_COUNT;

  struct Vertex {
    vec2f position = {};
    vec3f color = {};
  };

  static constexpr Vertex VERTICES[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

  using Index = u16;
  static constexpr Index INDICES[] = {0, 1, 2, 2, 3, 0};

  gpu::ProgramID program_id_ = gpu::ProgramID();
  gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
  gpu::BufferID index_buffer_id_ = gpu::BufferID();
  gpu::BufferID transform_q1_buffer_id_ = gpu::BufferID();
  gpu::BufferID transform_q2_buffer_id_ = gpu::BufferID();

  Vector<Transform> transforms_q1_;
  Vector<Transform> transforms_q2_;
  Vector<Transform> transient_transforms_;

  static auto fill_transform_vector(
    Vector<Transform>& transforms_vector,
    float x_start,
    float y_start,
    float x_end,
    float y_end,
    u32 row_count,
    u32 col_count) -> void
  {
    for (u32 col_idx = 0; col_idx < col_count; col_idx++) {
      for (u32 row_idx = 0; row_idx < row_count; row_idx++) {
        const auto x_offset = x_start + ((x_end - x_start) / static_cast<float>(col_count)) *
                                          (static_cast<float>(col_idx) + 0.5f);
        const auto y_offset = y_start + ((y_end - y_start) / static_cast<float>(row_count)) *
                                          (static_cast<float>(row_idx) + 0.5f);
        Transform transform = {
          .color = {1.0f, 0.0f, 0.0f},
          .scale = math::scale(mat4f::identity(), vec3f(0.25f, 0.25, 1.0f)),
          .translation = math::translate(mat4f::identity(), vec3f(x_offset, y_offset, 0.0f)),
          .rotation =
            math::rotate(mat4f::identity(), math::radians(45.0f), vec3f(0.0f, 0.0f, 1.0f)),
        };
        transforms_vector.push_back(transform);
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
      .node_id = render_target, .clear = true};

    const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

    const auto elapsed_seconds_float = get_elapsed_seconds();

    const auto transform_buffer_q1 =
      render_graph.import_buffer("Transform Buffer Q1", transform_q1_buffer_id_);
    const auto transform_buffer_q2 =
      render_graph.import_buffer("Transform Buffer Q2", transform_q2_buffer_id_);

    const auto transient_transform_buffer = render_graph.create_buffer(
      "Transient Transform Buffer",
      {
        .size = transient_transforms_.size() * sizeof(Transform),
      });

    struct UpdatePassParameter {
      gpu::BufferNodeID transform_buffer_q1;
      gpu::BufferNodeID transform_buffer_q2;
      gpu::BufferNodeID transient_transform_buffer;
    };
    UpdatePassParameter update_pass_parameter =
      render_graph
        .add_non_shader_pass<UpdatePassParameter>(
          "Update Transform Pass",
          gpu::QueueType::TRANSFER,
          [=](auto& parameter, auto& builder) {
            parameter.transform_buffer_q1 =
              builder.add_dst_buffer(transform_buffer_q1, gpu::TransferDataSource::CPU);
            parameter.transform_buffer_q2 =
              builder.add_dst_buffer(transform_buffer_q2, gpu::TransferDataSource::CPU);
            parameter.transient_transform_buffer =
              builder.add_dst_buffer(transient_transform_buffer, gpu::TransferDataSource::CPU);
          },
          [this, elapsed_seconds_float](const auto& parameter, auto& registry, auto& command_list) {
            {
              Transform transform = transforms_q1_.back();
              transform.rotation = get_rotation(elapsed_seconds_float);
              const gpu::BufferRegionCopy region_copy = {
                .dst_offset = (transforms_q1_.size() - 1) * sizeof(Transform),
                .size = sizeof(Transform),
              };

              const gpu::RenderCommandUpdateBuffer command = {
                .dst_buffer = registry.get_buffer(parameter.transform_buffer_q1),
                .data = &transform,
                .regions = u32cspan(&region_copy, 1),
              };
              command_list.push(command);
            }

            {
              Transform transform = transforms_q2_.back();
              transform.rotation = get_rotation(elapsed_seconds_float);
              const gpu::BufferRegionCopy region_copy = {
                .dst_offset = (transforms_q2_.size() - 1) * sizeof(Transform),
                .size = sizeof(Transform),
              };

              const gpu::RenderCommandUpdateBuffer command = {
                .dst_buffer = registry.get_buffer(parameter.transform_buffer_q2),
                .data = &transform,
                .regions = u32cspan(&region_copy, 1),
              };
              command_list.push(command);
            }

            {
              Transform& transform = transient_transforms_.back();
              transform.rotation = get_rotation(elapsed_seconds_float);
              const gpu::BufferRegionCopy region_copy = {
                .size = transient_transforms_.size() * sizeof(Transform)};

              const gpu::RenderCommandUpdateBuffer command = {
                .dst_buffer = registry.get_buffer(parameter.transient_transform_buffer),
                .data = transient_transforms_.data(),
                .regions = u32cspan(&region_copy, 1),
              };
              command_list.push(command);
            }
          })
        .get_parameter();

    struct CopyPassParameter {
      gpu::BufferNodeID transform_buffer_q1;
      gpu::BufferNodeID transform_buffer_q2;
      gpu::BufferNodeID transient_transform_buffer;
      gpu::BufferNodeID copy_dst_transform_buffer;
    };
    const auto copy_transform_buffer = render_graph.create_buffer(
      "Copy Transform Buffer",
      {
        .size = (transforms_q1_.size() + transforms_q2_.size() + transient_transforms_.size()) *
                sizeof(Transform),
      });
    CopyPassParameter copy_pass_parameter =
      render_graph
        .add_non_shader_pass<CopyPassParameter>(
          "Copy Transform Buffer",
          gpu::QueueType::TRANSFER,
          [=](auto& parameter, auto& builder) {
            parameter.transform_buffer_q1 =
              builder.add_src_buffer(update_pass_parameter.transform_buffer_q1);
            parameter.transform_buffer_q2 =
              builder.add_src_buffer(update_pass_parameter.transform_buffer_q2);
            parameter.transient_transform_buffer =
              builder.add_src_buffer(update_pass_parameter.transient_transform_buffer);
            parameter.copy_dst_transform_buffer =
              builder.add_dst_buffer(copy_transform_buffer, gpu::TransferDataSource::GPU);
          },
          [this](const auto& parameter, auto& registry, auto& command_list) {
            using Command = gpu::RenderCommandCopyBuffer;
            const gpu::BufferRegionCopy region_copy_q1 = {
              .size = transforms_q1_.size() * sizeof(Transform),
            };
            command_list.template push<Command>({
              .src_buffer = registry.get_buffer(parameter.transform_buffer_q1),
              .dst_buffer = registry.get_buffer(parameter.copy_dst_transform_buffer),
              .regions = u32cspan(&region_copy_q1, 1),
            });

            const gpu::BufferRegionCopy region_copy_q2 = {
              .dst_offset = transforms_q1_.size() * sizeof(Transform),
              .size = transforms_q2_.size() * sizeof(Transform),
            };
            command_list.template push<Command>({
              .src_buffer = registry.get_buffer(parameter.transform_buffer_q2),
              .dst_buffer = registry.get_buffer(parameter.copy_dst_transform_buffer),
              .regions = u32cspan(&region_copy_q2, 1),
            });

            const gpu::BufferRegionCopy region_copy_transient = {
              .dst_offset = (transforms_q1_.size() + transforms_q2_.size()) * sizeof(Transform),
              .size = transient_transforms_.size() * sizeof(Transform),
            };
            command_list.template push<Command>({
              .src_buffer = registry.get_buffer(parameter.transient_transform_buffer),
              .dst_buffer = registry.get_buffer(parameter.copy_dst_transform_buffer),
              .regions = u32cspan(&region_copy_transient, 1),
            });
          })
        .get_parameter();

    struct RenderPassParameter {
      gpu::BufferNodeID transform_buffer;
    };
    const auto& raster_node = render_graph.add_raster_pass<RenderPassParameter>(
      "Render Pass",
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [copy_pass_parameter](auto& parameter, auto& builder) {
        parameter.transform_buffer = builder.add_shader_buffer(
          copy_pass_parameter.copy_dst_transform_buffer,
          {gpu::ShaderStage::VERTEX},
          gpu::ShaderBufferReadUsage::STORAGE);
      },
      [viewport, this](const auto& parameter, auto& registry, auto& command_list) {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = program_id_,
          .input_bindings = {{.stride = sizeof(Vertex)}},
          .input_attributes =
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
          .viewport =
            {
              .width = static_cast<float>(viewport.x),
              .height = static_cast<float>(viewport.y),
            },
          .scissor = {.extent = viewport},
          .color_attachment_count = 1,
        };

        struct PushConstant {
          gpu::DescriptorID transform_descriptor_id = gpu::DescriptorID::null();
          u32 offset = 0;
        };

        auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);
        const auto transform_buffer = registry.get_buffer(parameter.transform_buffer);
        const auto transform_buffer_descriptor_id =
          gpu_system_->get_ssbo_descriptor_id(transform_buffer);

        const auto push_constants_count =
          transforms_q1_.size() + transforms_q2_.size() + transient_transforms_.size();
        const auto push_constant_indexes = std::views::iota(usize(0), push_constants_count);
        auto push_constants = Vector<PushConstant>::transform(
          push_constant_indexes, [=](usize push_constant_idx) -> PushConstant {
            return {
              .transform_descriptor_id = transform_buffer_descriptor_id,
              .offset = soul::cast<u32>(push_constant_idx * sizeof(Transform)),
            };
          });

        using Command = gpu::RenderCommandDrawIndex;
        command_list.template push<Command>(
          push_constants.size(), [=, this, &push_constants](const usize index) -> Command {
            return {
              .pipeline_state_id = pipeline_state_id,
              .push_constant_data = &push_constants[index],
              .push_constant_size = sizeof(PushConstant),
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
  explicit BufferTransferCommandSample(const AppConfig& app_config) : App(app_config)
  {
    gpu::ShaderSource shader_source = gpu::ShaderFile("buffer_transfer_command_sample.hlsl");
    std::filesystem::path search_path = "shaders/";
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vsMain"},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "psMain"},
    };

    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system_->create_program(program_desc);
    if (result.is_ok()) {
      SOUL_PANIC("Fail to create program");
    }
    program_id_ = result.ok_ref();

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

    fill_transform_vector(transforms_q1_, -1.0f, -1.0f, 0.0f, 0.0f, ROW_COUNT, COL_COUNT);
    transform_q1_buffer_id_ = gpu_system_->create_buffer(
      {
        .size = TRANSFORM_COUNT * sizeof(Transform),
        .usage_flags = {gpu::BufferUsage::STORAGE, gpu::BufferUsage::TRANSFER_SRC},
        .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::TRANSFER},
        .name = "Transform q1 buffer",
      },
      transforms_q1_.data());

    fill_transform_vector(transforms_q2_, 0.0f, -1.0f, 1.0f, 0.0f, ROW_COUNT, COL_COUNT);
    transform_q2_buffer_id_ = gpu_system_->create_buffer(
      {
        .size = TRANSFORM_COUNT * sizeof(Transform),
        .usage_flags = {gpu::BufferUsage::STORAGE, gpu::BufferUsage::TRANSFER_SRC},
        .queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::TRANSFER},
        .memory_option =
          gpu::MemoryOption{
            .required = {gpu::MemoryProperty::HOST_COHERENT},
            .preferred = {gpu::MemoryProperty::DEVICE_LOCAL}},
        .name = "Transform q2 buffer",
      },
      transforms_q2_.data());

    fill_transform_vector(transient_transforms_, -1.0f, 0.0f, 1.0f, 1.0f, ROW_COUNT, COL_COUNT * 2);
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  const ScreenDimension screen_dimension = {.width = 800, .height = 600};
  BufferTransferCommandSample app({.screen_dimension = screen_dimension});
  app.run();

  return 0;
}
