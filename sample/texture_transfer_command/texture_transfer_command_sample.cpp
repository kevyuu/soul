#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>

using namespace soul;


class Texture3DSampleApp final : public App
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

	struct Transform
	{
		float dummy = 0.0f;
		vec3f color = {};
		mat4f scale = {};
		mat4f position = {};
		mat4f rotation = {};
	};

	gpu::ProgramID program_id_ = gpu::ProgramID();
	gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
	gpu::BufferID index_buffer_id_ = gpu::BufferID();
	gpu::TextureID test_texture_id_ = gpu::TextureID();
	gpu::SamplerID test_sampler_id_ = gpu::SamplerID();
	void* test_texture_data_;
	uint32 width_, height_, channel_count_;

	const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

	void render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph) override
	{
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
		};

		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

        const auto persistent_texture_node_id = render_graph.import_texture("Persistent Texture", test_texture_id_);

		struct UpdatePassParameter
		{
			gpu::TextureNodeID persistent_texture;
		};
		const auto update_pass_parameter = render_graph.add_transfer_pass<UpdatePassParameter>("Update Texture Pass",
			[=](gpu::RGTransferPassDependencyBuilder& builder, UpdatePassParameter& parameter)
			{
				parameter.persistent_texture = builder.add_dst_texture(persistent_texture_node_id, gpu::TransferDataSource::CPU);
			},
			[this](const UpdatePassParameter& parameter, const gpu::RenderGraphRegistry& registry, gpu::TransferCommandList& command_list)
			{
				using Command = gpu::RenderCommandUpdateTexture;
				const gpu::TextureRegionUpdate region = {
					.subresource = {
						.layer_count = 1
					},
					.extent = {width_, height_, 1}
				};
				command_list.push<Command>({
					.dst_texture = registry.get_texture(parameter.persistent_texture),
					.data = test_texture_data_,
					.data_size = soul::cast<soul_size>(width_ * height_ * 4),
					.region_count = 1,
					.regions = &region
				});
			}).get_parameter();

		const auto dst_texture_node_id = render_graph.create_texture("Copy dst texture", gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, 
			{width_, height_}));
	    struct CopyPassParameter
		{
			gpu::TextureNodeID src_texture;
			gpu::TextureNodeID dst_texture;
		};
		auto copy_pass_parameter = render_graph.add_transfer_pass<CopyPassParameter>("Copy Texture Pass",
			[=](gpu::RGTransferPassDependencyBuilder& builder, CopyPassParameter& parameter)
			{
				parameter.src_texture = builder.add_src_texture(update_pass_parameter.persistent_texture);
				parameter.dst_texture = builder.add_dst_texture(dst_texture_node_id, gpu::TransferDataSource::GPU);
			},
			[this](const CopyPassParameter& parameter, const gpu::RenderGraphRegistry& registry, gpu::TransferCommandList& command_list)
			{
				using Command = gpu::RenderCommandCopyTexture;
				const gpu::TextureRegionCopy region = {
					.src_subresource = {.layer_count = 1},
					.dst_subresource = {.layer_count = 1},
					.extent = {width_, height_, 1}
				};
				command_list.push<Command>({
					.src_texture = registry.get_texture(parameter.src_texture),
					.dst_texture = registry.get_texture(parameter.dst_texture),
					.region_count = 1,
					.regions = &region
				});
			}).get_parameter();

		struct RenderPassParameter
		{
			gpu::TextureNodeID persistent_texture;
		};
		render_graph.add_graphic_pass<RenderPassParameter>("Render Pass",
			gpu::RGRenderTargetDesc(
				viewport,
				color_attachment_desc
			)
			, [copy_pass_parameter](gpu::RGShaderPassDependencyBuilder& builder, RenderPassParameter& parameter)
			{
				parameter.persistent_texture = builder.add_shader_texture(copy_pass_parameter.dst_texture, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderTextureReadUsage::UNIFORM);
			}, [viewport, this](const RenderPassParameter& parameter, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
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

				const gpu::TextureID persistent_texture = registry.get_texture(parameter.persistent_texture);
				const PushConstant push_constant = {
					.texture_descriptor_id = gpu_system_->get_srv_descriptor_id(persistent_texture),
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
	explicit Texture3DSampleApp(const AppConfig& app_config) : App(app_config), width_(0), height_(0), channel_count_(0)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("texture_transfer_command_sample.hlsl");
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
			test_texture_data_= stbi_load("assets/awesomeface.png", &width, &height, &channel_count, 0);
			width_ = width;
			height_ = height;
			channel_count_ = channel_count;

			test_texture_id_ = gpu_system_->create_texture(gpu::TextureDesc::d2("Test texture", gpu::TextureFormat::RGBA8, 1,
				{ gpu::TextureUsage::SAMPLED, gpu::TextureUsage::TRANSFER_SRC, gpu::TextureUsage::TRANSFER_DST }, { gpu::QueueType::GRAPHIC }, { static_cast<uint32>(width), static_cast<uint32>(height) }));
			test_sampler_id_ = gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT));

		}

	}
};

int main(int argc, char* argv[])
{
	stbi_set_flip_vertically_on_load(true);
	const ScreenDimension screen_dimension = { .width = 800, .height = 600 };
	Texture3DSampleApp app({ .screen_dimension = screen_dimension });
	app.run();

	return 0;
}
