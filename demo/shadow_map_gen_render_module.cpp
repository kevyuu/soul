#include "shadow_map_gen_render_module.h"

using namespace Soul;

void ShadowMapGenRenderModule::init(GPU::System* system)
{
	const char* vertSrc = LoadFile("shaders/shadow_map_gen.vert.glsl");
	GPU::ShaderDesc vertShaderDesc;
	vertShaderDesc.name = "Shadow map generation vertex shader";
	vertShaderDesc.source = vertSrc;
	vertShaderDesc.sourceSize = strlen(vertSrc);
	_vertShaderID = system->shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

	const char* fragSrc = LoadFile("shaders/shadow_map_gen.frag.glsl");
	GPU::ShaderDesc fragShaderDesc;
	fragShaderDesc.name = "Shadow map generation fragment shader";
	fragShaderDesc.source = fragSrc;
	fragShaderDesc.sourceSize = strlen(fragSrc);
	_fragShaderID = system->shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);

	GPU::ProgramDesc programDesc;
	programDesc.shaderIDs[GPU::ShaderStage::VERTEX] = _vertShaderID;
	programDesc.shaderIDs[GPU::ShaderStage::FRAGMENT] = _fragShaderID;
	_programID = system->programRequest(programDesc);
}

ShadowMapGenRenderModule::Parameter ShadowMapGenRenderModule::addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const ShadowMapGenRenderModule::Parameter& data, const DeferredPipeline::Scene& scene) {
	Parameter passData = data;
	for (int i = 0; i < 4; i++)
	{
		passData = renderGraph->addGraphicPass<Parameter>(
			"Shadow map gen pass",
			[this, &passData, scene = &scene, i]
			(GPU::GraphicNodeBuilder* builder, Parameter* data) -> void
			{
				data->modelBuffer = builder->addShaderBuffer(passData.modelBuffer, GPU::SHADER_STAGE_VERTEX, GPU::ShaderBufferReadUsage::UNIFORM);
				data->shadowMatrixesBuffer = builder->addShaderBuffer(passData.shadowMatrixesBuffer, GPU::SHADER_STAGE_VERTEX, GPU::ShaderBufferReadUsage::UNIFORM);
				for (GPU::BufferNodeID nodeID : passData.vertexBuffers)
				{
					data->vertexBuffers.add(builder->addVertexBuffer(nodeID));
				}

				for (GPU::BufferNodeID nodeID : passData.indexBuffers)
				{
					data->indexBuffers.add(builder->addIndexBuffer(nodeID));
				}

				GPU::DepthStencilAttachmentDesc depthAttchDesc;
				depthAttchDesc.clear = i == 0;
				depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
				depthAttchDesc.depthWriteEnable = true;
				data->depthTarget = builder->setDepthStencilAttachment(passData.depthTarget, depthAttchDesc);

				uint16 resolution = DeferredPipeline::DirectionalLight::SHADOW_MAP_RESOLUTION;
				builder->setRenderTargetDimension({ resolution, resolution });

			},
			[this, scene = &scene, i]
			(GPU::RenderGraphRegistry* registry, const Parameter& data, GPU::RenderCommandBucket* commandBucket) -> void
			{
				uint16 resolution = DeferredPipeline::DirectionalLight::SHADOW_MAP_RESOLUTION;
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
				GPU::PipelineStateDesc pipelineDesc;

				pipelineDesc.inputBindings[0] = { sizeof(DeferredPipeline::Vertex) };
				pipelineDesc.inputAttributes[0] = { 0, offsetof(DeferredPipeline::Vertex, pos) };
				pipelineDesc.inputAttributes[1] = { 0, offsetof(DeferredPipeline::Vertex, normal) };
				pipelineDesc.inputAttributes[2] = { 0, offsetof(DeferredPipeline::Vertex, texUV) };
				pipelineDesc.inputAttributes[3] = { 0, offsetof(DeferredPipeline::Vertex, binormal) };
				pipelineDesc.inputAttributes[4] = { 0, offsetof(DeferredPipeline::Vertex, tangent) };

				pipelineDesc.viewport = { 0, 0, resolution, resolution };
				pipelineDesc.scissor = { false,  scissorRegion.offsetX, scissorRegion.offsetY, scissorRegion.width, scissorRegion.height };
				pipelineDesc.programID = this->_programID;
				pipelineDesc.colorAttachmentCount = 1;
				pipelineDesc.depthStencilAttachment = { true, true, GPU::CompareOp::LESS };
				pipelineDesc.raster.cullMode = GPU::CullMode::NONE;
				GPU::PipelineStateID pipelineStateID = registry->getPipelineState(pipelineDesc);

				GPU::Descriptor set1Descriptor = GPU::Descriptor::Uniform(registry->getBuffer(data.shadowMatrixesBuffer), i, GPU::SHADER_STAGE_VERTEX);
				GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { 1, &set1Descriptor });

				commandBucket->reserve(scene->meshEntities.size());

				for (int j = 0; j < scene->meshEntities.size(); j++)
				{
					GPU::Descriptor set3Descriptor = GPU::Descriptor::Uniform(registry->getBuffer(data.modelBuffer), j, GPU::SHADER_STAGE_VERTEX);
					GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, { 1, &set3Descriptor });

					const DeferredPipeline::MeshEntity& meshEntity = scene->meshEntities[j];
					const DeferredPipeline::Mesh& mesh = scene->meshes[meshEntity.meshID];
					using DrawCommand = GPU::RenderCommandDrawIndex;
					DrawCommand* command = commandBucket->put<DrawCommand>(j, j);
					command->pipelineStateID = pipelineStateID;
					command->vertexBufferID = registry->getBuffer(data.vertexBuffers[meshEntity.meshID]);
					command->indexBufferID = registry->getBuffer(data.indexBuffers[meshEntity.meshID]);
					command->indexCount = mesh.indexCount;
					command->shaderArgSetIDs[1] = set1;
					command->shaderArgSetIDs[3] = set3;
				}

			});
	}
	return passData;
}