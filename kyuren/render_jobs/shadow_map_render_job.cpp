#include "shadow_map_render_job.h"
#include "core/array.h"
#include "../utils.h"
#include "gpu/gpu.h"

void ShadowMapRenderJob::init(const KyuRen::Scene* scene, Soul::GPU::System* gpuSystem) {
	_scene = scene;
	_gpuSystem = gpuSystem;

	_addInput({KyuRen::RenderJobParam::Type::GPUBuffer, "model", "Model buffer"});
	_addInput({KyuRen::RenderJobParam::Type::GPUBufferArray, "posVertexBuffers", "Vertex buffer for vertex position"});
	_addInput({ KyuRen::RenderJobParam::Type::GPUBufferArray, "indexBuffers", "Index Buffer" });

	_addOutput({ KyuRen::RenderJobParam::Type::GPUTexture2D, "shadowMap", "Shadow Map" });

	using namespace Soul;
	const char* shadowVertSrc = LoadFile("D:/Dev/soul/shaders/shadow_map_gen.vert.glsl");
	GPU::ShaderDesc shadowVertShaderDesc;
	shadowVertShaderDesc.name = "Shadow map vertex shader";
	shadowVertShaderDesc.source = shadowVertSrc;
	shadowVertShaderDesc.sourceSize = strlen(shadowVertSrc);
	_vertShaderID = gpuSystem->shaderCreate(shadowVertShaderDesc, GPU::ShaderStage::VERTEX);

	const char* shadowFragSrc = LoadFile("D:/Dev/soul/shaders/shadow_map_gen.frag.glsl");
	GPU::ShaderDesc shadowFragShaderDesc;
	shadowFragShaderDesc.name = "Shadow map fragment shader";
	shadowFragShaderDesc.source = shadowFragSrc;
	shadowFragShaderDesc.sourceSize = strlen(shadowFragSrc);
	_fragShaderID = gpuSystem->shaderCreate(shadowFragShaderDesc, GPU::ShaderStage::FRAGMENT);
}

void ShadowMapRenderJob::execute(Soul::GPU::RenderGraph* renderGraph, const KyuRen::RenderJobInputs& inputs, KyuRen::RenderJobOutputs& outputs) {
	using namespace Soul;

	GPU::BufferDesc shadowMatrixesBufferDesc;
	shadowMatrixesBufferDesc.typeSize = sizeof(Mat4);
	shadowMatrixesBufferDesc.typeAlignment = alignof(Mat4);
	shadowMatrixesBufferDesc.count = 4;
	shadowMatrixesBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	shadowMatrixesBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::BufferID shadowMatrixesBuffer = _gpuSystem->bufferCreate(shadowMatrixesBufferDesc,
		[this](int i, byte* data) -> void {
			auto shadowMatrix = (Mat4*)data;
			if (_scene->sunLightEntities.size() > 0) {
				*shadowMatrix = mat4Transpose(_scene->sunLightEntities[0].shadowMatrixes[i]);
			}
		});
	_gpuSystem->bufferDestroy(shadowMatrixesBuffer);

	GPU::RGTextureDesc shadowMapTexDesc;
	shadowMapTexDesc.width = KyuRen::SunLightEntity::SHADOW_MAP_RESOLUTION;
	shadowMapTexDesc.height = KyuRen::SunLightEntity::SHADOW_MAP_RESOLUTION;
	shadowMapTexDesc.depth = 1;
	shadowMapTexDesc.clear = true;
	shadowMapTexDesc.clearValue.depthStencil = { 1.0f, 0 };
	shadowMapTexDesc.format = GPU::TextureFormat::DEPTH32F;
	shadowMapTexDesc.mipLevels = 1;
	shadowMapTexDesc.type = GPU::TextureType::D2;
	GPU::TextureNodeID shadowMapNodeID = renderGraph->createTexture("Shadow Map", shadowMapTexDesc);
	
	struct PassData {
		GPU::BufferNodeID modelBuffer;
		GPU::BufferNodeID shadowMatrixesBuffer;
		Array<GPU::BufferNodeID> posVertexBuffers;
		Array<GPU::BufferNodeID> indexBuffers;
		GPU::TextureNodeID depthTarget;
	} passData;
	passData.modelBuffer = inputs["model"].val.bufferNodeID;
	passData.shadowMatrixesBuffer = renderGraph->importBuffer("Shadow Matrix", shadowMatrixesBuffer);
	passData.depthTarget = shadowMapNodeID;
	passData.posVertexBuffers = *(inputs["posVertexBuffers"].val.bufferArrays);
	passData.indexBuffers = *(inputs["indexBuffers"].val.bufferArrays);

	for (int i = 0; i < 4; i++) {
		passData = renderGraph->addGraphicPass<PassData>(
			"Shadow Pass",
			[i, &passData, this]
			(GPU::GraphicNodeBuilder* builder, PassData* data) {
				data->modelBuffer = builder->addShaderBuffer(passData.modelBuffer, GPU::SHADER_STAGE_VERTEX, GPU::ShaderBufferReadUsage::UNIFORM);
				data->shadowMatrixesBuffer = builder->addShaderBuffer(passData.shadowMatrixesBuffer, GPU::SHADER_STAGE_VERTEX, GPU::ShaderBufferReadUsage::UNIFORM);
				for (GPU::BufferNodeID nodeID : passData.posVertexBuffers)
				{
					data->posVertexBuffers.add(builder->addVertexBuffer(nodeID));
				}

				for (GPU::BufferNodeID nodeID : passData.indexBuffers)
				{
					data->indexBuffers.add(builder->addIndexBuffer(nodeID));
				}

				GPU::DepthStencilAttachmentDesc depthAttchDesc;
				depthAttchDesc.clear = i == 0;
				depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
				depthAttchDesc.depthWriteEnable = true;
				depthAttchDesc.depthTestEnable = true;
				depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
				data->depthTarget = builder->setDepthStencilAttachment(passData.depthTarget, depthAttchDesc);


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
				GPU::GraphicPipelineDesc pipelineConfig;
				pipelineConfig.viewport = { 0, 0, resolution, resolution };
				pipelineConfig.scissor = { false,  scissorRegion.offsetX, scissorRegion.offsetY, scissorRegion.width, scissorRegion.height };
				pipelineConfig.framebuffer = { uint16(resolution), uint16(resolution) };
				pipelineConfig.vertexShaderID = _vertShaderID;
				pipelineConfig.fragmentShaderID = _fragShaderID;
				pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

				builder->setPipelineConfig(pipelineConfig);
			},
			[this, i]
			(GPU::RenderGraphRegistry* registry, const PassData& data, GPU::CommandBucket* commandBucket) {
				GPU::Descriptor set1Descriptors[] = {
					GPU::Descriptor::Uniform(registry->getBuffer(data.shadowMatrixesBuffer), i, GPU::SHADER_STAGE_VERTEX)
				};
				GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { 1, set1Descriptors });

				commandBucket->reserve(_scene->meshEntities.size());

				for (int j = 0; j < _scene->meshEntities.size(); j++)
				{
					GPU::Descriptor set3Descriptors[] = {
						GPU::Descriptor::Uniform(registry->getBuffer(data.modelBuffer), j, GPU::SHADER_STAGE_VERTEX),
					};
					GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, { 1, set3Descriptors });

					const KyuRen::MeshEntity& meshEntity = _scene->meshEntities[j];
					uint32 meshInternalID = _scene->meshes.getInternalID(meshEntity.meshID);
					const KyuRen::Mesh& mesh = _scene->meshes[meshEntity.meshID];
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

	outputs["shadowMap"].val.textureNodeID = passData.depthTarget;
}