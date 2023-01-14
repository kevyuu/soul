#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"
#include "gpu/sl_type.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>
#include <texture_2d_pass.h>
#include <imgui/imgui.h>

#include "shaders/gpu_address_sample_type.hlsl"

using namespace soul;

class ComputeShaderSampleApp final : public App
{
	Texture2DRGPass texture_2d_pass;
	gpu::ProgramID program_id_ = gpu::ProgramID();

	const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

	GPUScene gpu_scene_ = {
		.sky_color = soulsl::float3(1.0f, 1.0f, 1.0f),
		.cube_color = soulsl::float3(0.0f, 1.0f, 0.0f)
	};

	void render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph) override
	{
		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

		if (ImGui::Begin("Options"))
		{
			ImGui::ColorEdit3("Sky color", reinterpret_cast<float*>(&gpu_scene_.sky_color));
			ImGui::ColorEdit3("Cube color", reinterpret_cast<float*>(&gpu_scene_.cube_color));
			ImGui::End();
		}

		const auto projection = math::perspective(math::radians(45.0f), math::fdiv(viewport.x, viewport.y), 0.1f, 10000.0f);
		const auto projection_inverse = math::inverse(projection);
		const auto view_inverse = math::inverse(camera_man_.get_view_matrix());

		gpu_scene_.projection_inverse = projection_inverse;
		gpu_scene_.view_inverse = view_inverse;
		const auto scene_buffer = render_graph.create_buffer("Scene Buffer", { .size = sizeof(GPUScene) });

		struct GPUSceneUploadPassParameter
		{
			gpu::BufferNodeID buffer;
		};
		const auto scene_upload_parameter = render_graph.add_transfer_pass<GPUSceneUploadPassParameter>("GPUScene upload",
			[scene_buffer](auto& builder, auto& parameter)
			{
				parameter.buffer = builder.add_dst_buffer(scene_buffer, gpu::TransferDataSource::CPU);
			},
			[this, viewport](const auto& parameter, auto& registry, auto& command_list)
			{
				using Command = gpu::RenderCommandUpdateBuffer;
				const gpu::BufferRegionCopy region_copy = {
					.dst_offset = 0,
					.size = sizeof(GPUScene)
				};
				const Command command = {
					.dst_buffer = registry.get_buffer(parameter.buffer),
					.data = soul::cast<void*>(&gpu_scene_),
					.region_count = 1,
					.regions = &region_copy
				};
				command_list.push(command);
			}).get_parameter();

		const gpu::TextureNodeID target_texture = render_graph.create_texture("Target Texture",
			gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, viewport,
				true, gpu::ClearValue(vec4f{ 0.0f, 0.0f, 0.0f, 1.0f }, 0.0f, 0.0f)));

		struct ComputePassParameter
		{
			gpu::TextureNodeID target_texture;
			gpu::BufferNodeID scene_buffer;
		};
		const auto compute_pass_parameter = render_graph.add_compute_pass<ComputePassParameter>("Compute Pass",
			[target_texture, scene_upload_parameter](auto& builder, auto& parameter)
			{
				parameter.target_texture = builder.add_shader_texture(target_texture, { gpu::ShaderStage::COMPUTE }, gpu::ShaderTextureWriteUsage::STORAGE);
				parameter.scene_buffer = builder.add_shader_buffer(scene_upload_parameter.buffer, { gpu::ShaderStage::COMPUTE }, gpu::ShaderBufferReadUsage::STORAGE);
			},
			[this, viewport](const auto& parameter, auto& registry, auto& command_list)
			{
				const gpu::ComputePipelineStateDesc desc = {
					.program_id = program_id_
				};

				const auto end = std::chrono::steady_clock::now();
				const std::chrono::duration<double> elapsed_seconds = end - start_;
				const auto elapsed_seconds_float = static_cast<float>(elapsed_seconds.count());
				PushConstant push_constant = {
					.dimension = viewport,
					.scene_gpu_address = gpu_system_->get_gpu_address(registry.get_buffer(parameter.scene_buffer)),
					.scene_descriptor_id = gpu_system_->get_ssbo_descriptor_id(registry.get_buffer(parameter.scene_buffer)),
				    .output_texture_descriptor_id = gpu_system_->get_uav_descriptor_id(registry.get_texture(parameter.target_texture)),
				};

				const auto pipeline_state_id = registry.get_pipeline_state(desc);
				using Command = gpu::RenderCommandDispatch;
				command_list.push<Command>({
					.pipeline_state_id = pipeline_state_id,
					.push_constant_data = &push_constant,
					.push_constant_size = sizeof(PushConstant),
					.group_count = { viewport.x / WORK_GROUP_SIZE_X, viewport.y / WORK_GROUP_SIZE_Y, 1 }
				});
			}).get_parameter();

		const Texture2DRGPass::Parameter texture_2d_parameter = {
			.sampled_texture = compute_pass_parameter.target_texture,
			.render_target = render_target
		};
		texture_2d_pass.add_pass(texture_2d_parameter, render_graph);
	}

public:
	explicit ComputeShaderSampleApp(const AppConfig& app_config) : App(app_config), texture_2d_pass(gpu_system_)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("gpu_address_sample.hlsl");
		std::filesystem::path search_path = "shaders/";
		const gpu::ProgramDesc program_desc = {
			.search_paths = &search_path,
			.search_path_count = 1,
			.sources = &shader_source,
			.source_count = 1,
			.entry_point_names = gpu::EntryPoints({
				{gpu::ShaderStage::COMPUTE, "cs_main"},
			})
		};
		auto result = gpu_system_->create_program_dxc(program_desc);
		if (!result)
		{
			SOUL_PANIC("Fail to create program");
		}
		program_id_ = result.value();

		camera_man_.set_camera(vec3f(0, 0, 5.0f), vec3f(0), vec3f(0, 1, 0));
	}
};

int main(int argc, char* argv[])
{
	ComputeShaderSampleApp app({ .enable_imgui = true });
	app.run();
	return 0;
}
