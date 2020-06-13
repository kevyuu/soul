#define VK_USE_PLATFORM_MACOS_MVK

#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"

#include "runtime/runtime.h"

#include "gpu/gpu.h"
#include <GLFW/glfw3.h>

#include "utils.h"
#include "widget/scene_panel.h"

#include "imgui_render_module.h"
#include "shadow_map_gen_render_module.h"
#include "final_gather_render_module.h"
#include "voxelize_render_module.h"
#include "voxel_gi_debug_render_module.h"
#include "gbuffer_gen_render_module.h"
#include "voxel_light_inject_render_module.h"

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>

#include <memory/allocators/page_allocator.h>
#include "scene.h"

using namespace Soul;

void glfwPrintErrorCallback(int code, const char* message)
{
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

int main()
{
	SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

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

	glfwSetErrorCallback(glfwPrintErrorCallback);
	SOUL_ASSERT(0, glfwInit(), "GLFW Init Failed !");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan not supported by glfw");

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Vulkan", nullptr, nullptr);
	glfwMaximizeWindow(window);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	GPU::System gpuSystem(Runtime::GetContextAllocator());
	GPU::System::Config config = {};
	config.windowHandle = window;
	config.swapchainWidth = 3360;
	config.swapchainHeight = 2010;
	config.maxFrameInFlight = 3;
	config.threadCount = Runtime::System::Get().getThreadCount();

	gpuSystem.init(config);

	Vec2ui32 sceneResolution = { 1920, 1080 };
	ScenePanel scenePanel(sceneResolution);
	ScenePanel scenePanel2(sceneResolution);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGuiRenderModule imguiRenderModule;
	imguiRenderModule.init(&gpuSystem);

	ShadowMapGenRenderModule shadowMapGenRenderModule;
	shadowMapGenRenderModule.init(&gpuSystem);

	GBufferGenRenderModule gBufferGenRenderModule;
	gBufferGenRenderModule.init(&gpuSystem);

	FinalGatherRenderModule finalGatherRenderModule;
	finalGatherRenderModule.init(&gpuSystem);

	VoxelizeRenderModule voxelizeRenderModule;
	voxelizeRenderModule.init(&gpuSystem);

	VoxelGIDebugRenderModule voxelGIDebugRenderModule;
	voxelGIDebugRenderModule.init(&gpuSystem);

	VoxelLightInjectRenderModule voxelLightInjectRenderModule;
	voxelLightInjectRenderModule.init(&gpuSystem);

    Scene scene;
    LoadScene(&gpuSystem, &scene,
              "assets/sponza/scene.gltf",
              true);

	Camera& camera = scene.camera;
	camera.position = Vec3f(1.0f, 1.0f, 0.0f);
	camera.direction = Vec3f(0.0f, 0.0f, -1.0f);
	camera.up = Vec3f(0.0f, 1.0f, 0.0f);
	camera.perspective.fov = PI / 4;
	camera.perspective.aspectRatio = sceneResolution.x / sceneResolution.y;
	camera.viewportWidth = sceneResolution.x;
	camera.viewportHeight = sceneResolution.y;
	camera.perspective.zNear = 0.1f;
	camera.perspective.zFar = 30.0f;
	camera.projection = mat4Perspective(
		camera.perspective.fov,
        camera.perspective.aspectRatio,
        camera.perspective.zNear,
        camera.perspective.zFar);

	struct CameraUBO
	{
		Mat4 projection;
		Mat4 view;
		Mat4 projectionView;
		Mat4 invProjectionView;

		Vec3f position;
		float exposure;
	};
	CameraUBO cameraDataUBO;

	struct DirLightUBO {
		Mat4 shadowMatrixes[4];
		Vec3f direction;
		float bias;
		Vec3f color;
		float preExposedIlluminance;
		float cascadeDepths[4];
	};

	GPU::BufferDesc modelBufferDesc;
	modelBufferDesc.typeSize = sizeof(Mat4);
	modelBufferDesc.typeAlignment = alignof(Mat4);
	modelBufferDesc.count = scene.meshEntities.size();
	modelBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	modelBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::BufferID modelBuffer = gpuSystem.bufferCreate(modelBufferDesc,
   [&scene](int i, byte* data)
   {
       auto model = (Mat4*)data;
       *model = mat4Transpose(
           mat4Transform(scene.meshEntities[i].worldTransform));
   });

	GPU::BufferDesc rotationBufferDesc;
    rotationBufferDesc.typeSize = sizeof(Mat4);
    rotationBufferDesc.typeAlignment = alignof(Mat4);
    rotationBufferDesc.count = scene.meshEntities.size();
    rotationBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
    rotationBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
    GPU::BufferID rotationBuffer = gpuSystem.bufferCreate(rotationBufferDesc,
    [&scene](int i, byte* data)
    {
       auto rotate = (Mat4*)data;
       *rotate = mat4Transpose(
               mat4Rotate(mat4Transform(scene.meshEntities[i].worldTransform)));
    });

	if (scene.textures.size() == 0) {
		scene.textures.add({"fontTex", imguiRenderModule._fontTex});
	}
	
	GPU::TextureDesc texDesc;
	texDesc.width = 1;
	texDesc.height = 1;
	texDesc.depth = 1;
	texDesc.type = GPU::TextureType::D2;
	texDesc.format = GPU::TextureFormat::RGBA8;
	texDesc.mipLevels = 1;
	texDesc.usageFlags = GPU::TEXTURE_USAGE_SAMPLED_BIT;
	texDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	texDesc.name = "Stub texture";
	uint32 stubTextureData = 0;
	GPU::TextureID stubTexture = gpuSystem.textureCreate(texDesc, (byte*)&stubTextureData, sizeof(stubTextureData));

    Vec2f quadVertices[] = {
        Vec2f(-1.0f, -1.0f),
        Vec2f(-1.0f, 1.0f),
        Vec2f(1.0f, -1.0f),
        Vec2f(1.0f, 1.0f)
    };
	GPU::BufferDesc quadBufferDesc;
    quadBufferDesc.typeSize = sizeof(Vec2f);
    quadBufferDesc.typeAlignment = alignof(Vec2f);
    quadBufferDesc.count = 4;
    quadBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
    quadBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
    GPU::BufferID quadBuffer = gpuSystem.bufferCreate(quadBufferDesc,
    [&quadVertices](int i, byte* data)
    {
        auto vertex = (Vec2f*)data;
        *vertex = quadVertices[i];
    });

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

		cameraDataUBO.projection = mat4Transpose(camera.projection);
		Mat4 viewMat = mat4View(camera.position, camera.position +
			camera.direction, camera.up);
		cameraDataUBO.view = mat4Transpose(viewMat);
		Mat4 projectionView = camera.projection * viewMat;
		cameraDataUBO.projectionView = mat4Transpose(projectionView);
		Mat4 invProjectionView = mat4Inverse(projectionView);
		cameraDataUBO.invProjectionView = mat4Transpose(invProjectionView);
		cameraDataUBO.position = camera.position;
		cameraDataUBO.exposure = camera.exposure;

		GPU::BufferDesc cameraBufferDesc;
		cameraBufferDesc.typeSize = sizeof(CameraUBO);
		cameraBufferDesc.typeAlignment = alignof(CameraUBO);
		cameraBufferDesc.count = 1;
		cameraBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		cameraBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID cameraBuffer = gpuSystem.bufferCreate(cameraBufferDesc,
			[&cameraDataUBO](int i, byte* data) {
				auto cameraData = (CameraUBO*)data;
				*cameraData = cameraDataUBO;
			});
		gpuSystem.bufferDestroy(cameraBuffer);	

		DirLightUBO dirLightUBO = {};
		for (int i = 0; i < 4; i++)
		{
			dirLightUBO.shadowMatrixes[i] = mat4Transpose(scene.dirLight.shadowMatrixes[i]);
		}
		dirLightUBO.direction = scene.dirLight.direction;
		dirLightUBO.bias = scene.dirLight.bias;
		dirLightUBO.color = scene.dirLight.color;
		dirLightUBO.preExposedIlluminance = scene.dirLight.illuminance;
		float cameraFar = camera.perspective.zFar;
		float cameraNear = camera.perspective.zNear;
		float cameraDepth = cameraFar - cameraNear;
		for (int j = 0; j < 3; j++) {
			dirLightUBO.cascadeDepths[j] = camera.perspective.zNear + (cameraDepth * scene.dirLight.split[j]);
		}
		dirLightUBO.cascadeDepths[3] = cameraFar;

		GPU::BufferDesc lightBufferDesc;
		lightBufferDesc.typeSize = sizeof(DirLightUBO);
		lightBufferDesc.typeAlignment = alignof(DirLightUBO);
		lightBufferDesc.count = 1;
		lightBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		lightBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT | GPU::QUEUE_COMPUTE_BIT;
		GPU::BufferID lightBuffer = gpuSystem.bufferCreate(lightBufferDesc,
			[&dirLightUBO](int i, byte* data) -> void
			{
				auto lightData = (DirLightUBO*)data;
				*lightData = dirLightUBO;
			});
		gpuSystem.bufferDestroy(lightBuffer);

		GPU::BufferDesc shadowMatrixesBufferDesc;
		shadowMatrixesBufferDesc.typeSize = sizeof(Mat4);
		shadowMatrixesBufferDesc.typeAlignment = alignof(Mat4);
		shadowMatrixesBufferDesc.count = 4;
		shadowMatrixesBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		shadowMatrixesBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID shadowMatrixesBuffer = gpuSystem.bufferCreate(shadowMatrixesBufferDesc,
			[&scene](int i, byte* data) -> void {
				auto shadowMatrix = (Mat4*)data;
				*shadowMatrix = mat4Transpose(scene.dirLight.shadowMatrixes[i]);
			});
		gpuSystem.bufferDestroy(shadowMatrixesBuffer);

		Vec4f dummy = scene.dirLight.shadowMatrixes[0] * Vec4f(100.0f, 1.0f, 1.0f, 1.0f);

		Array<GPU::TextureNodeID> sceneTextureNodeIDs;
		{
			sceneTextureNodeIDs.reserve(scene.textures.size());
			for (const SceneTexture& sceneTexture : scene.textures) {
				sceneTextureNodeIDs.add(renderGraph.importTexture("Scene Textures", sceneTexture.rid));
			}
		}

        struct VoxelGIDataUBO {
            Vec3f frustumCenter;
            int resolution;
            float frustumHalfSpan;
            float bias;
            float diffuseMultiplier;
            float specularMultiplier;
        };
		GPU::BufferDesc voxelGIDataBufferDesc;
		voxelGIDataBufferDesc.typeSize = sizeof(VoxelGIDataUBO);
		voxelGIDataBufferDesc.typeAlignment = alignof(VoxelGIDataUBO);
		voxelGIDataBufferDesc.count = 1;
		voxelGIDataBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		voxelGIDataBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT | GPU::QUEUE_COMPUTE_BIT;
        GPU::BufferID voxelGIDataBuffer = gpuSystem.bufferCreate(voxelGIDataBufferDesc,
            [&scene](int i, byte* data) -> void {
            VoxelGIDataUBO& voxelGIDataUBO = *(VoxelGIDataUBO*) data;
            voxelGIDataUBO.frustumCenter = scene.voxelGIConfig.center;
            voxelGIDataUBO.resolution = scene.voxelGIConfig.resolution;
            voxelGIDataUBO.bias = scene.voxelGIConfig.bias;
            voxelGIDataUBO.frustumHalfSpan = scene.voxelGIConfig.halfSpan;
            voxelGIDataUBO.diffuseMultiplier = scene.voxelGIConfig.diffuseMultiplier;
            voxelGIDataUBO.specularMultiplier = scene.voxelGIConfig.specularMultiplier;
        });
        gpuSystem.bufferDestroy(voxelGIDataBuffer);

        struct VoxelGIMatrixesUBO {
            Mat4 projectionView[3];
            Mat4 invProjectionView[3];
        };
        GPU::BufferDesc voxelGIMatrixesBufferDesc;
        voxelGIMatrixesBufferDesc.typeSize = sizeof(VoxelGIMatrixesUBO);
        voxelGIMatrixesBufferDesc.typeAlignment = alignof(VoxelGIMatrixesUBO);
        voxelGIMatrixesBufferDesc.count = 1;
        voxelGIMatrixesBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
        voxelGIMatrixesBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
        GPU::BufferID voxelGIMatrixesBuffer = gpuSystem.bufferCreate(voxelGIMatrixesBufferDesc,
        [&scene](int i, byte* data) -> void {
            VoxelGIMatrixesUBO& voxelGIMatrixesUBO = *(VoxelGIMatrixesUBO*) data;

			float voxelFrustumHalfSpan = scene.voxelGIConfig.halfSpan;
			Mat4 projection = mat4Ortho(
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan,
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan,
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan);
        	
            Vec3f voxelFrustumCenter = scene.voxelGIConfig.center;
            Mat4 view[3];
            view[0] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
            view[1] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f));
            view[2] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, 1.0f, 0.0f));

            for (int i = 0; i < 3; i++) {
                voxelGIMatrixesUBO.projectionView[i] = mat4Transpose(projection * view[i]);
                voxelGIMatrixesUBO.invProjectionView[i] = mat4Transpose(mat4Inverse(projection * view[i]));
            }
        });
        gpuSystem.bufferDestroy(voxelGIMatrixesBuffer);

		GPU::TextureNodeID stubTextureNodeID = renderGraph.importTexture("Stub Texture", stubTexture);

		GPU::BufferNodeID materialNodeID = renderGraph.importBuffer("Material Buffer", scene.materialBuffer);
		GPU::BufferNodeID modelNodeID = renderGraph.importBuffer("Model Buffer", modelBuffer);
		GPU::BufferNodeID rotationNodeID = renderGraph.importBuffer("Rotate Buffer", rotationBuffer);
		GPU::BufferNodeID cameraNodeID = renderGraph.importBuffer("Camera buffer", cameraBuffer);
		GPU::BufferNodeID lightNodeID = renderGraph.importBuffer("Light buffer", lightBuffer);
		GPU::BufferNodeID shadowMatrixesNodeID = renderGraph.importBuffer("Shadow Matrixes buffer", shadowMatrixesBuffer);
		GPU::BufferNodeID voxelGIDataNodeID = renderGraph.importBuffer("Voxel GI Data buffer", voxelGIDataBuffer);
		GPU::BufferNodeID voxelGIMatrixesNodeID = renderGraph.importBuffer("Voxel GI Matrixes buffer", voxelGIMatrixesBuffer);

		Array<GPU::BufferNodeID> vertexBufferNodeIDs;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Create vertex buffer node ids");
			vertexBufferNodeIDs.reserve(scene.meshes.size());
			for (const Mesh& mesh : scene.meshes) {
				vertexBufferNodeIDs.add(renderGraph.importBuffer("Vertex buffer", mesh.vertexBufferID));
			}
		}

		Array<GPU::BufferNodeID> indexBufferNodeIDs;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Create index buffer node ids");
			indexBufferNodeIDs.reserve(scene.meshes.size());
			for (const Mesh& mesh : scene.meshes) {
				indexBufferNodeIDs.add(renderGraph.importBuffer("Index buffer", mesh.indexBufferID));
			}
		}

		GPU::RGTextureDesc shadowMapTexDesc;
		shadowMapTexDesc.width = DirectionalLight::SHADOW_MAP_RESOLUTION;
		shadowMapTexDesc.height = DirectionalLight::SHADOW_MAP_RESOLUTION;
		shadowMapTexDesc.depth = 1;
		shadowMapTexDesc.clear = true;
		shadowMapTexDesc.clearValue.depthStencil = { 1.0f, 0 };
		shadowMapTexDesc.format = GPU::TextureFormat::DEPTH32F;
		shadowMapTexDesc.mipLevels = 1;
		shadowMapTexDesc.type = GPU::TextureType::D2;
		GPU::TextureNodeID shadowMapNodeID = renderGraph.createTexture("Shadow Map", shadowMapTexDesc);

		ShadowMapGenRenderModule::Parameter shadowMapGenData;
		shadowMapGenData.shadowMatrixesBuffer = shadowMatrixesNodeID;
		shadowMapGenData.modelBuffer = modelNodeID;
		shadowMapGenData.depthTarget = shadowMapNodeID;
		shadowMapGenData.vertexBuffers = vertexBufferNodeIDs;
		shadowMapGenData.indexBuffers = indexBufferNodeIDs;
		shadowMapGenData = shadowMapGenRenderModule.addPass(&gpuSystem, &renderGraph, shadowMapGenData, scene);

		GBufferGenRenderModule::Parameter gBufferGenParam;
		gBufferGenParam.vertexBuffers = vertexBufferNodeIDs;
		gBufferGenParam.indexBuffers = indexBufferNodeIDs;
		gBufferGenParam.sceneTextures = sceneTextureNodeIDs;
		gBufferGenParam.camera = cameraNodeID;
		gBufferGenParam.light = lightNodeID;
		gBufferGenParam.material = materialNodeID;
		gBufferGenParam.model = modelNodeID;
        GPU::RGTextureDesc renderTargetDesc;
        renderTargetDesc.width = sceneResolution.x;
        renderTargetDesc.height = sceneResolution.y;
        renderTargetDesc.depth = 1;
        renderTargetDesc.clear = true;
        renderTargetDesc.clearValue = {};
        renderTargetDesc.format = GPU::TextureFormat::RGBA8;
        renderTargetDesc.mipLevels = 1;
        renderTargetDesc.type = GPU::TextureType::D2;
        for (GPU::TextureNodeID& nodeID : gBufferGenParam.renderTargets) {
            nodeID = renderGraph.createTexture("Render Target", renderTargetDesc);
        }
        GPU::RGTextureDesc sceneDepthTexDesc;
        sceneDepthTexDesc.width = sceneResolution.x;
        sceneDepthTexDesc.height = sceneResolution.y;
        sceneDepthTexDesc.depth = 1;
        sceneDepthTexDesc.clear = true;
        sceneDepthTexDesc.clearValue.depthStencil = { 1.0f, 0 };
        sceneDepthTexDesc.format = GPU::TextureFormat::DEPTH32F;
        sceneDepthTexDesc.mipLevels = 1;
        sceneDepthTexDesc.type = GPU::TextureType::D2;
        gBufferGenParam.depthTarget = renderGraph.createTexture("Depth target", sceneDepthTexDesc);
        gBufferGenParam.shadowMap = shadowMapGenData.depthTarget;
        gBufferGenParam.stubTexture = stubTextureNodeID;
        gBufferGenRenderModule.addPass(&gpuSystem, &renderGraph, gBufferGenParam, scene);

        VoxelizeRenderModule::Parameter voxelizeParam;
        voxelizeParam.stubTexture = stubTextureNodeID;
        voxelizeParam.vertexBuffers = vertexBufferNodeIDs;
        voxelizeParam.indexBuffers = indexBufferNodeIDs;
        voxelizeParam.model = modelNodeID;
        voxelizeParam.rotation = rotationNodeID;
        voxelizeParam.voxelGIData = voxelGIDataNodeID;
		GPU::RGTextureDesc voxelTargetTexDesc;
		voxelTargetTexDesc.width = scene.voxelGIConfig.resolution;
		voxelTargetTexDesc.height = scene.voxelGIConfig.resolution;
		voxelTargetTexDesc.depth = scene.voxelGIConfig.resolution;
		voxelTargetTexDesc.clear = true;
		voxelTargetTexDesc.clearValue.color.uint32 = {};
		voxelTargetTexDesc.format = GPU::TextureFormat::RGBA8UI;
		voxelTargetTexDesc.mipLevels = 1;
		voxelTargetTexDesc.type = GPU::TextureType::D3;
		voxelizeParam.voxelAlbedo = renderGraph.createTexture("Voxel Albedo Target", voxelTargetTexDesc);
		voxelizeParam.voxelEmissive = renderGraph.createTexture("Voxel Emissive target", voxelTargetTexDesc);
		voxelizeParam.voxelNormal = renderGraph.createTexture("Voxel Normal target", voxelTargetTexDesc);
		voxelizeParam.material = materialNodeID;
		voxelizeParam.materialTextures = sceneTextureNodeIDs;
		voxelizeParam.voxelizeMatrixes = voxelGIMatrixesNodeID;
		voxelizeParam = voxelizeRenderModule.addPass(&gpuSystem, &renderGraph, voxelizeParam, scene);

        VoxelLightInjectRenderModule::Parameter voxelInjectParam;
        voxelInjectParam.voxelAlbedo = voxelizeParam.voxelAlbedo;
        voxelInjectParam.voxelNormal = voxelizeParam.voxelNormal;
        voxelInjectParam.voxelEmissive = voxelizeParam.voxelEmissive;
        GPU::RGTextureDesc voxelLightTexDesc = voxelTargetTexDesc;
        voxelLightTexDesc.format = GPU::TextureFormat::RGBA16F;
        voxelLightTexDesc.mipLevels = int(log2f(scene.voxelGIConfig.resolution));
        voxelInjectParam.voxelLight = renderGraph.createTexture("Voxel light Target", voxelLightTexDesc);
        voxelInjectParam.voxelGIData = voxelGIDataNodeID;
        voxelInjectParam.lightData = lightNodeID;
        voxelInjectParam = voxelLightInjectRenderModule.addPass(&gpuSystem, &renderGraph, voxelInjectParam, scene);


        VoxelGIDebugRenderModule::Parameter voxelGIDebugParam;
        voxelGIDebugParam.renderTarget = renderGraph.createTexture("Voxel GI color target", renderTargetDesc);
        voxelGIDebugParam.depthTarget = renderGraph.createTexture("Voxel GI depth target", sceneDepthTexDesc);
        voxelGIDebugParam.voxelGIData = voxelGIDataNodeID;
        voxelGIDebugParam.cameraData = cameraNodeID;
        voxelGIDebugParam.voxelAlbedo = voxelizeParam.voxelAlbedo;
        voxelGIDebugParam.voxelEmissive = voxelizeParam.voxelEmissive;
        voxelGIDebugParam.voxelNormal = voxelizeParam.voxelNormal;
        voxelGIDebugParam.voxelLight = voxelInjectParam.voxelLight;
        voxelGIDebugParam = voxelGIDebugRenderModule.addPass(&gpuSystem, &renderGraph, voxelGIDebugParam, scene);

		GPU::BufferNodeID quadBufferNodeID = renderGraph.importBuffer("Quad Buffer", quadBuffer);
		FinalGatherRenderModule::Parameter finalGatherParameter;
		for (int i = 0; i < 4; i++) {
		    finalGatherParameter.renderMap[i] = gBufferGenParam.renderTargets[i];
		}
		finalGatherParameter.renderTarget = renderGraph.createTexture("Final Gather Render Target", renderTargetDesc);
		finalGatherParameter.vertexBuffer = quadBufferNodeID;
        finalGatherParameter = finalGatherRenderModule.addPass(&gpuSystem, &renderGraph, finalGatherParameter, sceneResolution);

		// Start the Dear ImGui frame
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		GPU::TextureNodeID imguiFontNodeID = renderGraph.
			importTexture("Imgui Font", imguiRenderModule.getFontTexture());
		ImGui::GetIO().Fonts->TexID = SoulImTexture(imguiFontNodeID).getImTextureID();

		ImGui::SetNextWindowPos(ImVec2(0, 20));
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - 20));
		ImGuiWindowFlags leftDockWindowFlags = ImGuiWindowFlags_None;
		leftDockWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		leftDockWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::Begin("Left Dock", nullptr, leftDockWindowFlags);
		ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
		dockspaceFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
		ImGuiID dockspace_id = ImGui::GetID("Left Dock");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);

		if (ImGui::Begin("Scene Configuration"))
		{
			if (ImGui::CollapsingHeader("Directional Light")) {
                ImGui::SliderFloat3("Direction", (float *) &scene.dirLight.direction, -1.0f, 1.0f);
                ImGui::InputFloat3("Color", (float *) &scene.dirLight.color);
                ImGui::InputFloat("Illuminance", (float *) &scene.dirLight.illuminance);
                ImGui::InputFloat("Bias", &scene.dirLight.bias);
                ImGui::InputFloat3("Cascade split", (float *) &scene.dirLight.split);
            }

            if (ImGui::CollapsingHeader("Voxel GI")) {
                Scene::VoxelGIConfig& voxelGIConfig = scene.voxelGIConfig;
                ImGui::InputFloat3("Center", (float*) &voxelGIConfig.center);
                ImGui::InputFloat("Half Span", (float*)&voxelGIConfig.halfSpan);
                ImGui::InputFloat("Bias", (float*)&voxelGIConfig.bias);
                ImGui::InputFloat("Diffuse Multiplier", (float*)&voxelGIConfig.diffuseMultiplier);
                ImGui::InputFloat("Specular Multiplier", (float*)&voxelGIConfig.specularMultiplier);
                ImGui::InputInt("Resolution", (int*)&voxelGIConfig.resolution);
            }

			ImGui::End();
		}

		scenePanel.update("Scene", SoulImTexture(finalGatherParameter.renderTarget).getImTextureID());
		scenePanel.update("Voxel", SoulImTexture(voxelGIDebugParam.renderTarget).getImTextureID());
		ImGui::End();

		{
			SOUL_PROFILE_ZONE_WITH_NAME("Render Imgui");
			ImGui::Render();
		}
		imguiRenderModule.addPass(&gpuSystem, &renderGraph, *ImGui::GetDrawData(), gpuSystem.getSwapchainTexture());

		gpuSystem.renderGraphExecute(renderGraph);

		gpuSystem.frameFlush();
		renderGraph.cleanup();

		
		if (ImGui::IsMouseDown(2)) {
			static float translationSpeed = 1.0f;

			float cameraSpeedInc = 0.1f;
			translationSpeed += (cameraSpeedInc * translationSpeed * io.MouseWheel);


			if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
				translationSpeed *= 0.9f;
			}
			if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
				translationSpeed *= 1.1f;
			}

			if (ImGui::IsMouseDragging(2)) {

				Soul::Vec3f cameraRight = cross(camera.up, camera.direction) * -1.0f;

				{
					Soul::Mat4 rotate = mat4Rotate(cameraRight, -2.0f * io.MouseDelta.y / camera.viewportHeight * Soul::PI);
					camera.direction = rotate * camera.direction;
					camera.up = rotate * camera.up;
				}

				{
					Soul::Mat4 rotate = mat4Rotate(Soul::Vec3f(0.0f, 1.0f, 0.0f), -2.0f * io.MouseDelta.x / camera.viewportWidth * Soul::PI);

					if (camera.direction != Soul::Vec3f(0.0f, 1.0f, 0.0f))
						camera.direction = rotate * camera.direction;
					if (camera.up != Soul::Vec3f(0.0f, 1.0f, 0.0f))
						camera.up = rotate * camera.up;
				}

			}

			Soul::Vec3f right = Soul::unit(cross(camera.direction, camera.up));
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				camera.position += Soul::unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				camera.position -= Soul::unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				camera.position -= Soul::unit(right) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				camera.position += Soul::unit(right) * translationSpeed;
			}
		}
		camera.view = mat4View(camera.position, camera.position + camera.direction, camera.up);
		scene.dirLight.updateShadowMatrixes(camera);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
