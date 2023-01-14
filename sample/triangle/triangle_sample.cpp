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
		render_graph.add_graphic_pass<PassParameter>("Triangle Test",
			gpu::RGRenderTargetDesc(
				viewport,
				color_attachment_desc
			)
			, [](gpu::RGShaderPassDependencyBuilder& builder, PassParameter& parameter)
			{
				// We leave this empty, because there is no any shader dependency.
				// We will hardcode the triangle vertex on the shader in this example.
			}, [viewport, this](const PassParameter& parameter, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
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
		std::filesystem::path search_path = "shaders/";
		const gpu::ProgramDesc program_desc = {
			.search_paths = &search_path,
			.search_path_count = 1,
			.sources = &shader_source,
			.source_count = 1,
			.entry_point_names = gpu::EntryPoints({
				{gpu::ShaderStage::VERTEX, "vsMain"},
				{gpu::ShaderStage::FRAGMENT, "fsMain"}
			})
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
