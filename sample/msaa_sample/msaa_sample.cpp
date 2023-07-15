#include <app.h>
#include <imgui/imgui.h>

#include <random>

#include "core/type.h"
#include "gpu/gpu.h"
#include "math/math.h"
#include "shaders/msaa_type.hlsl"
#include "texture_2d_pass.h"

using namespace soul;

class MSAASample final : public App
{
  Texture2DRGPass texture_2d_pass_;

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

  Vector<MSAAPushConstant> push_constants_;

  gpu::TextureSampleCount msaa_sample_count_ = gpu::TextureSampleCount::COUNT_4;

  static constexpr auto TEXTURE_SAMPLE_COUNT_NAME =
    FlagMap<gpu::TextureSampleCount, const char*>::from_val_list(
      {"1", "2", "4", "8", "16", "32", "64"});

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    if (ImGui::Begin("Options")) {
      if (ImGui::BeginCombo("Sample Count", TEXTURE_SAMPLE_COUNT_NAME[msaa_sample_count_])) {
        const auto supported_sample_count_flags = gpu_properties_.limit.color_sample_count_flags &
                                                  gpu_properties_.limit.depth_sample_count_flags;
        supported_sample_count_flags.for_each([this](gpu::TextureSampleCount count) {
          const auto is_selected = (msaa_sample_count_ == count);
          if (ImGui::Selectable(TEXTURE_SAMPLE_COUNT_NAME[count], is_selected)) {
            msaa_sample_count_ = count;
          }

          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        });
        ImGui::EndCombo();
      }
      ImGui::End();
    }

    const auto enable_msaa = msaa_sample_count_ != gpu::TextureSampleCount::COUNT_1;
    const auto viewport = gpu_system_->get_swapchain_extent();

    const auto sample_render_target_desc =
      [this, enable_msaa, viewport, &render_graph]() -> gpu::RGRenderTargetDesc {
      const auto sample_render_target_dim = vec2ui32(viewport.x / 4, viewport.y / 4);
      if (enable_msaa) {
        const gpu::ColorAttachmentDesc color_attachment_desc = {
          .node_id = render_graph.create_texture(
            "MSAA Color Target",
            gpu::RGTextureDesc::create_d2(
              gpu::TextureFormat::RGBA8,
              1,
              sample_render_target_dim,
              true,
              gpu::ClearValue(),
              msaa_sample_count_)),
          .clear = true,
        };

        const gpu::ResolveAttachmentDesc resolve_attachment_desc = {
          .node_id = render_graph.create_texture(
            "MSAA Resolve Target",
            gpu::RGTextureDesc::create_d2(
              gpu::TextureFormat::RGBA8, 1, sample_render_target_dim, true)),
        };

        const gpu::DepthStencilAttachmentDesc depth_attachment_desc = {
          .node_id = render_graph.create_texture(
            "MSAA Depth Target",
            gpu::RGTextureDesc::create_d2(
              gpu::TextureFormat::DEPTH32F,
              1,
              sample_render_target_dim,
              true,
              gpu::ClearValue(),
              msaa_sample_count_)),
          .clear = true,
        };
        return gpu::RGRenderTargetDesc(
          sample_render_target_dim,
          msaa_sample_count_,
          color_attachment_desc,
          resolve_attachment_desc,
          depth_attachment_desc);
      }

      const gpu::ColorAttachmentDesc color_attachment_desc = {
        .node_id = render_graph.create_texture(
          "Color Target",
          gpu::RGTextureDesc::create_d2(
            gpu::TextureFormat::RGBA8, 1, sample_render_target_dim, true, gpu::ClearValue())),
        .clear = true,
      };

      const gpu::DepthStencilAttachmentDesc depth_attachment_desc = {
        .node_id = render_graph.create_texture(
          "Depth Target",
          gpu::RGTextureDesc::create_d2(
            gpu::TextureFormat::DEPTH32F, 1, sample_render_target_dim, true, gpu::ClearValue())),
        .clear = true,
      };

      return gpu::RGRenderTargetDesc(
        sample_render_target_dim, color_attachment_desc, depth_attachment_desc);
    }();

    struct RenderPassParameter {
    };
    const auto& msaa_render_node = render_graph.add_raster_pass<RenderPassParameter>(
      "Render Pass",
      sample_render_target_desc,
      [](auto& parameter, auto& builder) {

      },
      [render_dim = sample_render_target_desc.dimension,
       this](const auto& /* parameter */, auto& registry, auto& command_list) {
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
            },
          .viewport =
            {
              .width = static_cast<float>(render_dim.x),
              .height = static_cast<float>(render_dim.y),
            },
          .scissor = {.extent = render_dim},
          .color_attachment_count = 1,
          .depth_stencil_attachment = {
            true,
            true,
            gpu::CompareOp::GREATER_OR_EQUAL,
          }};
        auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

        using Command = gpu::RenderCommandDrawIndex;
        command_list.template push<Command>(
          push_constants_.size(), [=, this](const usize index) -> Command {
            return {
              .pipeline_state_id = pipeline_state_id,
              .push_constant_data = &push_constants_[index],
              .push_constant_size = sizeof(MSAAPushConstant),
              .vertex_buffer_ids = {vertex_buffer_id_},
              .index_buffer_id = index_buffer_id_,
              .first_index = 0,
              .index_count = std::size(INDICES),
            };
          });
      });

    const auto& sample_render_target = msaa_render_node.get_render_target();
    const auto texture_2d_sample_input = enable_msaa
                                           ? sample_render_target.resolve_attachments[0].out_node_id
                                           : sample_render_target.color_attachments[0].out_node_id;

    const Texture2DRGPass::Parameter texture_2d_parameter = {
      .sampled_texture = texture_2d_sample_input,
      .render_target = render_target,
    };
    return texture_2d_pass_.add_pass(texture_2d_parameter, render_graph);
  }

public:
  explicit MSAASample(const AppConfig& app_config) : App(app_config), texture_2d_pass_(gpu_system_)
  {
    gpu::ShaderSource shader_source = gpu::ShaderFile("msaa_sample.hlsl");
    std::filesystem::path search_path = "shaders/";
    constexpr auto entry_points = std::to_array<gpu::ShaderEntryPoint>({
      {
        gpu::ShaderStage::VERTEX,
        "vs_main",
      },
      {
        gpu::ShaderStage::FRAGMENT,
        "ps_main",
      },
    });
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

    const auto scale_vec = vec3f(0.5f, 0.5, 1.0f);
    const auto rotate_angle = math::radians(45.0f);
    const auto rotate_axis = vec3f(0.0f, 0.0f, 1.0f);
    push_constants_.push_back(MSAAPushConstant{
      .transform = math::scale(
        math::rotate(
          math::translate(mat4f::identity(), vec3f(-0.25f, 0.0f, 0.1f)), rotate_angle, rotate_axis),
        scale_vec),
      .color = vec3f(1.0f, 0.0f, 0.0f),
    });
    push_constants_.push_back(MSAAPushConstant{
      .transform = math::scale(
        math::rotate(
          math::translate(mat4f::identity(), vec3f(0.25f, 0.0f, 0.0f)), rotate_angle, rotate_axis),
        scale_vec),
      .color = vec3f(0.0f, 1.0f, 0.0f),
    });
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  MSAASample app({.enable_imgui = true});
  app.run();

  return 0;
}
