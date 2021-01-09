#include "lighting_render_job.h"
#include "core/array.h"
#include "../utils.h"
#include "gpu/gpu.h"
#include "runtime/runtime.h"

void LightingRenderJob::init(const KyuRen::Scene* scene, Soul::GPU::System* system) {
	const char* vertSrc = LoadFile("D:/Dev/soul/shaders/unlit.vert.glsl");
	Soul::GPU::ShaderDesc vertShaderDesc;
	vertShaderDesc.name = "GBuffer generation vertex shader";
	vertShaderDesc.source = vertSrc;
	vertShaderDesc.sourceSize = strlen(vertSrc);
	_vertShaderID = system->shaderCreate(vertShaderDesc, Soul::GPU::ShaderStage::VERTEX);
	Soul::Runtime::Deallocate((void*) vertSrc, vertShaderDesc.sourceSize + 1);

	const char* fragSrc = LoadFile("D:/Dev/soul/shaders/unlit.frag.glsl");
	Soul::GPU::ShaderDesc fragShaderDesc;
	fragShaderDesc.name = "GBuffer generation fragment shader";
	fragShaderDesc.source = fragSrc;
	fragShaderDesc.sourceSize = strlen(fragSrc);
	_fragShaderID = system->shaderCreate(fragShaderDesc, Soul::GPU::ShaderStage::FRAGMENT);
	Soul::Runtime::Deallocate((void*)fragSrc, fragShaderDesc.sourceSize + 1);

	_addInput({
		KyuRen::RenderJobParam::Type::GPUBufferArray, "posVertexBuffers", "Vertex Buffer Array for vertex position"
	});
	_addInput({
		KyuRen::RenderJobParam::Type::GPUBufferArray,
		"norVertexBuffers",
		"Vertex Buffer Array for normal position"
	});
	_addInput({
		KyuRen::RenderJobParam::Type::GPUBufferArray,
		"indexBuffers",
		"Index Buffer"
	});
	_addInput({
		KyuRen::RenderJobParam::Type::GPUBuffer,
		"modelBuffer",
		"Model Buffer"
	});
	_addInput({
		KyuRen::RenderJobParam::Type::GPUBuffer,
		"sceneBuffer",
		"Scene Buffer"
	});
	_addInput({
		KyuRen::RenderJobParam::Type::GPUTexture2D,
		"shadowMap",
		"Shadow Map"
	});

	_addOutput({
		KyuRen::RenderJobParam::Type::GPUTexture2D,
		"renderTarget",
		"Render Target"
	});

	_scene = scene;
	_gpuSystem = system;
}

