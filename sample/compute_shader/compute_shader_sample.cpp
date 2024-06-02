#include "core/type.h"
#include "gpu/gpu.h"
#include "gpu/sl_type.h"
#include "gpu/type.h"
#include "math/math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>
#include "shaders/compute_type.hlsl"
#include <texture_2d_pass.h>

using namespace soul;

class ComputeShaderSampleApp final : public App
{
  Texture2DRGPass texture_2d_pass;
  gpu::ProgramID program_id_ = gpu::ProgramID();

  const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    const vec2u32 viewport = gpu_system_->get_swapchain_extent();

    const gpu::TextureNodeID target_texture = render_graph.create_texture(
      "Target Texture"_str,
      gpu::RGTextureDesc::create_d2(
        gpu::TextureFormat::RGBA8,
        1,
        viewport,
        true,
        gpu::ClearValue(vec4f32{1.0f, 0.0f, 0.0f, 1.0f}, 0.0f, 0.0f)));

    struct ComputePassParameter
    {
      gpu::TextureNodeID target_texture;
    };

    const auto& compute_node = render_graph.add_compute_pass<ComputePassParameter>(
      "Compute Pass"_str,
      [target_texture](auto& parameter, auto& builder)
      {
        parameter.target_texture = builder.add_shader_texture(
          target_texture, {gpu::ShaderStage::COMPUTE}, gpu::ShaderTextureWriteUsage::STORAGE);
      },
      [this, viewport](const auto& parameter, auto& registry, auto& command_list)
      {
        const gpu::ComputePipelineStateDesc desc = {.program_id = program_id_};

        const auto end                                      = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed_seconds = end - start_;
        const auto elapsed_seconds_float  = static_cast<float>(elapsed_seconds.count());
        ComputePushConstant push_constant = {
          .output_uav_gpu_handle =
            gpu_system_->get_uav_descriptor_id(registry.get_texture(parameter.target_texture)),
          .dimension = viewport,
          .t         = elapsed_seconds_float,
        };

        const auto pipeline_state_id = registry.get_pipeline_state(desc);
        using Command                = gpu::RenderCommandDispatch;
        command_list.template push<Command>({
          .pipeline_state_id  = pipeline_state_id,
          .push_constant_data = &push_constant,
          .push_constant_size = sizeof(ComputePushConstant),
          .group_count        = {viewport.x / WORK_GROUP_SIZE_X, viewport.y / WORK_GROUP_SIZE_Y, 1},
        });
      });

    struct RenderPassParameter
    {
      gpu::TextureNodeID sampled_texture;
    };

    const Texture2DRGPass::Parameter texture_2d_parameter = {
      .sampled_texture = compute_node.get_parameter().target_texture,
      .render_target   = render_target,
    };
    return texture_2d_pass.add_pass(texture_2d_parameter, render_graph);
  }

public:
  explicit ComputeShaderSampleApp(const AppConfig& app_config)
      : App(app_config), texture_2d_pass(gpu_system_)
  {
    const auto shader_source = gpu::ShaderSource::From(
      gpu::ShaderFile{.path = Path::From("compute_shader_sample.hlsl"_str)});
    const auto search_path      = Path::From("shaders/"_str);
    constexpr auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::COMPUTE, "cs_main"_str},
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
  }
};

auto main(int argc, char* argv[]) -> int
{
  ComputeShaderSampleApp app({.enable_imgui = true});
  app.run();
  return 0;
}
