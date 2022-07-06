#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"

#include "runtime/scope_allocator.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>

#include <random>

#include "ktx_bundle.h"

using namespace soul;


class TextureCubemapSampleApp final : public App
{
	static constexpr float CYCLE_DURATION = 30.0f;
	static constexpr soul::vec3ui32 DIMENSION{ 128, 128, 128 };

	using SkyboxVertex = float;
	static constexpr SkyboxVertex SKYBOX_VERTICES[] =
	{
		//   Coordinates
		  -1.0f, -1.0f,  1.0f,//        7--------6
		   1.0f, -1.0f,  1.0f,//       /|       /|
		   1.0f, -1.0f, -1.0f,//      4--------5 |
	    -1.0f, -1.0f, -1.0f,//      | |      | |
	    -1.0f,  1.0f,  1.0f,//     | 3------|-2
	     1.0f,  1.0f,  1.0f,//     |/       |/
	     1.0f,  1.0f, -1.0f,//     0--------1
		-1.0f,  1.0f, -1.0f
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

	void render(gpu::RenderGraph& render_graph) override
	{
		
	}

public:
	explicit TextureCubemapSampleApp(const AppConfig& app_config) : App(app_config)
	{
		runtime::ScopeAllocator scope_allocator("Texture Cubemap Sample App");
		gpu::ShaderSource shader_source = gpu::ShaderFile("texture_cubemap_sample.hlsl");
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

		skybox_vertex_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(SkyboxVertex) * std::size(SKYBOX_VERTICES),
			.usage_flags = { gpu::BufferUsage::VERTEX },
			.queue_flags = { gpu::QueueType::GRAPHIC }
			}, SKYBOX_VERTICES);
		gpu_system_->flush_buffer(skybox_vertex_buffer_id_);

		skybox_index_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(SkyboxIndex) * std::size(SKYBOX_INDICES),
			.usage_flags = { gpu::BufferUsage::INDEX },
			.queue_flags = { gpu::QueueType::GRAPHIC }
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

			SOUL_ASSERT(0, ktxinfo.glType == image::KtxBundle::R11F_G11F_B10F, "");

			const auto tex_desc = gpu::TextureDesc::cube(name, 
			    gpu::TextureFormat::R11F_G11F_B10F, nmips, 
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

		ibl.reflectionTex = create_cube_map("./assets/default_env/default_env_ibl.ktx", "Default env IBL");


	}
};

int main(int argc, char* argv[])
{
	stbi_set_flip_vertically_on_load(true);
	TextureCubemapSampleApp app({ .enable_imgui = true });
	app.run();

	return 0;
}