void LightingRenderJob::execute(Soul::GPU::RenderGraph* renderGraph, const KyuRen::RenderJobInputs& inputs, KyuRen::RenderJobOutputs& outputs) {
	using namespace Soul;
	struct PassData {
		Array<GPU::BufferNodeID> posVertexBuffers;
		Array<GPU::BufferNodeID> norVertexBuffers;
		Array<GPU::BufferNodeID> indexBuffers;
		GPU::BufferNodeID scene;
		GPU::BufferNodeID model;
		GPU::TextureNodeID shadowMap;
		GPU::TextureNodeID renderTarget;
		GPU::TextureNodeID depthTarget;
	} inputData;

	inputData.posVertexBuffers = *(inputs["posVertexBuffers"].val.bufferArrays);
	inputData.norVertexBuffers = *(inputs["norVertexBuffers"].val.bufferArrays);
	inputData.indexBuffers = *(inputs["indexBuffers"].val.bufferArrays);
	inputData.model = inputs["modelBuffer"].val.bufferNodeID;
	inputData.scene = inputs["sceneBuffer"].val.bufferNodeID;
	inputData.shadowMap = inputs["shadowMap"].val.textureNodeID;


	float width = _scene->camera.viewDim.x;
	float height = _scene->camera.viewDim.y;

	GPU::RGTextureDesc renderTargetDesc;
	renderTargetDesc.width = width;
	renderTargetDesc.height = height;
	renderTargetDesc.depth = 1;
	renderTargetDesc.clear = true;
	renderTargetDesc.clearValue = {};
	renderTargetDesc.format = GPU::TextureFormat::RGBA8;
	renderTargetDesc.mipLevels = 1;
	renderTargetDesc.type = GPU::TextureType::D2;
	inputData.renderTarget = renderGraph->createTexture("Render Targe", renderTargetDesc);

	GPU::RGTextureDesc sceneDepthTexDesc;
	sceneDepthTexDesc.width = width;
	sceneDepthTexDesc.height = height;
	sceneDepthTexDesc.depth = 1;
	sceneDepthTexDesc.clear = true;
	sceneDepthTexDesc.clearValue.depthStencil = { 1.0f, 0 };
	sceneDepthTexDesc.format = GPU::TextureFormat::DEPTH32F;
	sceneDepthTexDesc.mipLevels = 1;
	sceneDepthTexDesc.type = GPU::TextureType::D2;
	inputData.depthTarget = renderGraph->createTexture("Depth target", sceneDepthTexDesc);

	PassData outputData = renderGraph->addGraphicPass<PassData>(
		"Lighting Render Pass",
		[&inputData, this, width, height]
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
			colorAttchDesc.clearValue.color.float32 = { 0, 0, 0, 0 };
			data->renderTarget = builder->addColorAttachment(inputData.renderTarget, colorAttchDesc);

			GPU::DepthStencilAttachmentDesc depthAttchDesc;
			depthAttchDesc.clear = true;
			depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
			depthAttchDesc.depthWriteEnable = true;
			depthAttchDesc.depthTestEnable = true;
			depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
			data->depthTarget = builder->setDepthStencilAttachment(inputData.depthTarget, depthAttchDesc);

			data->scene = builder->addShaderBuffer(inputData.scene, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
			data->model = builder->addShaderBuffer(inputData.model, GPU::SHADER_STAGE_VERTEX , GPU::ShaderBufferReadUsage::UNIFORM);
			data->shadowMap = builder->addShaderTexture(inputData.shadowMap, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderTextureReadUsage::UNIFORM);

			Vec2f sceneResolution = Vec2f(width, height);
			GPU::GraphicPipelineDesc pipelineConfig;
			pipelineConfig.viewport = { 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
			pipelineConfig.scissor = { false, 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
			pipelineConfig.framebuffer = { uint16(sceneResolution.x), uint16(sceneResolution.y) };
			pipelineConfig.vertexShaderID = _vertShaderID;
			pipelineConfig.fragmentShaderID = _fragShaderID;

			pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

			builder->setPipelineConfig(pipelineConfig);
		},
		[this]
		(GPU::RenderGraphRegistry* registry, const PassData& passData, GPU::CommandBucket* commandBucket) {
			using DrawCommand = GPU::Command::DrawIndex2;
			commandBucket->reserve(_scene->meshEntities.size());

			GPU::SamplerDesc samplerDesc = {};
			samplerDesc.minFilter = GPU::TextureFilter::LINEAR;
			samplerDesc.magFilter = GPU::TextureFilter::LINEAR;
			samplerDesc.mipmapFilter = GPU::TextureFilter::LINEAR;
			samplerDesc.wrapU = GPU::TextureWrap::REPEAT;
			samplerDesc.wrapV = GPU::TextureWrap::REPEAT;
			samplerDesc.wrapW = GPU::TextureWrap::REPEAT;
			samplerDesc.anisotropyEnable = false;
			samplerDesc.maxAnisotropy = 0.0f;
			GPU::SamplerID samplerID = _gpuSystem->samplerRequest(samplerDesc);

			GPU::Descriptor set0Descriptors[] = {
				GPU::Descriptor::Uniform(registry->getBuffer(passData.scene), 0, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT),
				GPU::Descriptor::SampledImage(registry->getTexture(passData.shadowMap), samplerID, GPU::SHADER_STAGE_FRAGMENT)
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
				Runtime::WaitTask(renderTaskID); */

				for (int index = 0; index < _scene->meshEntities.size(); index++) {
					DrawCommand* command = commandBucket->put<DrawCommand>(index, index);
					const KyuRen::MeshEntity& meshEntity = _scene->meshEntities[index];
					uint32 meshInternalID = _scene->meshes.getInternalID(meshEntity.meshID);
					const KyuRen::Mesh& mesh = _scene->meshes[meshEntity.meshID];
					GPU::BufferNodeID posVertexBufferNode = passData.posVertexBuffers[meshInternalID];
					command->vertexBufferIDs[0] = registry->getBuffer(posVertexBufferNode);
					GPU::BufferNodeID norVertexBufferNode = passData.norVertexBuffers[meshInternalID];
					command->vertexBufferIDs[1] = registry->getBuffer(norVertexBufferNode);

					command->vertexCount = 2;
					command->indexBufferID = registry->getBuffer(passData.indexBuffers[meshInternalID]);
					command->indexCount = mesh.indexCount;
					command->shaderArgSets[0] = set0;

					GPU::Descriptor set1Descriptors[1] = {
						GPU::Descriptor::Uniform(registry->getBuffer(passData.model), index, GPU::SHADER_STAGE_VERTEX)
					};

					GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { 1, set1Descriptors });
					command->shaderArgSets[1] = set1;
				}

			}
		}
		);

	outputs["renderTarget"].val.textureNodeID = outputData.renderTarget;
}

