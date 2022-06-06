#include "core/type.h"
#include "gpu/gpu.h"

#include <app.h>

using namespace soul;

class TriangleSampleApp : public App
{
private:
	gpu::ProgramID program_id_;

	void render(gpu::RenderGraph& render_graph) override
	{
		const gpu::TextureID swapchain_texture_id = gpu_system_->get_swapchain_texture();
		const gpu::TextureNodeID render_target = render_graph.import_texture("Color Output", swapchain_texture_id);
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.nodeID = render_target,
			.clear = true
		};

		const Vec2ui32 viewport = gpu_system_->get_swapchain_extent();

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
					.programID = program_id_,
					.viewport = {
						.width = viewport.x,
						.height = viewport.y
					},
					.scissor = {
						.width = viewport.x,
						.height = viewport.y
					},
					.colorAttachmentCount = 1
				};

				using Command = gpu::RenderCommandDraw;
				const Command command = {
					.pipelineStateID = registry.get_pipeline_state(pipeline_desc),
					.vertexCount = 3,
					.instanceCount = 1
				};
				command_list.push(command);
			});
	}

public:
	TriangleSampleApp() : App()
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("triangle.slang");
		std::filesystem::path search_path = "shaders/";
		const gpu::ProgramDesc program_desc = {
			.searchPaths = &search_path,
			.searchPathCount = 1,
			.sources = &shader_source,
			.sourceCount = 1,
			.entryPointNames = gpu::EntryPoints({
				{gpu::ShaderStage::VERTEX, "vsMain"},
				{gpu::ShaderStage::FRAGMENT, "fsMain"}
			})
		};
		program_id_ = gpu_system_->create_program(program_desc);
	}
};

int main(int argc, char* argv[])
{
	
	TriangleSampleApp app;
	app.run();

	return 0;
}
