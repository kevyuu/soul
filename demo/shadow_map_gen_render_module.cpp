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
				data->modelBuffer = builder->addInShaderBuffer(passData.modelBuffer, 3, 0);
				data->shadowMatrixesBuffer = builder->addInShaderBuffer(passData.shadowMatrixesBuffer, 1, 0);
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
				depthAttchDesc.depthTestEnable = true;
				depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
				data->depthTarget = builder->setDepthStencilAttachment(passData.depthTarget, depthAttchDesc);


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
				GPU::GraphicPipelineConfig pipelineConfig;
				pipelineConfig.viewport = { 0, 0, resolution, resolution };
				pipelineConfig.scissor = { false,  scissorRegion.offsetX, scissorRegion.offsetY, scissorRegion.width, scissorRegion.height };
				pipelineConfig.framebuffer = { uint16(resolution), uint16(resolution) };
				pipelineConfig.vertexShaderID = this->_vertShaderID;
				pipelineConfig.fragmentShaderID = this->_fragShaderID;
				pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

				builder->setPipelineConfig(pipelineConfig);

			},
			[scene = &scene, i]
			(GPU::RenderGraphRegistry* registry, const Parameter& data, GPU::CommandBucket* commandBucket) -> void
			{
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

					const DeferredPipeline::MeshEntity& meshEntity = scene->meshEntities[j];
					const DeferredPipeline::Mesh& mesh = scene->meshes[meshEntity.meshID];
					using DrawCommand = GPU::Command::DrawIndex;
					DrawCommand* command = commandBucket->put<DrawCommand>(j, j);
					command->vertexBufferID = registry->getBuffer(data.vertexBuffers[meshEntity.meshID]);
					command->indexBufferID = registry->getBuffer(data.indexBuffers[meshEntity.meshID]);
					command->indexCount = mesh.indexCount;
					command->shaderArgSets[1] = set1;
					command->shaderArgSets[3] = set3;
				}

			});
	}
	return passData;
}