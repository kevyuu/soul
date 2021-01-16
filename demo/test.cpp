#define VK_USE_PLATFORM_MACOS_MVK

#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"

#include "runtime/runtime.h"

#include "gpu/gpu.h"
#include <GLFW/glfw3.h>

#include "utils.h"
#include "ui/ui.h"

#include "imgui_render_module.h"
#include "render_pipeline/deferred/renderer.h"

#include <memory/allocators/page_allocator.h>
#include "data.h"
#include <vector>
#include <fstream>

#include <chrono>

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
		Runtime::DefaultAllocatorProxy(
			Memory::CounterProxy(),
			Memory::ClearValuesProxy(0xFA, 0xFF),
			Memory::BoundGuardProxy()));

	Memory::PageAllocator pageAllocator("Page Allocator");
	Memory::LinearAllocator linearAllocator("Main Thread Temp Allocator", 10 * ONE_MEGABYTE, &pageAllocator);
	Runtime::TempAllocator tempAllocator(&linearAllocator,
		Runtime::TempProxy());

	Runtime::Init({
		0,
		4096,
		&tempAllocator,
		20 * ONE_MEGABYTE,
		&defaultAllocator
		});

	GPU::System gpuSystem(Runtime::GetContextAllocator());
	GPU::System::Config config = {};
	config.windowHandle = window;
	config.swapchainWidth = 3360;
	config.swapchainHeight = 2010;
	config.maxFrameInFlight = 3;
	config.threadCount = Runtime::ThreadCount();

	gpuSystem.init(config);

	UI::Init(window);

	ImGuiRenderModule imguiRenderModule;
	imguiRenderModule.init(&gpuSystem);

    DeferredPipeline::Renderer renderer(&gpuSystem);
	renderer.init();
	renderer.getScene()->setViewport({ 1920, 1080 });

	UI::Store store;
	store.scene = renderer.getScene();
	store.gpuSystem = &gpuSystem;


	struct Vertex {
		Vec2f pos;
		Vec3f color;
	};

	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	GPU::BufferDesc vertexBufferDesc;
	vertexBufferDesc.typeSize = sizeof(Vertex);
	vertexBufferDesc.typeAlignment = alignof(Vertex);
	vertexBufferDesc.count = vertices.size();
	vertexBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
	vertexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;

	GPU::BufferID vertexBuffer = gpuSystem.bufferCreate(
		vertexBufferDesc,
		[&vertices](int i, byte* data) {
			auto vertex = (Vertex*)data;
			*vertex = vertices[i];
		}
	);

	const char* vertSrc = LoadFile("shaders/triangle.vert.glsl");
	GPU::ShaderDesc vertShaderDesc;
	vertShaderDesc.name = "Triangle Verte Shader";
	vertShaderDesc.source = vertSrc;
	vertShaderDesc.sourceSize = strlen(vertSrc);
	GPU::ShaderID vertShaderID = gpuSystem.shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

	const char* fragSrc = LoadFile("shaders/triangle.frag.glsl");
	GPU::ShaderDesc fragShaderDesc;
	fragShaderDesc.name = "Imgui fragment shader";
	fragShaderDesc.source = fragSrc;
	fragShaderDesc.sourceSize = strlen(fragSrc);
	GPU::ShaderID fragShaderID = gpuSystem.shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);

	struct Data {
		GPU::BufferNodeID vertexBuffer;
		GPU::TextureNodeID targetTex;
	};

	while (!glfwWindowShouldClose(window))
	{
		SOUL_PROFILE_FRAME();
		Runtime::System::Get().beginFrame();

		{
			SOUL_PROFILE_ZONE_WITH_NAME("GLFW Poll Events");
			glfwPollEvents();
		}

		Vec2ui32 extent = gpuSystem.getSwapchainExtent();

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
