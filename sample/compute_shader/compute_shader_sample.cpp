#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"
#include "gpu/sl_type.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>
#include <texture_2d_pass.h>
#include "shaders/compute_type.hlsl"

using namespace soul;

class ComputeShaderSampleApp final : public App
{
	Texture2DRGPass texture_2d_pass;
	gpu::ProgramID program_id_ = gpu::ProgramID();

	const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

	void render(gpu::RenderGraph& render_graph) override
	{
		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

		const gpu::TextureNodeID target_texture = render_graph.create_texture("Target Texture",
			gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, viewport, 
				true, gpu::ClearValue(vec4f{ 1.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f)));

		struct ComputePassParameter
		{
		    gpu::TextureNodeID target_texture;
		};
		render_graph.add_compute_pass<ComputePassParameter>("Compute Pass",
			[target_texture](gpu::RGShaderPassDependencyBuilder& builder, ComputePassParameter& parameter)
			{
				parameter.target_texture = builder.add_shader_texture(target_texture, { gpu::ShaderStage::COMPUTE }, gpu::ShaderTextureWriteUsage::STORAGE);
			},
			[this, viewport](const ComputePassParameter& parameter, gpu::RenderGraphRegistry& registry, gpu::ComputeCommandList& command_list)
			{
				const gpu::ComputePipelineStateDesc desc = {
					.program_id = program_id_
				};

				const auto end = std::chrono::steady_clock::now();
				std::chrono::duration<double> elapsed_seconds = end - start_;
				const auto elapsed_seconds_float = static_cast<float>(elapsed_seconds.count());
				SOUL_LOG_INFO("Elapsed seconds = %.3f", elapsed_seconds_float);
				ComputePushConstant push_constant = {
					.output_uav_gpu_handle = gpu_system_->get_uav_descriptor_id(registry.get_texture(parameter.target_texture)),
					.dimension = viewport,
					.t = elapsed_seconds_float
				};

				const auto pipeline_state_id = registry.get_pipeline_state(desc);
				using Command = gpu::RenderCommandDispatch;
				command_list.push<Command>({
					.pipeline_state_id = pipeline_state_id,
					.push_constant_data = &push_constant,
					.push_constant_size = sizeof(ComputePushConstant),
					.group_count = {viewport.x / WORK_GROUP_SIZE_X, viewport.y / WORK_GROUP_SIZE_Y, 1}
				});
			});

		struct RenderPassParameter
		{
			gpu::TextureNodeID sampled_texture;
		};

		const Texture2DRGPass::Parameter texture_2d_parameter = {
			.sampled_texture = target_texture
		};
		texture_2d_pass.add_pass(texture_2d_parameter, render_graph);
	}

public:
	explicit ComputeShaderSampleApp(const AppConfig& app_config) : App(app_config), texture_2d_pass(gpu_system_)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("compute_shader_sample.hlsl");
		std::filesystem::path search_path = "shaders/";
		const gpu::ProgramDesc program_desc = {
			.search_paths = &search_path,
			.search_path_count = 1,
			.sources = &shader_source,
			.source_count = 1,
			.entry_point_names = gpu::EntryPoints({
				{gpu::ShaderStage::COMPUTE, "csMain"},
			})
		};
		auto result = gpu_system_->create_program_dxc(program_desc);
		if (!result)
		{
			SOUL_PANIC("Fail to create program");
		}
		program_id_ = result.value();
	}
};

int main(int argc, char* argv[])
{
	ComputeShaderSampleApp app({ .enable_imgui = true });
	app.run();
	return 0;
}
