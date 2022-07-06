
#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"

#include <app.h>

#include "shaders/transform.hlsl"

using namespace soul;

class StorageBufferSampleApp final : public App
{

	static constexpr soul_size ROW_COUNT = 4;
	static constexpr soul_size COL_COUNT = 5;
	static constexpr soul_size TRANSFORM_COUNT = ROW_COUNT * COL_COUNT;

    struct Vertex
	{
		vec2f position = {};
		vec3f color = {};
	};

	static constexpr Vertex VERTICES[4] = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	using Index = uint16;
	static constexpr Index INDICES[] = {
		0, 1, 2, 2, 3, 0
	};

    gpu::ProgramID program_id_ = gpu::ProgramID();
	gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
	gpu::BufferID index_buffer_id_ = gpu::BufferID();
	gpu::BufferID transform_buffer_id_ = gpu::BufferID();
	
	void render(gpu::RenderGraph& render_graph) override
	{
		const gpu::TextureID swapchain_texture_id = gpu_system_->get_swapchain_texture();
		const gpu::TextureNodeID render_target = render_graph.import_texture("Color Output", swapchain_texture_id);
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
		};

		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

		struct PassParameter {};
		render_graph.add_graphic_pass<PassParameter>("Storage Buffer Test",
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

				struct PushConstant
				{
					gpu::DescriptorID transform_descriptor_id = gpu::DescriptorID::null();
					uint32 offset = 0;
				};
				
				using Command = gpu::RenderCommandDrawIndex;
				
				auto transform_descriptor_id = gpu_system_->get_ssbo_descriptor_id(transform_buffer_id_);
				auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);
				soul::Vector<PushConstant> push_constants(TRANSFORM_COUNT);
				command_list.push<Command>(TRANSFORM_COUNT, [=, this, &push_constants](const soul_size index) -> Command
				{
					push_constants[index] = {
						.transform_descriptor_id = transform_descriptor_id,
						.offset = soul::cast<uint32>(index * sizeof(Transform))
					};
					return {
						.pipeline_state_id = pipeline_state_id,
					    .push_constant_data = &push_constants[index],
					    .push_constant_size = sizeof(PushConstant),
					    .vertex_buffer_ids = {
						    this->vertex_buffer_id_
					    },
					    .index_buffer_id = index_buffer_id_,
					    .first_index = 0,
					    .index_count = std::size(INDICES)
					};
				});
			});
	}

public:
	explicit StorageBufferSampleApp(const AppConfig& app_config) : App(app_config)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("storage_buffer_sample.hlsl");
		std::filesystem::path search_path = "shaders/";
		const gpu::ProgramDesc program_desc = {
			.search_paths = &search_path,
			.search_path_count = 1,
			.sources = &shader_source,
			.source_count = 1,
			.entry_point_names = gpu::EntryPoints({
				{gpu::ShaderStage::VERTEX, "vsMain"},
				{gpu::ShaderStage::FRAGMENT, "psMain"}
			})
		};
		auto result = gpu_system_->create_program_dxc(program_desc);
		if (!result)
		{
			SOUL_PANIC("Fail to create program");
		}
		program_id_ = result.value();

		vertex_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(Vertex) * std::size(VERTICES),
			.usage_flags = { gpu::BufferUsage::VERTEX },
			.queue_flags = { gpu::QueueType::GRAPHIC }
			}, VERTICES);
		gpu_system_->flush_buffer(vertex_buffer_id_);

		index_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(Index) * std::size(INDICES),
			.usage_flags = { gpu::BufferUsage::INDEX },
			.queue_flags = { gpu::QueueType::GRAPHIC }
			}, INDICES);
		gpu_system_->flush_buffer(index_buffer_id_);

		soul::Vector<Transform> transforms(TRANSFORM_COUNT);
		for (soul_size transform_idx = 0; transform_idx < transforms.size(); transform_idx++)
		{
			const auto col_idx = transform_idx % COL_COUNT;
			const auto row_idx = transform_idx / COL_COUNT;
			const auto x_offset = -1.0f + (2.0f / COL_COUNT) * (static_cast<float>(col_idx) + 0.5f);
			const auto y_offset = -1.0f + (2.0f / ROW_COUNT) * (static_cast<float>(row_idx) + 0.5f);
			transforms[transform_idx] = {
				.color = {1.0f, 0.0f, 0.0f},
			    .scale = math::scale(mat4f::identity(), vec3f(0.25f, 0.25, 1.0f)),
			    .translation = math::translate(mat4f::identity(), vec3f(x_offset, y_offset, 0.0f)),
			    .rotation = math::rotate(mat4f::identity(), math::radians(45.0f), vec3f(0.0f, 0.0f, 1.0f))
			};
		}

		transform_buffer_id_ = gpu_system_->create_buffer({
			.size = TRANSFORM_COUNT * sizeof(Transform),
			.usage_flags = {gpu::BufferUsage::STORAGE},
			.queue_flags = {gpu::QueueType::GRAPHIC}
		}, transforms.data());
		gpu_system_->flush_buffer(transform_buffer_id_);

	}
};

int main(int argc, char* argv[])
{
	const ScreenDimension screen_dimension = { .width = 800, .height = 600 };
	StorageBufferSampleApp app({ .screen_dimension = screen_dimension });
	app.run();

	return 0;
}
