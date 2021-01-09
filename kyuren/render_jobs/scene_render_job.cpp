#include "scene_render_job.h"
#include "core/array.h"
#include "../utils.h"
#include "gpu/gpu.h"
#include "runtime/runtime.h"

void SceneRenderJob::init(const KyuRen::Scene* scene, Soul::GPU::System* system) {
	_scene = scene;
	_gpuSystem = system;

	SOUL_LOG_INFO("Scene render job init");

	_addOutput({ KyuRen::RenderJobParam::Type::GPUBuffer, "sceneBuffer", "Scene Buffer" });
	_addOutput({ KyuRen::RenderJobParam::Type::GPUBuffer, "modelBuffer", "Model Buffer" });
	_addOutput({ KyuRen::RenderJobParam::Type::GPUBufferArray, "posVertexBuffers", "Position Vertex Buffers" });
	_addOutput({ KyuRen::RenderJobParam::Type::GPUBufferArray, "norVertexBuffers", "Normal Vertex Buffers" });
	_addOutput({ KyuRen::RenderJobParam::Type::GPUBufferArray, "indexBuffers", "Index Buffers" });
}

void SceneRenderJob::execute(Soul::GPU::RenderGraph* renderGraph, const KyuRen::RenderJobInputs& inputs, KyuRen::RenderJobOutputs& outputs) {
	using namespace Soul;

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
	cameraDataUBO.projection = mat4Transpose(_scene->camera.projectionMatrix);
	cameraDataUBO.view = mat4Transpose(_scene->camera.viewMatrix);
	Soul::Mat4 projectionView = _scene->camera.projectionMatrix * _scene->camera.viewMatrix;
	cameraDataUBO.projectionView = mat4Transpose(projectionView);
	cameraDataUBO.invProjectionView = mat4Transpose(mat4Inverse(projectionView));
	cameraDataUBO.position = _scene->camera.origin;

	SceneUBO sceneDataUBO;
	sceneDataUBO.camera = cameraDataUBO;

	const KyuRen::Camera& camera = _scene->camera;

	if (_scene->meshEntities.size() != 0) {
		Soul::Vec3f position = _scene->meshEntities[0].worldMatrix * Vec3f(0, 0, 0);
		SOUL_LOG_INFO("Position ; (%f, %f, %f)", position.x, position.y, position.z);
	}

	if (_scene->sunLightEntities.size() != 0) {
		KyuRen::SunLightEntity& entity = (KyuRen::SunLightEntity&) _scene->sunLightEntities[0];
		entity.updateShadowMatrixes(camera);

		SOUL_LOG_INFO("World matrix sun light : ");
		for (int i = 0; i < 4; i++) {
			SOUL_LOG_INFO("(%f , %f, %f, %f)", entity.worldMatrix.elem[i][0], entity.worldMatrix.elem[i][1], entity.worldMatrix.elem[i][2], entity.worldMatrix.elem[i][3]);
		}

		Soul::Vec3f direction = (entity.worldMatrix * Vec3f(0, 0, 1)) - (entity.worldMatrix * Vec3f(0, 0, 0));
		direction *= -1;
		direction = Soul::unit(direction);

		SOUL_LOG_INFO("Direction : (%f, %f, %f)", direction.x, direction.y, direction.z);

		const KyuRen::SunLight& light = _scene->sunLights[entity.sunLightID];

		for (int i = 0; i < 4; i++) {
			sceneDataUBO.light.shadowMatrix[i] = mat4Transpose(entity.shadowMatrixes[i]);
		}
		sceneDataUBO.light.direction = direction;
		sceneDataUBO.light.bias = 0.001f;
		sceneDataUBO.light.color = light.color;
		float cameraFar = camera.perspective.zFar;
		float cameraNear = camera.perspective.zNear;
		float cameraDepth = 200 - cameraNear;
		for (int j = 0; j < 3; j++) {
			sceneDataUBO.light.cascadeDepths[j] = _scene->camera.perspective.zNear + (cameraDepth * entity.split[j]);
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

	GPU::BufferID sceneBuffer = _gpuSystem->bufferCreate(sceneBufferDesc,
		[&sceneDataUBO](int i, byte* data) {
			auto sceneData = (SceneUBO*)data;
			*sceneData = sceneDataUBO;
		});
	_gpuSystem->bufferDestroy(sceneBuffer);
	outputs["sceneBuffer"].val.bufferNodeID = renderGraph->importBuffer("Scene Buffer", sceneBuffer);

	GPU::BufferDesc modelBufferDesc;
	modelBufferDesc.typeSize = sizeof(Soul::Mat4);
	modelBufferDesc.typeAlignment = alignof(Soul::Mat4);
	modelBufferDesc.count = _scene->meshEntities.size();
	modelBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	modelBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::BufferID modelBuffer = _gpuSystem->bufferCreate(modelBufferDesc,
		[this](int i, byte* data)
		{
			auto model = (Soul::Mat4*)data;
			*model = mat4Transpose(
				_scene->meshEntities[i].worldMatrix);
		});
	_gpuSystem->bufferDestroy(modelBuffer);
	outputs["modelBuffer"].val.bufferNodeID = renderGraph->importBuffer("Model Buffer", modelBuffer);

	using BufferNodeArray = Array<GPU::BufferNodeID>;
	BufferNodeArray* posVertexBuffers = (BufferNodeArray*) Runtime::GetTempAllocator()->create<BufferNodeArray>();
	BufferNodeArray* norVertexBuffers = (BufferNodeArray*)Runtime::GetTempAllocator()->create<BufferNodeArray>();
	BufferNodeArray* indexBuffers = (BufferNodeArray*)Runtime::GetTempAllocator()->create<BufferNodeArray>();

	posVertexBuffers->reserve(_scene->meshes.size());
	norVertexBuffers->reserve(_scene->meshes.size());
	indexBuffers->reserve(_scene->meshes.size());

	{
		SOUL_PROFILE_ZONE_WITH_NAME("Build input data");
		for (const KyuRen::Mesh& mesh : _scene->meshes) {
			posVertexBuffers->add(renderGraph->importBuffer("Pos Vertex Buffer", mesh.posVertexBufferID));
			norVertexBuffers->add(renderGraph->importBuffer("Nor Vertex Buffer", mesh.norVertexBufferID));
			indexBuffers->add(renderGraph->importBuffer("Index Buffer", mesh.indexBufferID));
		}
	}

	outputs["posVertexBuffers"].val.bufferArrays = posVertexBuffers;
	outputs["norVertexBuffers"].val.bufferArrays = norVertexBuffers;
	outputs["indexBuffers"].val.bufferArrays = indexBuffers;
}

