#include "core/type.h"
#include "gpu/gpu.h"

#include <app.h>

using namespace soul;

class TriangleSampleApp final : public App
{
	gpu::ProgramID program_id_;

	void render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph) override
	{
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
		};

		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

		struct PassParameter {};
		render_graph.add_raster_pass<PassParameter>("Triangle Test",
			gpu::RGRenderTargetDesc(
				viewport,
				color_attachment_desc
			)
			, [](auto& parameter, auto& builder) -> void
			{
				// We leave this empty, because there is no any shader dependency.
				// We will hardcode the triangle vertex on the shader in this example.
			}, [viewport, this](const auto& parameter, auto& registry, auto& command_list) -> void
			{
				const gpu::GraphicPipelineStateDesc pipeline_desc = {
					.program_id = program_id_,
					.viewport = {
						.width = static_cast<float>(viewport.x),
						.height = static_cast<float>(viewport.y)
					},
					.scissor = {
						.extent = viewport
					},
					.color_attachment_count = 1
				};

				using Command = gpu::RenderCommandDraw;
				const Command command = {
					.pipeline_state_id = registry.get_pipeline_state(pipeline_desc),
					.vertex_count = 3,
					.instance_count = 1
				};
				command_list.push(command);
			});
	}

public:
	explicit TriangleSampleApp(const AppConfig& app_config) : App(app_config)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("triangle_sample.hlsl");
		const std::filesystem::path search_path = "shaders/";
		const auto entry_points = std::to_array<gpu::ShaderEntryPoint>({
			{gpu::ShaderStage::VERTEX, "vsMain"},
			{gpu::ShaderStage::FRAGMENT, "fsMain"}
		});
		const gpu::ProgramDesc program_desc = {
			.search_path_count = 1,
			.search_paths = &search_path,
			.source_count = 1,
			.sources = &shader_source,
			.entry_point_count = entry_points.size(),
			.entry_points = entry_points.data()
		};
		program_id_ = gpu_system_->create_program(program_desc).value();
	}
};

int main(int argc, char* argv[])
{
	
	TriangleSampleApp app({});
	app.run();

	return 0;
}
