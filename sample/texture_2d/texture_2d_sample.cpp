#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>

using namespace soul;


class Texture2DSampleApp final : public App
{

	static constexpr soul_size ROW_COUNT = 2;
	static constexpr soul_size COL_COUNT = 2;
	static constexpr soul_size TRANSFORM_COUNT = ROW_COUNT * COL_COUNT;

	struct Vertex
	{
		vec2f position = {};
		vec3f color = {};
		vec2f texture_coords = {};
	};

	static constexpr Vertex VERTICES[4] = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 2.0f}}, // top right
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {2.0f, 2.0f}}, // top left
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {2.0f, 0.0f}}, // bottom left
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}} // bottom right
	};

	using Index = uint16;
	static constexpr Index INDICES[] = {
		0, 1, 2, 2, 3, 0
	};

	gpu::ProgramID program_id_ = gpu::ProgramID();
	gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
	gpu::BufferID index_buffer_id_ = gpu::BufferID();
	gpu::TextureID test_texture_id_ = gpu::TextureID();
	gpu::SamplerID test_sampler_id_ = gpu::SamplerID();

	const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

	void render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph) override
	{
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
		};

		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

	    struct RenderPassParameter {};
	    render_graph.add_raster_pass<RenderPassParameter>("Render Pass",
		    gpu::RGRenderTargetDesc(
			    viewport,
			    color_attachment_desc
		    )
		    , [this](auto& parameter, auto& builder)
		    {
		    }, [viewport, this](const auto& parameter, auto& registry, auto& command_list)
		    {

			    const gpu::GraphicPipelineStateDesc pipeline_desc = {
				    .program_id = program_id_,
				    .input_bindings = {
					    {.stride = sizeof(Vertex)}
				    },
				    .input_attributes = {
					    {.binding = 0, .offset = offsetof(Vertex, position), .type = gpu::VertexElementType::FLOAT2},
					    {.binding = 0, .offset = offsetof(Vertex, color), .type = gpu::VertexElementType::FLOAT3},
						{.binding = 0, .offset = offsetof(Vertex, texture_coords), .type = gpu::VertexElementType::FLOAT3}
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
				const auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

			    struct PushConstant
			    {
				    gpu::DescriptorID texture_descriptor_id;
				    gpu::DescriptorID sampler_descriptor_id;
			    };

			    using Command = gpu::RenderCommandDrawIndex;

				const PushConstant push_constant = {
					.texture_descriptor_id = gpu_system_->get_srv_descriptor_id(test_texture_id_),
					.sampler_descriptor_id = gpu_system_->get_sampler_descriptor_id(test_sampler_id_)
				};

				command_list.push<Command>({
				    .pipeline_state_id = pipeline_state_id,
					.push_constant_data = soul::cast<void*>(&push_constant),
					.push_constant_size = sizeof(PushConstant),
					.vertex_buffer_ids = {
						vertex_buffer_id_
					},
					.index_buffer_id = index_buffer_id_,
					.first_index = 0,
					.index_count = std::size(INDICES)
				});

		    });
	}

public:
	explicit Texture2DSampleApp(const AppConfig& app_config) : App(app_config)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("texture_2d_sample.hlsl");
		std::filesystem::path search_path = "shaders/";
		constexpr auto entry_points = std::to_array<gpu::ShaderEntryPoint>({
			{gpu::ShaderStage::VERTEX, "vsMain"},
			{gpu::ShaderStage::FRAGMENT, "psMain"}
		});
		const gpu::ProgramDesc program_desc = {
			.search_path_count = 1,
			.search_paths = &search_path,
			.source_count = 1,
			.sources = &shader_source,
			.entry_point_count = entry_points.size(),
			.entry_points = entry_points.data()
		};
		auto result = gpu_system_->create_program(program_desc);
		if (!result)
		{
			SOUL_PANIC("Fail to create program");
		}
		program_id_ = result.value();

		vertex_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(Vertex) * std::size(VERTICES),
			.usage_flags = { gpu::BufferUsage::VERTEX },
			.queue_flags = { gpu::QueueType::GRAPHIC },
			.name = "Vertex buffer"
		}, VERTICES);
		gpu_system_->flush_buffer(vertex_buffer_id_);

		index_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(Index) * std::size(INDICES),
			.usage_flags = { gpu::BufferUsage::INDEX },
			.queue_flags = { gpu::QueueType::GRAPHIC },
			.name = "Index buffer"
		}, INDICES);
		gpu_system_->flush_buffer(index_buffer_id_);

		{
			int width, height, channel_count;
			void* data = stbi_load("assets/awesomeface.png", &width, &height, &channel_count, 0);
			SOUL_ASSERT(0, data != nullptr, "");

			const gpu::TextureRegionUpdate region_load = {
				.subresource = {
					.layer_count = 1
				},
				.extent = { static_cast<uint32>(width), static_cast<uint32>(height), 1}
			};

			const gpu::TextureLoadDesc load_desc = {
				.data = data,
				.data_size = soul::cast<soul_size>(width * height * channel_count),
				.region_count = 1,
				.regions = &region_load
			};

			test_texture_id_ = gpu_system_->create_texture(gpu::TextureDesc::d2("Test texture", gpu::TextureFormat::RGBA8, 1,
				{ gpu::TextureUsage::SAMPLED }, { gpu::QueueType::GRAPHIC }, { static_cast<uint32>(width), static_cast<uint32>(height) }), load_desc);
			gpu_system_->flush_texture(test_texture_id_, { gpu::TextureUsage::SAMPLED });
		    test_sampler_id_ = gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT));

		}
		
	}
};

int main(int argc, char* argv[])
{
	stbi_set_flip_vertically_on_load(true);
	const ScreenDimension screen_dimension = { .width = 800, .height = 600 };
	Texture2DSampleApp app({ .screen_dimension = screen_dimension });
	app.run();

	return 0;
}
