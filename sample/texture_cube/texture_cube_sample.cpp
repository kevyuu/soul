#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"

#include "runtime/scope_allocator.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>

#include <random>

#include <fstream>

#include "ktx_bundle.h"
#include "camera_manipulator.h"

using namespace soul;

class TextureCubeSampleApp final : public App
{
	static constexpr float CYCLE_DURATION = 30.0f;
	static constexpr soul::vec3ui32 DIMENSION{ 128, 128, 128 };

	using SkyboxVertex = soul::vec3f;
	static constexpr SkyboxVertex SKYBOX_VERTICES[] =
	{
		//   Coordinates
		{-5.0f, -5.0f,  5.0f},//        7--------6
		{5.0f, -5.0f,  5.0f},//        /|       /|
		{5.0f, -5.0f, -5.0f},//       4--------5 |
		{-5.0f, -5.0f, -5.0f},//      | |      | |
		{-5.0f,  5.0f,  5.0f },//     | 3------|-2
		 {5.0f,  5.0f,  5.0f},//      |/       |/
		 {5.0f,  5.0f, -5.0f },//     0--------1
		{-5.0f,  5.0f, -5.0f}
	};

	using SkyboxIndex = uint16;
	static constexpr SkyboxIndex SKYBOX_INDICES[] =
	{
		// Right
		1, 2, 6,
		6, 5, 1,
		// Left
		0, 4, 7,
		7, 3, 0,
		// Top
		4, 5, 6,
		6, 7, 4,
		// Bottom
		0, 3, 2,
		2, 1, 0,
		// Back
		0, 1, 5,
		5, 4, 0,
		// Front
		3, 7, 6,
		6, 2, 3
	};

	gpu::ProgramID program_id_ = gpu::ProgramID();
	gpu::BufferID skybox_vertex_buffer_id_ = gpu::BufferID();
	gpu::BufferID skybox_index_buffer_id_ = gpu::BufferID();

	gpu::TextureID skybox_texture_ = gpu::TextureID();
	gpu::SamplerID skybox_sampler_ = gpu::SamplerID();

