#include "gbuffer_gen_render_module.h"

using namespace Soul;

void GBufferGenRenderModule::init(GPU::System* system)
{
    const char* vertSrc = LoadFile("shaders/gbuffer_gen.vert.glsl");
    GPU::ShaderDesc vertShaderDesc;
    vertShaderDesc.name = "GBuffer generation vertex shader";
    vertShaderDesc.source = vertSrc;
    vertShaderDesc.sourceSize = strlen(vertSrc);
    _vertShaderID = system->shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

    const char* fragSrc = LoadFile("shaders/gbuffer_gen.frag.glsl");
    GPU::ShaderDesc fragShaderDesc;
    fragShaderDesc.name = "GBuffer generation fragment shader";
    fragShaderDesc.source = fragSrc;
    fragShaderDesc.sourceSize = strlen(fragSrc);
    _fragShaderID = system->shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);
}

GBufferGenRenderModule::Parameter GBufferGenRenderModule::addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const GBufferGenRenderModule::Parameter& inputParams, const DeferredPipeline::Scene& scene) {
    Parameter passParam = renderGraph->addGraphicPass<Parameter>(
        "G-Buffer Gen Pass",
        [this, &inputParams, &scene]
    (GPU::GraphicNodeBuilder* builder, Parameter* params) -> void {
            SOUL_PROFILE_ZONE_WITH_NAME("Setup render albedo pass");
            for (GPU::TextureNodeID nodeID : inputParams.sceneTextures) {
                params->sceneTextures.add(builder->addShaderTexture(nodeID, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderTextureReadUsage::UNIFORM));
            }

            params->stubTexture = builder->addShaderTexture(inputParams.stubTexture, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderTextureReadUsage::UNIFORM);

            for (GPU::BufferNodeID nodeID : inputParams.vertexBuffers) {
                params->vertexBuffers.add(builder->addVertexBuffer(nodeID));
            }

            for (GPU::BufferNodeID nodeID : inputParams.indexBuffers) {
                params->indexBuffers.add(builder->addIndexBuffer(nodeID));
            }

            params->camera = builder->addShaderBuffer(inputParams.camera, GPU::SHADER_STAGE_VERTEX | GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
            params->light = builder->addShaderBuffer(inputParams.light, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
            params->material = builder->addShaderBuffer(inputParams.material, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderBufferReadUsage::UNIFORM);
            params->model = builder->addShaderBuffer(inputParams.model, GPU::SHADER_STAGE_VERTEX, GPU::ShaderBufferReadUsage::UNIFORM);
            params->shadowMap = builder->addShaderTexture(inputParams.shadowMap, GPU::SHADER_STAGE_FRAGMENT, GPU::ShaderTextureReadUsage::UNIFORM);

            GPU::ColorAttachmentDesc colorAttchDesc;
            colorAttchDesc.blendEnable = false;
            colorAttchDesc.clear = true;
            colorAttchDesc.clearValue.color.float32 = { 1.0f, 0.0f, 0.0f, 1.0f };
            for (int i = 0; i < 4; i++) {
                params->renderTargets[i] = builder->addColorAttachment(inputParams.renderTargets[i], colorAttchDesc);
            }

            GPU::DepthStencilAttachmentDesc depthAttchDesc;
            depthAttchDesc.clear = true;
            depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
            depthAttchDesc.depthWriteEnable = true;
            depthAttchDesc.depthTestEnable = true;
            depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
            params->depthTarget = builder->setDepthStencilAttachment(inputParams.depthTarget, depthAttchDesc);

            Vec2f sceneResolution = Vec2f(scene.camera.viewportWidth, scene.camera.viewportHeight);
            GPU::GraphicPipelineConfig pipelineConfig;
            pipelineConfig.viewport = { 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
            pipelineConfig.scissor = { false, 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
            pipelineConfig.framebuffer = { uint16(sceneResolution.x), uint16(sceneResolution.y) };
            pipelineConfig.vertexShaderID = this->_vertShaderID;
            pipelineConfig.fragmentShaderID = this->_fragShaderID;
            pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

            builder->setPipelineConfig(pipelineConfig);
        },
        [&scene, system]
        (GPU::RenderGraphRegistry* registry, const Parameter& params, GPU::CommandBucket* commandBucket) {
            SOUL_PROFILE_ZONE_WITH_NAME("Execute GBuffer Gen Passs");

            GPU::SamplerDesc samplerDesc = {};
            samplerDesc.minFilter = GPU::TextureFilter::LINEAR;
            samplerDesc.magFilter = GPU::TextureFilter::LINEAR;
            samplerDesc.mipmapFilter = GPU::TextureFilter::LINEAR;
            samplerDesc.wrapU = GPU::TextureWrap::REPEAT;
            samplerDesc.wrapV = GPU::TextureWrap::REPEAT;
            samplerDesc.wrapW = GPU::TextureWrap::REPEAT;
            samplerDesc.anisotropyEnable = false;
            samplerDesc.maxAnisotropy = 0.0f;
            GPU::SamplerID samplerID = system->samplerRequest(samplerDesc);

            commandBucket->reserve(scene.meshEntities.size());

            GPU::Descriptor set0Descriptors[3] = {
                    GPU::Descriptor::Uniform(registry->getBuffer(params.camera), 0),
                    GPU::Descriptor::Uniform(registry->getBuffer(params.light), 0),
                    GPU::Descriptor::SampledImage(registry->getTexture(params.shadowMap), samplerID)
            };
            GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, { 3, set0Descriptors });

            struct JobData {
                GPU::RenderGraphRegistry* registry;
                GPU::CommandBucket* commandBucket;
                const Parameter& param;
                GPU::ShaderArgSetID set0;
                GPU::SamplerID samplerID;
            };

            JobData jobData = {
                    registry,
                    commandBucket,
                    params,
                    set0,
                    samplerID
            };

            Runtime::TaskID commandCreateTask = Runtime::ParallelForTaskCreate(
                0, scene.meshEntities.size(), 256,
                [&scene, &jobData](int index) {
                    SOUL_PROFILE_ZONE_WITH_NAME(
                        "Record G-Buffer Generation Commands");
                    GPU::RenderGraphRegistry* registry = jobData.registry;
                    GPU::CommandBucket* commandBucket = jobData.commandBucket;
                    const Parameter& data = jobData.param;
                    GPU::ShaderArgSetID set0 = jobData.set0;
                    GPU::SamplerID samplerID = jobData.samplerID;

                    const DeferredPipeline::MeshEntity& meshEntity = scene.meshEntities[index];
                    const DeferredPipeline::SceneMaterial& material = scene.materials[meshEntity.materialID];

                    GPU::TextureID stubTexture = registry->getTexture(data.stubTexture);

                    GPU::Descriptor materialBufferDescriptor = GPU::Descriptor::Uniform(registry->getBuffer(data.material), meshEntity.materialID);
                    GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, { 1, &materialBufferDescriptor });

                    GPU::Descriptor materialMapDescriptor[6] = {
                            GPU::Descriptor::SampledImage(material.useAlbedoTex ? registry->getTexture(data.sceneTextures[material.albedoTexID]) : stubTexture, samplerID),
                            GPU::Descriptor::SampledImage(material.useNormalTex ? registry->getTexture(data.sceneTextures[material.normalTexID]) : stubTexture, samplerID),
                            GPU::Descriptor::SampledImage(material.useMetallicTex ? registry->getTexture(data.sceneTextures[material.metallicTexID]) : stubTexture, samplerID),
                            GPU::Descriptor::SampledImage(material.useRoughnessTex ? registry->getTexture(data.sceneTextures[material.roughnessTexID]) : stubTexture, samplerID),
                            GPU::Descriptor::SampledImage(material.useAOTex ? registry->getTexture(data.sceneTextures[material.aoTexID]) : stubTexture, samplerID),
                            GPU::Descriptor::SampledImage(material.useEmissiveTex ? registry->getTexture(data.sceneTextures[material.emissiveTexID]) : stubTexture, samplerID)
                    };
                    GPU::ShaderArgSetID set2 = registry->getShaderArgSet(2, { 6, materialMapDescriptor });

                    GPU::Descriptor modelDescriptor = GPU::Descriptor::Uniform(registry->getBuffer(data.model), index);
                    GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, { 1, &modelDescriptor });

                    const DeferredPipeline::Mesh& mesh = scene.meshes[meshEntity.meshID];
                    using DrawCommand = GPU::Command::DrawIndex;
                    DrawCommand* command = commandBucket->put<DrawCommand>(
                        index, index);
                    command->vertexBufferID = registry->getBuffer(
                        data.vertexBuffers[meshEntity.meshID]);
                    command->indexBufferID = registry->getBuffer(
                        data.indexBuffers[meshEntity.meshID]);
                    command->indexCount = mesh.indexCount;
                    command->shaderArgSets[0] = set0;
                    command->shaderArgSets[1] = set1;
                    command->shaderArgSets[2] = set2;
                    command->shaderArgSets[3] = set3;
                });

            Runtime::RunTask(commandCreateTask);
            Runtime::WaitTask(commandCreateTask);
        });
    return passParam;
}