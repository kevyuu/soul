#include "gpu/gpu.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>

#include "imgui/imgui.h"
#include "texture_2d_pass.h"

using namespace soul;

class ImguiSampleApp final : public App
{
	Texture2DRGPass texture_2d_pass_;
	void render(gpu::RenderGraph& render_graph) override
	{
		ImGui::ShowDemoWindow();
		
		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();
	    const gpu::TextureNodeID blank_texture = render_graph.create_texture("Blank Texture",
			gpu::RGTextureDesc::create_d2(gpu::TextureFormat::RGBA8, 1, viewport, true, {}));
		texture_2d_pass_.add_pass({ blank_texture }, render_graph);
	}

public:
	explicit ImguiSampleApp(const AppConfig& app_config) : App(app_config), texture_2d_pass_(gpu_system_) {}
	
};

int main(int argc, char* argv[])
{
	stbi_set_flip_vertically_on_load(true);
	ImguiSampleApp app({.enable_imgui = true });
	app.run();
	return 0;
}