	void render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph) override
	{
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
		};

		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();
		struct RenderPassParameter {};
		render_graph.add_graphic_pass<RenderPassParameter>("Render texture cube pass",
			gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
			[this](gpu::RGShaderPassDependencyBuilder& builder, auto parameter)
			{

			}, [viewport, this](const auto& parameter, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
			{
				const gpu::GraphicPipelineStateDesc pipeline_desc = {
						.program_id = program_id_,
						.input_bindings = {
							{.stride = sizeof(SkyboxVertex)}
						},
						.input_attributes = {
							{.binding = 0, .offset = 0, .type = gpu::VertexElementType::FLOAT3},
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

				using Command = gpu::RenderCommandDrawIndex;

				const struct PushConstant {
					soulsl::float4x4 projection = soulsl::float4x4();
					soulsl::float4x4 view = soulsl::float4x4();
					soulsl::DescriptorID texture_descriptor_id;
					soulsl::DescriptorID sampler_descriptor_id;
					float align1 = 0.0f;
					float align2 = 0.0f;
				} push_constant = {
				    .projection = soul::math::perspective(soul::math::radians(60.0f), soul::math::fdiv(viewport.x, viewport.y), 0.1f, 512.0f),
					.view = camera_man_.get_view_matrix(),
					.texture_descriptor_id = gpu_system_->get_srv_descriptor_id(skybox_texture_),
					.sampler_descriptor_id = gpu_system_->get_sampler_descriptor_id(skybox_sampler_),
				};

				command_list.push<Command>({
					.pipeline_state_id = pipeline_state_id,
					.push_constant_data = soul::cast<void*>(&push_constant),
					.push_constant_size = sizeof(PushConstant),
					.vertex_buffer_ids = {
						skybox_vertex_buffer_id_
					},
					.index_buffer_id = skybox_index_buffer_id_,
					.first_index = 0,
					.index_count = std::size(SKYBOX_INDICES)
				});
			});
	}

public:
	explicit TextureCubeSampleApp(const AppConfig& app_config) :
		App(app_config)
	{
		runtime::ScopeAllocator scope_allocator("Texture Cube Sample App");
		gpu::ShaderSource shader_source = gpu::ShaderFile("texture_cube_sample.hlsl");
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

		skybox_vertex_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(SkyboxVertex) * std::size(SKYBOX_VERTICES),
			.usage_flags = { gpu::BufferUsage::VERTEX },
			.queue_flags = { gpu::QueueType::GRAPHIC },
			.name = "Skybox vertex buffer"
		}, SKYBOX_VERTICES);
		gpu_system_->flush_buffer(skybox_vertex_buffer_id_);

		skybox_index_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(SkyboxIndex) * std::size(SKYBOX_INDICES),
			.usage_flags = { gpu::BufferUsage::INDEX },
			.queue_flags = { gpu::QueueType::GRAPHIC },
			.name = "Skybox index buffer"
		}, SKYBOX_INDICES);
		gpu_system_->flush_buffer(skybox_index_buffer_id_);


		auto create_cube_map = [this](const char* path, const char* name) -> gpu::TextureID
		{
			using namespace std;
			ifstream file(path, ios::binary);
			std::vector<uint8_t> contents((istreambuf_iterator<char>(file)), {});
			image::KtxBundle ktx(contents.data(), soul::cast<uint32>(contents.size()));
			const auto& ktxinfo = ktx.getInfo();
			const auto nmips = ktx.getNumMipLevels();

			SOUL_ASSERT(0, 
				ktxinfo.glType == image::KtxBundle::UNSIGNED_BYTE && ktxinfo.glFormat == image::KtxBundle::RGBA,
				"");

			const auto tex_desc = gpu::TextureDesc::cube(name, 
			    gpu::TextureFormat::RGBA8, nmips, 
			    { gpu::TextureUsage::SAMPLED }, 
			    { gpu::QueueType::GRAPHIC }, 
			    { ktxinfo.pixelWidth, ktxinfo.pixelHeight });

			soul::Vector<gpu::TextureRegionUpdate> region_loads;
			region_loads.reserve(nmips);

			const auto* skybox_data = ktx.getRawData();
			const auto skybox_data_size = ktx.getTotalSize();
		    
			for (uint32 level = 0; level < nmips; level++)
			{
				uint8* level_data;
				uint32 level_size;
				ktx.getBlob({ level, 0, 0 }, &level_data, &level_size);
				const auto level_width = std::max(1u, ktxinfo.pixelWidth >> level);
				const auto level_height = std::max(1u, ktxinfo.pixelHeight >> level);

				const gpu::TextureRegionUpdate region_update = {
					.buffer_offset = soul::cast<uint64>(level_data - skybox_data),
				    .subresource = {
						.mip_level = level,
					    .base_array_layer = 0,
						.layer_count = 6
					},
					.extent = {level_width, level_height, 1}
				};

				region_loads.add(region_update);
			}
			const gpu::TextureLoadDesc load_desc = {
				.data = skybox_data,
				.data_size = skybox_data_size,
				.region_count = soul::cast<uint32>(region_loads.size()),
				.regions = region_loads.data()
			};

			const auto texture_id = gpu_system_->create_texture(tex_desc, load_desc);
			gpu_system_->flush_texture(texture_id, {gpu::TextureUsage::SAMPLED});

			return texture_id;
		};

		skybox_texture_ = create_cube_map("assets/cubemap_yokohama_rgba.ktx", "Default env IBL");
		skybox_sampler_ = gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(
		    gpu::TextureFilter::LINEAR,
			gpu::TextureWrap::CLAMP_TO_EDGE
		));
	}
};

int main(int argc, char* argv[])
{
	stbi_set_flip_vertically_on_load(true);
	TextureCubeSampleApp app({ .enable_imgui = false });
	app.run();

	return 0;
}
