#include "core/type.h"
#include "math/math.h"

#include <app.h>

#include "gpu/gpu.h"
#include "gpu/id.h"
#include "gpu/render_graph.h"

using namespace soul;

class IndexBufferSampleApp final : public App
{
	gpu::ProgramID program_id_;

	struct Vertex
	{
		vec2f position;
		vec3f color;
	};

	static constexpr Vertex VERTICES[4] = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	using Index = uint16;
	static constexpr Index INDICES[]  = {
		0, 1, 2, 2, 3, 0
	};
	static_assert(std::same_as<Index, uint16> || std::same_as<Index, uint32>);
	static constexpr gpu::IndexType INDEX_TYPE = std::same_as<Index, uint16> ? gpu::IndexType::UINT16 : gpu::IndexType::UINT32;

	gpu::BufferID vertex_buffer_id_;
	gpu::BufferID index_buffer_id_;

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
				
			}, [viewport, this](const PassParameter& parameter, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
			{

				const gpu::GraphicPipelineStateDesc pipeline_desc = {
					.program_id = program_id_,
					.input_bindings = {
						{.stride = sizeof(Vertex)}
					},
					.input_attributes = {
						{.binding = 0, .offset = offsetof(Vertex, position), .type = gpu::VertexElementType::FLOAT2},
						{.binding = 0, .offset = offsetof(Vertex, color), .type = gpu::VertexElementType::FLOAT3}
					},
					.viewport = {
						.width = static_cast<float>(viewport.x),
						.height = static_cast<float>(viewport.y)
					},
					.scissor = {
						.extent = viewport
					},
					.color_attachment_count = 1
				};

				using Command = gpu::RenderCommandDrawIndex;
				const Command command = {
					.pipeline_state_id = registry.get_pipeline_state(pipeline_desc),
					.vertex_buffer_ids = {
						vertex_buffer_id_
					},
					.index_buffer_id = index_buffer_id_,
					.index_type = INDEX_TYPE,
					.first_index = 0,
					.index_count = std::size(INDICES)
				};
				command_list.push(command);
			});
	}

public:
	explicit IndexBufferSampleApp(const AppConfig& app_config) : App(app_config)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("index_buffer_sample.hlsl");
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
		auto result = gpu_system_->create_program_dxc(program_desc);
        if (!result)
        {
			SOUL_ASSERT(0, "Cannot create shader program");
        }
		program_id_ = result.value();
		vertex_buffer_id_ = gpu_system_->create_buffer({
			.size = std::size(VERTICES) * sizeof(Vertex),
			.usage_flags = { gpu::BufferUsage::VERTEX },
			.queue_flags = { gpu::QueueType::GRAPHIC },
			.name = "Vertex buffer"
		}, VERTICES);
		gpu_system_->flush_buffer(vertex_buffer_id_);

		index_buffer_id_ = gpu_system_->create_buffer({
			.size = std::size(INDICES) * sizeof(Index),
			.usage_flags = { gpu::BufferUsage::INDEX },
			.queue_flags = { gpu::QueueType::GRAPHIC },
			.name = "Index buffer"
		}, INDICES);
		gpu_system_->flush_buffer(index_buffer_id_);

	}
};

int main(int argc, char* argv[])
{
	const ScreenDimension screen_dimension = { .width = 800, .height = 600 };
	IndexBufferSampleApp app({.screen_dimension = screen_dimension});
	app.run();

	return 0;
}
