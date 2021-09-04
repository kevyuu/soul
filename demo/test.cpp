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

using namespace Soul;
using namespace Demo;

void glfwPrintErrorCallback(int code, const char* message)
{
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

int main()
{
	SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

	glfwSetErrorCallback(glfwPrintErrorCallback);
	SOUL_ASSERT(0, glfwInit(), "GLFW Init Failed !");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw");

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	SOUL_ASSERT(0, mode != nullptr, "Mode cannot be nullpointer");
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Vulkan", nullptr, nullptr);
	glfwMaximizeWindow(window);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	Memory::MallocAllocator mallocAllocator("Default");
	Runtime::DefaultAllocator defaultAllocator(&mallocAllocator,
		Runtime::DefaultAllocatorProxy::Config(
			Memory::MutexProxy::Config(),
			Memory::CounterProxy::Config(),
			Memory::ClearValuesProxy::Config{ char(0xFA), char(0xFF) },
			Memory::BoundGuardProxy::Config()));

	Memory::PageAllocator pageAllocator("Page Allocator");
	Memory::LinearAllocator linearAllocator("Main Thread Temp Allocator", 10 * Soul::Memory::ONE_MEGABYTE, &pageAllocator);
	Runtime::TempAllocator tempAllocator(&linearAllocator,
		Runtime::TempProxy::Config());

	Runtime::Init({
		0,
		4096,
		&tempAllocator,
		20 * Soul::Memory::ONE_MEGABYTE,
		&defaultAllocator
		});


	GPU::System gpuSystem(Runtime::GetContextAllocator());
	GPU::System::Config config = {};
	config.windowHandle = window;
	config.swapchainWidth = 3360;
	config.swapchainHeight = 2010;
	config.maxFrameInFlight = 3;
	config.threadCount = Runtime::GetThreadCount();

	gpuSystem.init(config);

	UI::Init(window);

	ImGuiRenderModule imguiRenderModule;
	imguiRenderModule.init(&gpuSystem);

    SoulFila::Renderer renderer(&gpuSystem);
	renderer.init();
	renderer.getScene()->setViewport({ 614, 640 });

	UI::Store store;
	store.scene = renderer.getScene();
	store.gpuSystem = &gpuSystem;

	while (!glfwWindowShouldClose(window))
	{
		SOUL_PROFILE_FRAME();
		Runtime::System::Get().beginFrame();

		{
			SOUL_PROFILE_ZONE_WITH_NAME("GLFW Poll Events");
			glfwPollEvents();
		}

		GPU::RenderGraph renderGraph;
		GPU::TextureNodeID imguiFontNodeID = renderGraph.importTexture("Imgui Font", imguiRenderModule.getFontTexture());

		GPU::TextureNodeID renderTarget = renderer.computeRenderGraph(&renderGraph);

		store.fontTex = UI::SoulImTexture(imguiFontNodeID);
		store.sceneTex = UI::SoulImTexture(renderTarget);
		UI::Render(&store);

		imguiRenderModule.addPass(&gpuSystem, &renderGraph, *ImGui::GetDrawData(), gpuSystem.getSwapchainTexture());

		gpuSystem.renderGraphExecute(renderGraph);

		gpuSystem.frameFlush();

		renderGraph.cleanup();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
