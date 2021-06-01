#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"
#include "gpu/gpu.h"

#include <GLFW/glfw3.h>

#include "ui/ui.h"

#include "imgui_render_module.h"
#include "render_pipeline/deferred/renderer.h"
#include "render_pipeline/filament/data.h"
#include "render_pipeline/filament/gpu_program_registry.h"
#include "render_pipeline/filament/cgltf.h"

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

	cgltf_material kDefaultMat;
	kDefaultMat.name = (char*)"Default GLTF material";
	kDefaultMat.has_pbr_metallic_roughness = true;
	kDefaultMat.has_pbr_specular_glossiness = false;
	kDefaultMat.has_clearcoat = true;
	kDefaultMat.has_transmission = false;
	kDefaultMat.has_ior = false;
	kDefaultMat.has_specular = false;
	kDefaultMat.has_sheen = false;
	kDefaultMat.pbr_metallic_roughness = {
		{},
		{},
		{1.0, 1.0, 1.0, 1.0},
		1.0,
		1.0
	};
	cgltf_material* inputMat = &kDefaultMat;

	auto mrConfig = inputMat->pbr_metallic_roughness;
	auto sgConfig = inputMat->pbr_specular_glossiness;
	auto ccConfig = inputMat->clearcoat;
	auto trConfig = inputMat->transmission;
	auto shConfig = inputMat->sheen;

	bool hasTextureTransforms =
		sgConfig.diffuse_texture.has_transform ||
		sgConfig.specular_glossiness_texture.has_transform ||
		mrConfig.base_color_texture.has_transform ||
		mrConfig.metallic_roughness_texture.has_transform ||
		inputMat->normal_texture.has_transform ||
		inputMat->occlusion_texture.has_transform ||
		inputMat->emissive_texture.has_transform ||
		ccConfig.clearcoat_texture.has_transform ||
		ccConfig.clearcoat_roughness_texture.has_transform ||
		ccConfig.clearcoat_normal_texture.has_transform ||
		shConfig.sheen_color_texture.has_transform ||
		shConfig.sheen_roughness_texture.has_transform ||
		trConfig.transmission_texture.has_transform;

	cgltf_texture_view baseColorTexture = mrConfig.base_color_texture;
	cgltf_texture_view metallicRoughnessTexture = mrConfig.metallic_roughness_texture;

	SoulFila::GPUProgramKey matkey;
	matkey.doubleSided = !!inputMat->double_sided;
	matkey.unlit = !!inputMat->unlit;
	matkey.hasVertexColors = true;
	matkey.hasBaseColorTexture = !!baseColorTexture.texture;
	matkey.hasNormalTexture = !!inputMat->normal_texture.texture;
	matkey.hasOcclusionTexture = !!inputMat->occlusion_texture.texture;
	matkey.hasEmissiveTexture = !!inputMat->emissive_texture.texture;
	matkey.enableDiagnostics = true;
	matkey.baseColorUV = (uint8_t)baseColorTexture.texcoord;
	matkey.hasClearCoatTexture = !!ccConfig.clearcoat_texture.texture;
	matkey.clearCoatUV = (uint8_t)ccConfig.clearcoat_texture.texcoord;
	matkey.hasClearCoatRoughnessTexture = !!ccConfig.clearcoat_roughness_texture.texture;
	matkey.clearCoatRoughnessUV = (uint8_t)ccConfig.clearcoat_roughness_texture.texcoord;
	matkey.hasClearCoatNormalTexture = !!ccConfig.clearcoat_normal_texture.texture;
	matkey.clearCoatNormalUV = (uint8_t)ccConfig.clearcoat_normal_texture.texcoord;
	matkey.hasClearCoat = !!inputMat->has_clearcoat;
	matkey.hasTransmission = !!inputMat->has_transmission;
	matkey.hasTextureTransforms = hasTextureTransforms;
	matkey.emissiveUV = (uint8_t)inputMat->emissive_texture.texcoord;
	matkey.aoUV = (uint8_t)inputMat->occlusion_texture.texcoord;
	matkey.normalUV = (uint8_t)inputMat->normal_texture.texcoord;
	matkey.hasTransmissionTexture = !!trConfig.transmission_texture.texture;
	matkey.transmissionUV = (uint8_t)trConfig.transmission_texture.texcoord;
	matkey.hasSheenColorTexture = !!shConfig.sheen_color_texture.texture;
	matkey.sheenColorUV = (uint8_t)shConfig.sheen_color_texture.texcoord;
	matkey.hasSheenRoughnessTexture = !!shConfig.sheen_roughness_texture.texture;
	matkey.sheenRoughnessUV = (uint8_t)shConfig.sheen_roughness_texture.texcoord;
	matkey.hasSheen = !!inputMat->has_sheen;

	if (inputMat->has_pbr_specular_glossiness) {
		matkey.useSpecularGlossiness = true;
		if (sgConfig.diffuse_texture.texture) {
			baseColorTexture = sgConfig.diffuse_texture;
			matkey.hasBaseColorTexture = true;
			matkey.baseColorUV = (uint8_t)baseColorTexture.texcoord;
		}
		if (sgConfig.specular_glossiness_texture.texture) {
			metallicRoughnessTexture = sgConfig.specular_glossiness_texture;
			matkey.hasSpecularGlossinessTexture = true;
			matkey.specularGlossinessUV = (uint8_t)metallicRoughnessTexture.texcoord;
		}
	}
	else {
		matkey.hasMetallicRoughnessTexture = !!metallicRoughnessTexture.texture;
		matkey.metallicRoughnessUV = (uint8_t)metallicRoughnessTexture.texcoord;
	}

	switch (inputMat->alpha_mode) {
	case cgltf_alpha_mode_opaque:
		matkey.alphaMode = SoulFila::AlphaMode::OPAQUE;
		break;
	case cgltf_alpha_mode_mask:
		matkey.alphaMode = SoulFila::AlphaMode::MASK;
		break;
	case cgltf_alpha_mode_blend:
		matkey.alphaMode = SoulFila::AlphaMode::BLEND;
		break;
	}

	SoulFila::GPUProgramRegistry programRegistry(Runtime::GetContextAllocator(), &gpuSystem);
	programRegistry.createProgramSet(matkey);

	UI::Init(window);

	ImGuiRenderModule imguiRenderModule;
	imguiRenderModule.init(&gpuSystem);

    DeferredPipeline::Renderer renderer(&gpuSystem);
	renderer.init();
	renderer.getScene()->setViewport({ 1920, 1080 });

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
