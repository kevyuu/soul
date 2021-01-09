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

	const char* vertSrc = LoadFile("shaders/unlit.vert.glsl");
	GPU::ShaderDesc vertShaderDesc;
	vertShaderDesc.name = "Triangle Verte Shader";
	vertShaderDesc.source = vertSrc;
	vertShaderDesc.sourceSize = strlen(vertSrc);
	GPU::ShaderID vertShaderID = gpuSystem.shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

	const char* fragSrc = LoadFile("shaders/unlit.frag.glsl");
	GPU::ShaderDesc fragShaderDesc;
	fragShaderDesc.name = "Unlit fragment shader";
	fragShaderDesc.source = fragSrc;
	fragShaderDesc.sourceSize = strlen(fragSrc);
	GPU::ShaderID fragShaderID = gpuSystem.shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);

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
		/*GPU::TextureNodeID imguiFontNodeID = renderGraph.importTexture("Imgui Font", imguiRenderModule.getFontTexture());

		GPU::TextureNodeID renderTarget = renderer.computeRenderGraph(&renderGraph);

		store.fontTex = UI::SoulImTexture(imguiFontNodeID);
		store.sceneTex = UI::SoulImTexture(renderTarget);
		UI::Render(&store);

		imguiRenderModule.addPass(&gpuSystem, &renderGraph, *ImGui::GetDrawData(), gpuSystem.getSwapchainTexture());

		gpuSystem.renderGraphExecute(renderGraph);
		renderGraph.cleanup();

		gpuSystem.frameFlush();*/


		uint32 width = gpuSystem.getSwapchainExtent().x;
		uint32 height = gpuSystem.getSwapchainExtent().y;
		GPU::TextureNodeID renderTargetNodeID = renderGraph.importTexture("Render target", gpuSystem.getSwapchainTexture());

		Vec2f positions[4] = {
			{-0.5f, -0.5f},
			{0.5f, -0.5f},
			{0.5f, 0.5f},
			{-0.5f, 0.5f}
		};
		GPU::BufferDesc posBufferDesc;
		posBufferDesc.count = sizeof(positions) / sizeof(positions[0]);
		posBufferDesc.typeSize = sizeof(Vec2f);
		posBufferDesc.typeAlignment = alignof(Vec2f);
		posBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		posBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
		GPU::BufferID posVertexBuffer = gpuSystem.bufferCreate(
			posBufferDesc,
			[positions](int index, void* byte) {
				Vec2f* position = (Vec2f*)byte;
				(*position) = positions[index];
			}
		);
		GPU::BufferNodeID posBufferNodeID = renderGraph.importBuffer("Pos Buffer", posVertexBuffer);

		Vec3f colors[4] = {
			{1.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f}
		};
		GPU::BufferDesc colorBufferDesc;
		colorBufferDesc.count = sizeof(colors) / sizeof(colors[0]);
		colorBufferDesc.typeSize = sizeof(Vec3f);
		colorBufferDesc.typeAlignment = alignof(Vec3f);
		colorBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		colorBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
		GPU::BufferID colorVertexBuffer = gpuSystem.bufferCreate(
			colorBufferDesc,
			[colors](int index, void* byte) {
				Vec3f* color = (Vec3f*)byte;
				(*color) = colors[index];
			}
		);
		GPU::BufferNodeID colorBufferNodeID = renderGraph.importBuffer("Color Buffer", colorVertexBuffer);

		uint16 indexes[6] = {
			 0, 1, 2, 2, 3, 0
		};
		GPU::BufferDesc indexBufferDesc;
		indexBufferDesc.count = sizeof(indexes) / sizeof(indexes[0]);
		indexBufferDesc.typeSize = sizeof(uint16);
		indexBufferDesc.typeAlignment = alignof(uint16);
		indexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		indexBufferDesc.usageFlags = GPU::BUFFER_USAGE_INDEX_BIT;
		GPU::BufferID indexBuffer = gpuSystem.bufferCreate(
			indexBufferDesc,
			[indexes](int i, void* byte) {
				uint16* index = (uint16*)byte;
				(*index) = indexes[i];
			}
		);
		GPU::BufferNodeID indexBufferNodeID = renderGraph.importBuffer("Index Buffer", indexBuffer);

		struct PassData {
			GPU::BufferNodeID posVertexBuffer;
			GPU::BufferNodeID colorVertexBuffer;
			GPU::BufferNodeID indexBuffer;
			GPU::TextureNodeID renderTargetTex;
		};

		renderGraph.addGraphicPass<PassData>(
			"Rectange Render Pass",
			[posBufferNodeID, colorBufferNodeID, indexBufferNodeID, renderTargetNodeID, vertShaderID, fragShaderID, width, height]
		(GPU::GraphicNodeBuilder* builder, PassData* data) {
				data->posVertexBuffer = builder->addVertexBuffer(posBufferNodeID);
				data->colorVertexBuffer = builder->addVertexBuffer(colorBufferNodeID);
				data->indexBuffer = builder->addIndexBuffer(indexBufferNodeID);
				GPU::ColorAttachmentDesc colorAttchDesc;
				colorAttchDesc.blendEnable = false;
				colorAttchDesc.clear = true;
				colorAttchDesc.clearValue.color.float32 = {0, 0, 0, 0};
				data->renderTargetTex = builder->addColorAttachment(renderTargetNodeID, colorAttchDesc);

				Vec2f sceneResolution = Vec2f(width, height);
				GPU::GraphicPipelineConfig pipelineConfig;
				pipelineConfig.viewport = { 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
				pipelineConfig.scissor = { false, 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
				pipelineConfig.framebuffer = { uint16(sceneResolution.x), uint16(sceneResolution.y) };
				pipelineConfig.vertexShaderID = vertShaderID;
				pipelineConfig.fragmentShaderID = fragShaderID;
				pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

				builder->setPipelineConfig(pipelineConfig);
			},
			[]
			(GPU::RenderGraphRegistry* registry, const PassData& passData, GPU::CommandBucket* commandBucket) {
				using DrawCommand = GPU::Command::DrawIndex2;
				DrawCommand* command = commandBucket->add<DrawCommand>(0);
				command->vertexBufferIDs[0] = registry->getBuffer(passData.posVertexBuffer);
				command->vertexBufferIDs[1] = registry->getBuffer(passData.colorVertexBuffer);
				command->vertexCount = 2;
				command->indexBufferID = registry->getBuffer(passData.indexBuffer);
				command->indexCount = 6;
			}
		);

		gpuSystem.renderGraphExecute(renderGraph);
		gpuSystem.frameFlush();
		renderGraph.cleanup();

	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}