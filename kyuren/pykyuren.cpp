#include "pykyuren.h"
#include <GLFW/glfw3.h>

#include "core/type.h"
#include "core/dev_util.h"

#include "runtime/runtime.h"
#include "gpu/gpu.h"
#include <vector>
#include <fstream>

#include <memory/allocators/page_allocator.h>
#include <chrono>
#include "data.h"

#include "blenkyu/depsgraph.h"
#include "blenkyu/object.h"
#include "blenkyu/mesh.h"
#include "blenkyu/light.h"

#include "render_jobs/lighting_render_job.h"
#include "render_jobs/shadow_map_render_job.h"
#include "render_jobs/scene_render_job.h"

inline char* LoadFile(const char* filepath) {
	FILE* f = fopen(filepath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	char* string = (char*)malloc(fsize + 1);
	fread(string, 1, fsize, f);
	fclose(f);

	string[fsize] = 0;

	return string;
}

const int KYUREN_ID_NULL = -1;

typedef struct {
	// The following is expanded form of the macro BASE_MATH_MEMBERS(matrix)
	PyObject_VAR_HEAD float* matrix;  // The only thing we are interested in
	PyObject* cb_user;
	unsigned char cb_type;
	unsigned char cb_subtype;
	unsigned char flag;

	unsigned short num_col;
	unsigned short num_row;
} MatrixObject;

using namespace Soul;
using IDMap = Soul::UInt64HashMap<KyuRen::ResourceID>;
struct Session {
	uint32 id;
    GLFWwindow* window;
	Soul::GPU::System* gpuSystem;

	Memory::MallocAllocator mallocAllocator;
	Runtime::DefaultAllocator defaultAllocator;
	Memory::PageAllocator pageAllocator;
	Memory::LinearAllocator linearAllocator;
	Runtime::TempAllocator tempAllocator;

	KyuRen::Scene* scene;
	KyuRen::RenderPipeline* renderPipeline;
	IDMap* idMap;

	Session() : 
		id(1234),
		mallocAllocator("Default"), 
		defaultAllocator(&mallocAllocator,
			Runtime::DefaultAllocatorProxy(
				Memory::CounterProxy(),
				Memory::ClearValuesProxy(0xFA, 0xFF),
				Memory::BoundGuardProxy())),
		pageAllocator("Page Allocator"), 
		linearAllocator("Main Thread Temp Allocator", 10 * ONE_MEGABYTE, &pageAllocator), 
		tempAllocator(&linearAllocator,
			Runtime::TempProxy()) {}
};

void glfwPrintErrorCallback(int code, const char* message)
{
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

PyObject* on_register_func(PyObject*, PyObject* args, PyObject* keywds) {
	
	PyObject* pyObjectRNA;
	PyObject* pyMeshRNA;
	PyObject* pyDepsgraphRNA;

	static char* kwlist[] = { "objectRNA", "meshRNA", "depsgraphRNA", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, keywds, "OOO", kwlist,
		&pyObjectRNA, &pyMeshRNA, &pyDepsgraphRNA))
		return NULL;

	Blender::StructRNA* objectRNA = (Blender::StructRNA*) PyLong_AsVoidPtr(pyObjectRNA);
	Blender::StructRNA* meshRNA = (Blender::StructRNA*) PyLong_AsVoidPtr(pyMeshRNA);
	Blender::StructRNA* depsgraphRNA = (Blender::StructRNA*) PyLong_AsVoidPtr(pyDepsgraphRNA);
	
	BlenKyu::Depsgraph::Init(depsgraphRNA);
	BlenKyu::Mesh::Init(meshRNA);

	Py_RETURN_NONE;
}

PyObject* init_func(PyObject* /*self*/, PyObject* args)
{
    Session* session = new Session();

    SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

    glfwSetErrorCallback(glfwPrintErrorCallback);
    SOUL_ASSERT(0, glfwInit(), "GLFW Init Failed !");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    SOUL_LOG_INFO("GLFW initialization sucessful");

    SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw");

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	SOUL_ASSERT(0, mode != nullptr, "Mode cannot be nullpointer");
	session->window = glfwCreateWindow(2, 2, "Vulkan", nullptr, nullptr);
	glfwHideWindow(session->window);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	Runtime::Init({
		0,
		4096,
		&session->tempAllocator,
		20 * ONE_MEGABYTE,
		&session->defaultAllocator
	});

	GPU::System* gpuSystem = Runtime::Create<GPU::System>(Runtime::GetContextAllocator());
	GPU::System::Config config = {};
	config.windowHandle = session->window;
	config.swapchainWidth = mode->width;
	config.swapchainHeight = mode->height;
	config.maxFrameInFlight = 3;
	config.threadCount = Runtime::ThreadCount();

	gpuSystem->init(config);

	session->gpuSystem = gpuSystem;

	session->scene = Runtime::Create<KyuRen::Scene>();

	SOUL_LOG_INFO("Create pipeline");
	KyuRen::RenderPipeline* renderPipeline = Runtime::Create<KyuRen::RenderPipeline>(gpuSystem);
	session->renderPipeline = renderPipeline;

	SOUL_LOG_INFO("Create render job");
	KyuRen::RenderJob* sceneRenderJob = Runtime::Create<SceneRenderJob>();
	sceneRenderJob->init(session->scene, gpuSystem);

	SOUL_LOG_INFO("Create shadow map render job");
	KyuRen::RenderJob* shadowMapRenderJob = Runtime::Create<ShadowMapRenderJob>();
	shadowMapRenderJob->init(session->scene, gpuSystem);
	KyuRen::RenderJob* lightingRenderJob = Runtime::Create<LightingRenderJob>();
	lightingRenderJob->init(session->scene, gpuSystem);

	SOUL_LOG_INFO("Render Pipeline add renderjob");
	KyuRen::RenderJobID sceneRenderJobID = renderPipeline->addJob(sceneRenderJob);
	KyuRen::RenderJobID shadowMapRenderJobID = renderPipeline->addJob(shadowMapRenderJob);
	KyuRen::RenderJobID lightingRenderJobID = renderPipeline->addJob(lightingRenderJob);

	SOUL_LOG_INFO("connect");
	renderPipeline->connect(sceneRenderJobID, "modelBuffer", shadowMapRenderJobID, "model");
	renderPipeline->connect(sceneRenderJobID, "posVertexBuffers", shadowMapRenderJobID, "posVertexBuffers");
	renderPipeline->connect(sceneRenderJobID, "indexBuffers", shadowMapRenderJobID, "indexBuffers");

	renderPipeline->connect(sceneRenderJobID, "posVertexBuffers", lightingRenderJobID, "posVertexBuffers");
	renderPipeline->connect(sceneRenderJobID, "norVertexBuffers", lightingRenderJobID, "norVertexBuffers");
	renderPipeline->connect(sceneRenderJobID, "indexBuffers", lightingRenderJobID, "indexBuffers");
	renderPipeline->connect(sceneRenderJobID, "modelBuffer", lightingRenderJobID, "modelBuffer");
	renderPipeline->connect(sceneRenderJobID, "sceneBuffer", lightingRenderJobID, "sceneBuffer");
	renderPipeline->connect(shadowMapRenderJobID, "shadowMap", lightingRenderJobID, "shadowMap");

	renderPipeline->setOutput(lightingRenderJobID, "renderTarget");

	SOUL_LOG_INFO("Render Pipeline Compile");
	renderPipeline->compile();

	session->idMap = Runtime::Create<Soul::UInt64HashMap<KyuRen::ResourceID>>(Runtime::GetContextAllocator());

    return PyLong_FromVoidPtr(session);
}

PyObject* update_mesh_func(PyObject*, PyObject* args) {
	PyObject* pySession;
	PyObject* pyObject;
	PyObject* pyMesh;

	if (!PyArg_ParseTuple(args, "OOO", &pySession, &pyObject, &pyMesh)) {
		return NULL;
	}

	Session* session = (Session*)PyLong_AsVoidPtr(pySession);
	KyuRen::Scene* scene = session->scene;
	GPU::System* gpuSystem = session->gpuSystem;

	BlenKyu::Object blenKyuObj(PyLong_AsVoidPtr(pyObject));
	BlenKyu::Mesh blenKyuMesh(PyLong_AsVoidPtr(pyMesh));

	if (session->idMap->isExist(blenKyuObj.id())) {
		KyuRen::ResourceID kyurenID = (*session->idMap)[blenKyuObj.id()];
		const KyuRen::Mesh& mesh = scene->meshes[kyurenID.index()];
		gpuSystem->bufferDestroy(mesh.posVertexBufferID);
		gpuSystem->bufferDestroy(mesh.norVertexBufferID);
		gpuSystem->bufferDestroy(mesh.indexBufferID);
		scene->meshes.remove(kyurenID.index());
	}

	BlenKyu::MeshVertexList vertexList = blenKyuMesh.vertices();

	GPU::BufferID posVertexBufferID = gpuSystem->bufferCreate(
		{ vertexList.count(), sizeof(Vec3f), alignof(Vec3f), GPU::BUFFER_USAGE_VERTEX_BIT, GPU::QUEUE_GRAPHIC_BIT },
		[vertexList](int index, byte* data)
		{
			auto posData = (Vec3f*)data;
			const BlenKyu::Vertex& mvert = vertexList[index];
			*posData = mvert.pos;
		});

	GPU::BufferID norVertexBufferID = gpuSystem->bufferCreate(
		{ vertexList.count(), sizeof(Vec3f), alignof(Vec3f), GPU::BUFFER_USAGE_VERTEX_BIT, GPU::QUEUE_GRAPHIC_BIT },
		[vertexList](int index, byte* data)
		{
			auto norData = (Vec3f*)data;
			const BlenKyu::Vertex& mvert = vertexList[index];
			*norData = unit(mvert.normal);
		});

	BlenKyu::MeshIndexList indexList = blenKyuMesh.indexes();
	GPU::BufferID indexBufferID = gpuSystem->bufferCreate(
		{ indexList.count(), sizeof(uint32), alignof(uint32), GPU::BUFFER_USAGE_INDEX_BIT, GPU::QUEUE_GRAPHIC_BIT },
		[indexList](int index, byte* data) {
			auto indexData = (uint32*)data;
			*indexData = indexList[index];
		});

	PackedID meshID = session->scene->meshes.add({ posVertexBufferID, norVertexBufferID, vertexList.count(), indexBufferID, indexList.count() });

	session->idMap->add(blenKyuObj.id(), KyuRen::ResourceID(uint8(KyuRen::ResourceType::MESH), meshID));

	Py_RETURN_NONE;
}

PyObject* update_light_func(PyObject*, PyObject* args) {
	PyObject* pySession;
	PyObject* pyObject;
	PyObject* pyLight;

	if (!PyArg_ParseTuple(args, "OOO", &pySession, &pyObject, &pyLight)) {
		return NULL;
	}

	Session* session = (Session*) PyLong_AsVoidPtr(pySession);
	BlenKyu::Object blenKyuObj(PyLong_AsVoidPtr(pyObject));
	BlenKyu::Light blenKyuLight(PyLong_AsVoidPtr(pyLight));

	switch (blenKyuLight.type()) {
	case BlenKyu::LightType::SUNLIGHT: {
		PackedID lightID = session->scene->sunLights.add(blenKyuLight.sunLight());
		session->idMap->add(blenKyuObj.id(), KyuRen::ResourceID(uint8(KyuRen::ResourceType::SUNLIGHT), lightID));
		break;
	}
	default:
		SOUL_LOG_INFO("Light type unknown");
		break;
	}

	Py_RETURN_NONE;
}

PyObject* sync_depsgraph_func(PyObject*, PyObject* args) {
	PyObject* pySession;
	PyObject* pyDepsgraph;
	if (!PyArg_ParseTuple(args, "OO", &pySession, &pyDepsgraph)) {
		SOUL_LOG_INFO("Parse error");
		return NULL;
	}

	Session* session = (Session*)PyLong_AsVoidPtr(pySession);
	KyuRen::Scene* scene = (KyuRen::Scene*) session->scene;

	scene->meshEntities.clear();
	scene->sunLightEntities.clear();
	IDMap* idMap = session->idMap;
	BlenKyu::Depsgraph depsgraph(PyLong_AsVoidPtr(pyDepsgraph));
	depsgraph.forEachObjectInstance([scene, idMap](BlenKyu::Depsgraph::Instance instance) {
		if (idMap->isExist(instance.obj->id())) {
			KyuRen::ResourceID kyurenID = (*idMap)[instance.obj->id()];
			switch (kyurenID.type()) {
			case uint8(KyuRen::ResourceType::MESH):
				scene->meshEntities.add({ mat4Transpose(mat4(instance.matrixWorld)), kyurenID.index() });
				break;
			case uint8(KyuRen::ResourceType::SUNLIGHT): {
				KyuRen::SunLightEntity entity;
				entity.worldMatrix = mat4Transpose(mat4(instance.matrixWorld));
				entity.sunLightID = kyurenID.index();
				scene->sunLightEntities.add(entity);
				break;
			}
			default:
				break;
			}
		}
	});

	Py_RETURN_NONE;
}

PyObject* create_mesh_entity_func(PyObject*, PyObject* args) {
	PyObject* pySession;
	MatrixObject* matrix;

	uint32 meshID;
	if (!PyArg_ParseTuple(args, "OOI", &pySession, &matrix, &meshID)) {
		SOUL_LOG_INFO("Parse error");
		return NULL;
	}

	Session* session = (Session*)PyLong_AsVoidPtr(pySession);
	KyuRen::Scene* scene = session->scene;

	scene->meshEntities.add({ mat4Transpose(mat4(matrix->matrix)), meshID });

	Py_RETURN_NONE;
}

PyObject* clear_mesh_entities_func(PyObject*, PyObject* args) {
	PyObject* pySession;
	if (!PyArg_ParseTuple(args, "O", &pySession)) {
		return NULL;
	}
	Session* session = (Session*)PyLong_AsVoidPtr(pySession);
	KyuRen::Scene* scene = session->scene;
	scene->meshEntities.clear();

	Py_RETURN_NONE;
}

PyObject* set_perspective_camera_func(PyObject*, PyObject* args) {
	
	PyObject* pySession;
	MatrixObject* viewMatrix;
	MatrixObject* projectionMatrix;
	if (!PyArg_ParseTuple(args, "OOO", &pySession, &viewMatrix, &projectionMatrix)) {
		return NULL;
	}

	Session* session = (Session*)PyLong_AsVoidPtr(pySession);
	KyuRen::Scene* scene = session->scene;
	KyuRen::Camera& camera = scene->camera;
	camera.type = KyuRen::PERSPECTIVE;
	camera.viewMatrix = mat4Transpose(mat4(viewMatrix->matrix));
	camera.projectionMatrix = mat4Transpose(mat4(projectionMatrix->matrix));

	Soul::Mat4 invViewMatrix = mat4Inverse(camera.viewMatrix);
	camera.origin = invViewMatrix * Vec3f(0, 0, 0);
	camera.up = invViewMatrix * Vec3f(0, 1, 0);
	camera.target = invViewMatrix * Vec3f(0, 0, -1);

	camera.perspective.fov = 2 * atan(1 / camera.projectionMatrix.elem[1][1]);
	camera.perspective.aspectRatio = camera.projectionMatrix.elem[1][1] / camera.projectionMatrix.elem[0][0];
	camera.perspective.zNear = camera.projectionMatrix.elem[2][3] / (camera.projectionMatrix.elem[2][2] - 1.0f);
	camera.perspective.zFar = camera.projectionMatrix.elem[2][3] / (camera.projectionMatrix.elem[2][2] + 1.0f);

	SOUL_LOG_INFO("Camera zfar = %f", camera.perspective.zFar);

	Py_RETURN_NONE;
}

PyObject* draw_func(PyObject*, PyObject* args) 
{
	SOUL_PROFILE_FRAME();
	SOUL_PROFILE_ZONE_WITH_NAME("Draw func");

	PyObject* pySession;
	int width, height;
	if (!PyArg_ParseTuple(args, "Oii", &pySession, &width, &height))
		return NULL;

	Session* session = (Session*)PyLong_AsVoidPtr(pySession);
	KyuRen::Scene* scene = session->scene;
	GPU::System* gpuSystem = session->gpuSystem;

	SOUL_LOG_INFO("Mesh entity count = %d, mesh count = %d", scene->meshEntities.size(), scene->meshes.size());
	Runtime::System::Get().beginFrame();

	scene->camera.viewDim = { uint32(width), uint32(height) };

	char* pixels = (char*)Runtime::GetTempAllocator()->allocate(width * height * sizeof(char) * 4, alignof(char), "Pixel Buffer").addr;

	SOUL_LOG_INFO("Render pipeline execute");
	session->renderPipeline->execute(pixels);

	/*GPU::RenderGraph renderGraph;

	GPU::RGTextureDesc renderTargetDesc;
	renderTargetDesc.width = width;
	renderTargetDesc.height = height;
	renderTargetDesc.depth = 1;
	renderTargetDesc.clear = true;
	renderTargetDesc.clearValue = {};
	renderTargetDesc.format = GPU::TextureFormat::RGBA8;
	renderTargetDesc.mipLevels = 1;
	renderTargetDesc.type = GPU::TextureType::D2;
	GPU::TextureNodeID renderTargetNodeID = renderGraph.createTexture("Render Targe", renderTargetDesc);

	GPU::RGTextureDesc sceneDepthTexDesc;
	sceneDepthTexDesc.width = width;
	sceneDepthTexDesc.height = height;
	sceneDepthTexDesc.depth = 1;
	sceneDepthTexDesc.clear = true;
	sceneDepthTexDesc.clearValue.depthStencil = { 1.0f, 0 };
	sceneDepthTexDesc.format = GPU::TextureFormat::DEPTH32F;
	sceneDepthTexDesc.mipLevels = 1;
	sceneDepthTexDesc.type = GPU::TextureType::D2;
	GPU::TextureNodeID depthTargetNodeID = renderGraph.createTexture("Depth target", sceneDepthTexDesc);

	struct PassData {
		Array<GPU::BufferNodeID> posVertexBuffers;
		Array<GPU::BufferNodeID> norVertexBuffers;
		Array<GPU::BufferNodeID> indexBuffers;
		GPU::BufferNodeID scene;
		GPU::BufferNodeID model;
		GPU::TextureNodeID renderTarget;
		GPU::TextureNodeID depthTarget;
		GPU::TextureNodeID shadowMap;
	};

	PassData inputData;
	inputData.posVertexBuffers.reserve(scene->meshes.size());
	inputData.indexBuffers.reserve(scene->meshes.size());

	{
		SOUL_PROFILE_ZONE_WITH_NAME("Build input data");
		for (const KyuRen::Mesh& mesh : scene->meshes) {
			inputData.posVertexBuffers.add(renderGraph.importBuffer("Pos Vertex Buffer", mesh.posVertexBufferID));
			inputData.norVertexBuffers.add(renderGraph.importBuffer("Nor Vertex Buffer", mesh.norVertexBufferID));
			inputData.indexBuffers.add(renderGraph.importBuffer("Index Buffer", mesh.indexBufferID));
		}
	}

	inputData.renderTarget = renderTargetNodeID;
	inputData.depthTarget = depthTargetNodeID;

	struct CameraUBO
	{
		Soul::Mat4 projection;
		Soul::Mat4 view;
		Soul::Mat4 projectionView;
		Soul::Mat4 invProjectionView;

		Vec3f position;
		float pad1;
	
	};

	struct SunLightUBO
	{
		Soul::Mat4 shadowMatrix[4];
		Soul::Vec3f direction;
		float bias;
		Soul::Vec3f color;
		float pad1;
		float cascadeDepths[4];
	};

	struct SceneUBO {
		CameraUBO camera;
		SunLightUBO light;
		uint32 sunlightCount;
	};

	CameraUBO cameraDataUBO;
	cameraDataUBO.projection = mat4Transpose(scene->camera.projectionMatrix);
	cameraDataUBO.view = mat4Transpose(scene->camera.viewMatrix);
	Soul::Mat4 projectionView = scene->camera.projectionMatrix * scene->camera.viewMatrix;
	cameraDataUBO.projectionView = mat4Transpose(projectionView);
	cameraDataUBO.invProjectionView = mat4Transpose(mat4Inverse(projectionView));
	cameraDataUBO.position = scene->camera.origin;
	
	SceneUBO sceneDataUBO;
	sceneDataUBO.camera = cameraDataUBO;

	KyuRen::Camera& camera = scene->camera;

	if (scene->meshEntities.size() != 0) {
		Soul::Vec3f position = scene->meshEntities[0].worldMatrix * Vec3f(0, 0, 0);
		SOUL_LOG_INFO("Position ; (%f, %f, %f)", position.x, position.y, position.z);
	}

	if (scene->sunLightEntities.size() != 0) {
		KyuRen::SunLightEntity& entity = scene->sunLightEntities[0];
		entity.updateShadowMatrixes(camera);

		SOUL_LOG_INFO("World matrix sun light : ");
		for (int i = 0; i < 4; i++) {
			SOUL_LOG_INFO("(%f , %f, %f, %f)", entity.worldMatrix.elem[i][0], entity.worldMatrix.elem[i][1], entity.worldMatrix.elem[i][2], entity.worldMatrix.elem[i][3]);
		}

		Soul::Vec3f direction = (entity.worldMatrix * Vec3f(0, 0, 1)) - (entity.worldMatrix * Vec3f(0, 0, 0));
		direction *= -1;
		direction = Soul::unit(direction);

		SOUL_LOG_INFO("Direction : (%f, %f, %f)", direction.x, direction.y, direction.z);

		KyuRen::SunLight& light = scene->sunLights[entity.sunLightID];

		for (int i = 0; i < 4; i++) {
			sceneDataUBO.light.shadowMatrix[i] = mat4Transpose(entity.shadowMatrixes[i]);
		}
		sceneDataUBO.light.direction = direction;
		sceneDataUBO.light.bias = 0.001f;
		sceneDataUBO.light.color = light.color;
		float cameraFar = camera.perspective.zFar;
		float cameraNear = camera.perspective.zNear;
		float cameraDepth = cameraFar - cameraNear;
		for (int j = 0; j < 3; j++) {
			sceneDataUBO.light.cascadeDepths[j] = scene->camera.perspective.zNear + (cameraDepth * entity.split[j]);
		}
		sceneDataUBO.light.cascadeDepths[3] = cameraFar;


		sceneDataUBO.sunlightCount = 1;
	}
	
	GPU::BufferDesc sceneBufferDesc;
	sceneBufferDesc.typeSize = sizeof(SceneUBO);
	sceneBufferDesc.typeAlignment = alignof(SceneUBO);
	sceneBufferDesc.count = 1;
	sceneBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	sceneBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;


	GPU::BufferID sceneBuffer = gpuSystem->bufferCreate(sceneBufferDesc,
		[&sceneDataUBO](int i, byte* data) {
			auto sceneData = (SceneUBO*)data;
			*sceneData = sceneDataUBO;
		});
	gpuSystem->bufferDestroy(sceneBuffer);
	inputData.scene = renderGraph.importBuffer("Scene Buffer", sceneBuffer);

	GPU::BufferDesc modelBufferDesc;
	modelBufferDesc.typeSize = sizeof(Soul::Mat4);
	modelBufferDesc.typeAlignment = alignof(Soul::Mat4);
	modelBufferDesc.count = scene->meshEntities.size();
	modelBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	modelBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::BufferID modelBuffer = gpuSystem->bufferCreate(modelBufferDesc,
		[scene](int i, byte* data)
	{
		auto model = (Soul::Mat4*)data;
		*model = mat4Transpose(
			scene->meshEntities[i].worldMatrix);
	});
	gpuSystem->bufferDestroy(modelBuffer);
	inputData.model = renderGraph.importBuffer("Model Buffer", modelBuffer);

	GPU::BufferDesc shadowMatrixesBufferDesc;
	shadowMatrixesBufferDesc.typeSize = sizeof(Mat4);
	shadowMatrixesBufferDesc.typeAlignment = alignof(Mat4);
	shadowMatrixesBufferDesc.count = 4;
	shadowMatrixesBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	shadowMatrixesBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::BufferID shadowMatrixesBuffer = gpuSystem->bufferCreate(shadowMatrixesBufferDesc,
		[scene](int i, byte* data) -> void {
		auto shadowMatrix = (Mat4*)data;
		if (scene->sunLightEntities.size() > 0) {
			*shadowMatrix = mat4Transpose(scene->sunLightEntities[0].shadowMatrixes[i]);
		}
	});
	gpuSystem->bufferDestroy(shadowMatrixesBuffer);

	GPU::RGTextureDesc shadowMapTexDesc;
	shadowMapTexDesc.width = KyuRen::SunLightEntity::SHADOW_MAP_RESOLUTION;
	shadowMapTexDesc.height = KyuRen::SunLightEntity::SHADOW_MAP_RESOLUTION;
	shadowMapTexDesc.depth = 1;
	shadowMapTexDesc.clear = true;
	shadowMapTexDesc.clearValue.depthStencil = { 1.0f, 0 };
	shadowMapTexDesc.format = GPU::TextureFormat::DEPTH32F;
	shadowMapTexDesc.mipLevels = 1;
	shadowMapTexDesc.type = GPU::TextureType::D2;
	GPU::TextureNodeID shadowMapNodeID = renderGraph.createTexture("Shadow Map", shadowMapTexDesc);

	struct ShadowPassData {
		GPU::BufferNodeID modelBuffer;
		GPU::BufferNodeID shadowMatrixesBuffer;
		Array<GPU::BufferNodeID> posVertexBuffers;
		Array<GPU::BufferNodeID> indexBuffers;
		GPU::TextureNodeID depthTarget;
	} shadowPassData;
	shadowPassData.modelBuffer = inputData.model;
	shadowPassData.shadowMatrixesBuffer = renderGraph.importBuffer("Shadow Matrix", shadowMatrixesBuffer);
	shadowPassData.depthTarget = shadowMapNodeID;
	shadowPassData.posVertexBuffers = inputData.posVertexBuffers;
	shadowPassData.indexBuffers = inputData.indexBuffers;
	GPU::ShaderID shadowVertShaderID = session->shadowVertShaderID;
	GPU::ShaderID shadowFragShaderID = session->shadowFragShaderID;
	for (int i = 0; i < 4; i++) {
		shadowPassData = renderGraph.addGraphicPass<ShadowPassData>(
			"Shadow Pass",
			[i, &shadowPassData, shadowVertShaderID, shadowFragShaderID]
		(GPU::GraphicNodeBuilder* builder, ShadowPassData* data) {
				data->modelBuffer = builder->addInShaderBuffer(shadowPassData.modelBuffer, 3, 0);
				data->shadowMatrixesBuffer = builder->addInShaderBuffer(shadowPassData.shadowMatrixesBuffer, 1, 0);
				for (GPU::BufferNodeID nodeID : shadowPassData.posVertexBuffers)
				{
					data->posVertexBuffers.add(builder->addVertexBuffer(nodeID));
				}

				for (GPU::BufferNodeID nodeID : shadowPassData.indexBuffers)
				{
					data->indexBuffers.add(builder->addIndexBuffer(nodeID));
				}

				GPU::DepthStencilAttachmentDesc depthAttchDesc;
				depthAttchDesc.clear = i == 0;
				depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
				depthAttchDesc.depthWriteEnable = true;
				depthAttchDesc.depthTestEnable = true;
				depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
				data->depthTarget = builder->setDepthStencilAttachment(shadowPassData.depthTarget, depthAttchDesc);


				uint16 resolution = KyuRen::SunLightEntity::SHADOW_MAP_RESOLUTION;
				uint16 halfResolution = resolution / 2;
				struct ViewRegion
				{
					uint16 offsetX, offsetY, width, height;
				};
				static ViewRegion scissorRegions[4] = {
					{0, 0, halfResolution, halfResolution},
					{halfResolution, 0, halfResolution, halfResolution},
					{0, halfResolution, halfResolution, halfResolution},
					{ halfResolution, halfResolution, halfResolution, halfResolution}
				};
				ViewRegion scissorRegion = scissorRegions[i];
				GPU::GraphicPipelineConfig pipelineConfig;
				pipelineConfig.viewport = { 0, 0, resolution, resolution };
				pipelineConfig.scissor = { false,  scissorRegion.offsetX, scissorRegion.offsetY, scissorRegion.width, scissorRegion.height };
				pipelineConfig.framebuffer = { uint16(resolution), uint16(resolution) };
				pipelineConfig.vertexShaderID = shadowVertShaderID;
				pipelineConfig.fragmentShaderID = shadowFragShaderID;
				pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

				builder->setPipelineConfig(pipelineConfig);
			},
			[scene, i]
			(GPU::RenderGraphRegistry* registry, const ShadowPassData& data, GPU::CommandBucket* commandBucket) {
				GPU::Descriptor set1Descriptor = {};
				set1Descriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
				set1Descriptor.uniformInfo.bufferID = registry->getBuffer(data.shadowMatrixesBuffer);
				set1Descriptor.uniformInfo.unitIndex = i;
				GPU::ShaderArgSetDesc set1Desc;
				set1Desc.bindingCount = 1;
				set1Desc.bindingDescriptions = &set1Descriptor;
				GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, set1Desc);

				commandBucket->reserve(scene->meshEntities.size());

				for (int j = 0; j < scene->meshEntities.size(); j++)
				{
					GPU::Descriptor set3Descriptor = {};
					set3Descriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
					set3Descriptor.uniformInfo.bufferID = registry->getBuffer(data.modelBuffer);
					set3Descriptor.uniformInfo.unitIndex = j;
					GPU::ShaderArgSetDesc set3Desc;
					set3Desc.bindingCount = 1;
					set3Desc.bindingDescriptions = &set3Descriptor;
					GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, set3Desc);

					const KyuRen::MeshEntity& meshEntity = scene->meshEntities[j];
					uint32 meshInternalID = scene->meshes.getInternalID(meshEntity.meshID);
					const KyuRen::Mesh& mesh = scene->meshes[meshEntity.meshID];
					GPU::BufferNodeID posVertexBufferNode = data.posVertexBuffers[meshInternalID];

					using DrawCommand = GPU::Command::DrawIndex2;
					DrawCommand* command = commandBucket->put<DrawCommand>(j, j);

					command->vertexBufferIDs[0] = registry->getBuffer(posVertexBufferNode);
					command->vertexCount = 1;
					command->indexBufferID = registry->getBuffer(data.indexBuffers[meshInternalID]);
					command->indexCount = mesh.indexCount;
					command->shaderArgSets[1] = set1;
					command->shaderArgSets[3] = set3;
				}
			}
			);
	}
	
	GPU::ShaderID vertShaderID = session->vertShaderID;
	GPU::ShaderID fragShaderID = session->fragShaderID;
	inputData.shadowMap = shadowPassData.depthTarget;
	renderGraph.addGraphicPass<PassData>(
		"Rectangle Render Pass",
		[&inputData, vertShaderID, fragShaderID, width, height]
		(GPU::GraphicNodeBuilder* builder, PassData* data) {
			SOUL_PROFILE_ZONE_WITH_NAME("Setup rectangle render pass");
			for (GPU::BufferNodeID nodeID : inputData.posVertexBuffers) {
				data->posVertexBuffers.add(builder->addVertexBuffer(nodeID));
			}

			for (GPU::BufferNodeID nodeID : inputData.norVertexBuffers) {
				data->norVertexBuffers.add(builder->addVertexBuffer(nodeID));
			}

			for (GPU::BufferNodeID nodeID : inputData.indexBuffers) {
				data->indexBuffers.add(builder->addIndexBuffer(nodeID));
			}

			GPU::ColorAttachmentDesc colorAttchDesc;
			colorAttchDesc.blendEnable = false;
			colorAttchDesc.clear = true;
			colorAttchDesc.clearValue.color.float32 = {0, 0, 0, 0};
			data->renderTarget = builder->addColorAttachment(inputData.renderTarget, colorAttchDesc);

			GPU::DepthStencilAttachmentDesc depthAttchDesc;
			depthAttchDesc.clear = true;
			depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
			depthAttchDesc.depthWriteEnable = true;
			depthAttchDesc.depthTestEnable = true;
			depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
			data->depthTarget = builder->setDepthStencilAttachment(inputData.depthTarget, depthAttchDesc);

			data->scene = builder->addInShaderBuffer(inputData.scene, 0, 0);
			data->model = builder->addInShaderBuffer(inputData.model, 1, 0);
			data->shadowMap = builder->addInShaderTexture(inputData.shadowMap, 0, 1);

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
		[scene, gpuSystem]
		(GPU::RenderGraphRegistry* registry, const PassData& passData, GPU::CommandBucket* commandBucket) {
			using DrawCommand = GPU::Command::DrawIndex2;
			commandBucket->reserve(scene->meshEntities.size());

			GPU::SamplerDesc samplerDesc = {};
			samplerDesc.minFilter = GPU::TextureFilter::LINEAR;
			samplerDesc.magFilter = GPU::TextureFilter::LINEAR;
			samplerDesc.mipmapFilter = GPU::TextureFilter::LINEAR;
			samplerDesc.wrapU = GPU::TextureWrap::REPEAT;
			samplerDesc.wrapV = GPU::TextureWrap::REPEAT;
			samplerDesc.wrapW = GPU::TextureWrap::REPEAT;
			samplerDesc.anisotropyEnable = false;
			samplerDesc.maxAnisotropy = 0.0f;
			GPU::SamplerID samplerID = gpuSystem->samplerRequest(samplerDesc);

			GPU::Descriptor set0Descriptors[2] = {
				GPU::Descriptor::Uniform(registry->getBuffer(passData.scene), 0),
				GPU::Descriptor::SampledImage(registry->getTexture(passData.shadowMap), samplerID)
			};
			GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, { 2, set0Descriptors });
			{
				SOUL_PROFILE_ZONE_WITH_NAME("Fill command buckets");
				/* struct TaskData {
					GPU::CommandBucket* commandBucket;
					const PassData& passData;
					KyuRen::Scene* scene;
					GPU::ShaderArgSetID set0;
					GPU::RenderGraphRegistry* registry;
				};

				TaskData taskData = {commandBucket, passData, scene, set0, registry};

				Runtime::TaskID renderTaskID= Runtime::ParallelForTaskCreate(0, scene->meshEntities.size(), 32,
					[&taskData](int index) {
					SOUL_PROFILE_ZONE_WITH_NAME("Fill command buckets child");
					GPU::RenderGraphRegistry* registry = taskData.registry;
					GPU::CommandBucket* commandBucket = taskData.commandBucket;
					KyuRen::Scene* scene = taskData.scene;
					GPU::ShaderArgSetID set0 = taskData.set0;
					const PassData& passData = taskData.passData;

					DrawCommand* command = commandBucket->put<DrawCommand>(index, index);
					const KyuRen::MeshEntity& meshEntity = scene->meshEntities[index];
					uint32 meshInternalID = scene->meshes.getInternalId(meshEntity.meshID);
					const KyuRen::Mesh& mesh = scene->meshes[meshEntity.meshID];
					GPU::BufferNodeID posVertexBufferNode = passData.posVertexBuffers[meshInternalID];
					command->vertexBufferIDs[0] = registry->getBuffer(posVertexBufferNode);
					command->vertexCount = 1;
					command->indexBufferID = registry->getBuffer(passData.indexBuffers[meshInternalID]);
					command->indexCount = mesh.indexCount;
					command->shaderArgSets[0] = set0;

					GPU::Descriptor set1Descriptors[1] = {
						GPU::Descriptor::Uniform(registry->getBuffer(passData.model), index)
					};

					GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { 1, set1Descriptors });
					command->shaderArgSets[1] = set1;

				});

				Runtime::RunTask(renderTaskID);
				Runtime::WaitTask(renderTaskID);

				for (int index = 0; index < scene->meshEntities.size(); index++) {
					DrawCommand* command = commandBucket->put<DrawCommand>(index, index);
					const KyuRen::MeshEntity& meshEntity = scene->meshEntities[index];
					uint32 meshInternalID = scene->meshes.getInternalID(meshEntity.meshID);
					const KyuRen::Mesh& mesh = scene->meshes[meshEntity.meshID];
					GPU::BufferNodeID posVertexBufferNode = passData.posVertexBuffers[meshInternalID];
					command->vertexBufferIDs[0] = registry->getBuffer(posVertexBufferNode);
					GPU::BufferNodeID norVertexBufferNode = passData.norVertexBuffers[meshInternalID];
					command->vertexBufferIDs[1] = registry->getBuffer(norVertexBufferNode);

					command->vertexCount = 2;
					command->indexBufferID = registry->getBuffer(passData.indexBuffers[meshInternalID]);
					command->indexCount = mesh.indexCount;
					command->shaderArgSets[0] = set0;

					GPU::Descriptor set1Descriptors[1] = {
						GPU::Descriptor::Uniform(registry->getBuffer(passData.model), index)
					};

					GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { 1, set1Descriptors });
					command->shaderArgSets[1] = set1;
				}

			}

		}
	);

	char* pixels = (char*)Runtime::GetTempAllocator()->allocate(width * height * sizeof(char) * 4, alignof(char), "Pixel Buffer").addr;
	renderGraph.exportTexture(renderTargetNodeID, pixels);

	gpuSystem->renderGraphExecute(renderGraph);
	gpuSystem->frameFlush();
	renderGraph.cleanup(); */

	return PyByteArray_FromStringAndSize(pixels, width * height * 4);
}

PyObject* exit_func(PyObject* /*self*/, PyObject* value)
{
    Session* session = (Session*)PyLong_AsVoidPtr(value);
	
	session->gpuSystem->shutdown();

	Runtime::Destroy(session->gpuSystem);
	Runtime::Destroy(session->scene);
	
	Runtime::Shutdown();
    glfwDestroyWindow(session->window);
    glfwTerminate();

	delete session;

	Py_RETURN_NONE;
}