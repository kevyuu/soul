#include "texture_2d_pass.h"

#include "gpu/gpu.h"

static constexpr const char* TEXTURE_2D_HLSL = R"HLSL(
struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
	[[vk::location(1)]] float2 tex_coord: TEXCOORD;
};

[[vk::push_constant]]
struct push_constant {
	uint texture_descriptor_id;
	uint sampler_descriptor_id;
} push_constant;

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 tex_coord: TEXCOORD;
};

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 0.0, 1.0);
	output.tex_coord = input.tex_coord;
	return output;
}

struct PSOutput
{
	[[vk::location(0)]] float4 color: SV_Target;
};

[shader("pixel")]
PSOutput psMain(VSOutput input)
{
	PSOutput output;
	Texture2D test_texture = get_texture_2d(push_constant.texture_descriptor_id);
	SamplerState test_sampler = get_sampler(push_constant.sampler_descriptor_id);
	output.color = test_texture.Sample(test_sampler, input.tex_coord);
	return output;
}
)HLSL";

using namespace soul;

struct Vertex
{
	vec2f position = {};
	vec2f texture_coords = {};
};

static constexpr Vertex VERTICES[4] = {
	{{-1.0f, -1.0f}, {0.0f, 1.0f}}, // top left
	{{1.0f, -1.0f}, {1.0f, 1.0f}}, // top right
	{{1.0f, 1.0f}, {1.0f, 0.0f}}, // bottom right
	{{-1.0f, 1.0f}, {0.0f, 0.0f}} // bottom left
};

using Index = uint16;
static constexpr Index INDICES[] = {
	0, 1, 2, 2, 3, 0
};

Texture2DRGPass::Texture2DRGPass(soul::gpu::System* gpu_system) : gpu_system_(gpu_system)
{
	gpu::ShaderSource shader_source = gpu::ShaderString(TEXTURE_2D_HLSL);
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

	sampler_id_ = gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(gpu::TextureFilter::LINEAR, gpu::TextureWrap::REPEAT));
}

Texture2DRGPass::~Texture2DRGPass()
{
    
}

void Texture2DRGPass::add_pass(const Parameter& parameter, soul::gpu::RenderGraph& render_graph)
{
	const gpu::TextureID swapchain_texture_id = gpu_system_->get_swapchain_texture();
	const gpu::TextureNodeID render_target = render_graph.import_texture("Color Output", swapchain_texture_id);
	const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
	};

	const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

	render_graph.add_graphic_pass<Parameter>("Render Pass Parameter",
		gpu::RGRenderTargetDesc(
		    viewport,
			color_attachment_desc
		)
		,[this, in_parameter = parameter](gpu::RGShaderPassDependencyBuilder& builder, Parameter& parameter)
		{
			parameter.sampled_texture = builder.add_shader_texture(in_parameter.sampled_texture, { gpu::ShaderStage::FRAGMENT }, gpu::ShaderTextureReadUsage::UNIFORM);
		}
		,[this, viewport](const Parameter& parameter, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
		{
			const gpu::GraphicPipelineStateDesc pipeline_desc = {
					.program_id = program_id_,
					.input_bindings = {
						{.stride = sizeof(Vertex)}
					},
					.input_attributes = {
						{.binding = 0, .offset = offsetof(Vertex, position), .type = gpu::VertexElementType::FLOAT2},
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
				.texture_descriptor_id = gpu_system_->get_srv_descriptor_id(registry.get_texture(parameter.sampled_texture)),
				.sampler_descriptor_id = gpu_system_->get_sampler_descriptor_id(sampler_id_)
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