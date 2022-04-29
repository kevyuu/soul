#include "argparse.h"

#include <string>

#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"
#include "gpu/gpu.h"

#include <GLFW/glfw3.h>

#include "ui/ui.h"

#include "imgui_render_module.h"
#include "render_pipeline/filament/renderer.h"
#include "render_pipeline/filament/data.h"

#include "data.h"

using namespace soul;
using namespace Demo;

void glfwPrintErrorCallback(int code, const char* message)
{
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

struct Args : public argparse::Args
{
	std::optional<std::string>& path = kwarg("g,gltf", "GLTF file to show");
};

int main(int argc, char* argv[])
{
	auto args = argparse::parse<Args>(argc, argv);
	args.print();

	SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

	glfwSetErrorCallback(glfwPrintErrorCallback);
	bool glfw_init_success = glfwInit();
	SOUL_ASSERT(0, glfw_init_success, "GLFW initialization failed!");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW Initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw");

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	SOUL_ASSERT(0, mode != nullptr, "Mode cannot be nullpointer");
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Vulkan", nullptr, nullptr);
	glfwMaximizeWindow(window);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	memory::MallocAllocator mallocAllocator("Default");
	runtime::DefaultAllocator defaultAllocator(&mallocAllocator,
		runtime::DefaultAllocatorProxy::Config(
			memory::MutexProxy::Config(),
			memory::ProfileProxy::Config(),
			memory::CounterProxy::Config(),
			memory::ClearValuesProxy::Config{ char(0xFA), char(0xFF) },
			memory::BoundGuardProxy::Config()));

	memory::PageAllocator pageAllocator("Page Allocator");
	memory::LinearAllocator linearAllocator("Main Thread Temporary Allocator", 10 * soul::memory::ONE_MEGABYTE, &pageAllocator);
	runtime::TempAllocator tempAllocator(&linearAllocator,
		runtime::TempProxy::Config());

	runtime::init({
		0,
		4096,
		&tempAllocator,
		20 * soul::memory::ONE_MEGABYTE,
		&defaultAllocator
		});


	gpu::System gpuSystem(runtime::get_context_allocator());
	gpu::System::Config config = {};
	config.windowHandle = window;
	config.swapchainWidth = 3360;
	config.swapchainHeight = 2010;
	config.maxFrameInFlight = 3;
	config.threadCount = runtime::get_thread_count();

	gpuSystem.init(config);

	UI::Init(window);

	ImGuiRenderModule imguiRenderModule;
	imguiRenderModule.init(&gpuSystem);

    SoulFila::Renderer renderer(&gpuSystem);
	renderer.init();
	renderer.getScene()->setViewport({ 1448, 1057 });

	UI::Store store;
	store.scene = renderer.getScene();
	store.gpuSystem = &gpuSystem;

	if (args.path)
		renderer.getScene()->importFromGLTF(args.path.value().c_str());

	while (!glfwWindowShouldClose(window))
	{
		SOUL_PROFILE_FRAME();
		runtime::System::Get().beginFrame();

		{
			SOUL_PROFILE_ZONE_WITH_NAME("GLFW Poll Events");
			glfwPollEvents();
		}

		gpu::RenderGraph renderGraph;
		gpu::TextureNodeID imguiFontNodeID = renderGraph.import_texture("ImGui Font", imguiRenderModule.getFontTexture());

		gpu::TextureNodeID renderTarget = renderer.computeRenderGraph(&renderGraph);

		store.fontTex = UI::SoulImTexture(imguiFontNodeID);
		store.sceneTex = UI::SoulImTexture(renderTarget);
		UI::Render(&store);

		imguiRenderModule.addPass(&gpuSystem, &renderGraph, *ImGui::GetDrawData(), gpuSystem.getSwapchainTexture());

		gpuSystem.execute(renderGraph);

		gpuSystem.frameFlush();

		renderGraph.cleanup();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
