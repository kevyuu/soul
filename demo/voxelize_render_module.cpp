#include "voxelize_render_module.h"

using namespace Soul;

void VoxelizeRenderModule::init(GPU::System* system)
{
    const char* vertSrc = LoadFile("shaders/voxelize.vert.glsl");
    GPU::ShaderDesc vertShaderDesc;
    vertShaderDesc.name = "Voxelization vertex shader";
    vertShaderDesc.source = vertSrc;
    vertShaderDesc.sourceSize = strlen(vertSrc);
    _vertShaderID = system->shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

    const char* geomSrc = LoadFile("shaders/voxelize.geom.glsl");
    GPU::ShaderDesc geomShaderDesc;
    geomShaderDesc.name = "Voxelization geometry shader";
    geomShaderDesc.source = geomSrc;
    geomShaderDesc.sourceSize = strlen(geomSrc);
    _geomShaderID = system->shaderCreate(geomShaderDesc, GPU::ShaderStage::GEOMETRY);

    const char* fragSrc = LoadFile("shaders/voxelize.frag.glsl");
    GPU::ShaderDesc fragShaderDesc;
    fragShaderDesc.name = "Voxelization Fragment shader";
    fragShaderDesc.source = fragSrc;
    fragShaderDesc.sourceSize = strlen(fragSrc);
    _fragShaderID = system->shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);
}

VoxelizeRenderModule::Parameter VoxelizeRenderModule::addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& inputParams, const DeferredPipeline::Scene& scene) {
    return renderGraph->addGraphicPass<Parameter>(
        "Voxelization pass",
        [this, &inputParams, scene = &scene]
    (GPU::GraphicNodeBuilder* builder, Parameter* params) -> void {
            for (GPU::BufferNodeID nodeID : inputParams.vertexBuffers) {
                params->vertexBuffers.add(builder->addVertexBuffer(nodeID));
            }

            for (GPU::BufferNodeID nodeID : inputParams.indexBuffers) {
                params->indexBuffers.add(builder->addIndexBuffer(nodeID));
            }

            params->voxelizeMatrixes = builder->addInShaderBuffer(inputParams.voxelizeMatrixes, 0, 0);
            params->voxelGIData = builder->addInShaderBuffer(inputParams.voxelGIData, 0, 1);
            params->voxelAlbedo = builder->addOutShaderTexture(inputParams.voxelAlbedo, 0, 2);
            params->voxelNormal = builder->addOutShaderTexture(inputParams.voxelNormal, 0, 3);
            params->voxelEmissive = builder->addOutShaderTexture(inputParams.voxelEmissive, 0, 4);

            params->material = builder->addInShaderBuffer(inputParams.material, 1, 0);

            for (GPU::TextureNodeID nodeID : inputParams.materialTextures) {
                params->materialTextures.add(builder->addInShaderTexture(nodeID, 2, 0));
            }
            params->stubTexture = builder->addInShaderTexture(inputParams.stubTexture, 2, 0);

            params->model = builder->addInShaderBuffer(inputParams.model, 3, 0);

            params->rotation = builder->addInShaderBuffer(inputParams.rotation, 4, 0);

            Vec2f voxelFrusutmReso = Vec2f(scene->voxelGIConfig.resolution, scene->voxelGIConfig.resolution);
            GPU::GraphicPipelineConfig pipelineConfig;
            pipelineConfig.viewport = { 0, 0, uint16(voxelFrusutmReso.x), uint16(voxelFrusutmReso.y) };
            pipelineConfig.scissor = { false, 0, 0, uint16(voxelFrusutmReso.x), uint16(voxelFrusutmReso.y) };
            pipelineConfig.framebuffer = { uint16(voxelFrusutmReso.x), uint16(voxelFrusutmReso.y) };
            pipelineConfig.vertexShaderID = this->_vertShaderID;
            pipelineConfig.fragmentShaderID = this->_fragShaderID;
            pipelineConfig.geometryShaderID = this->_geomShaderID;
            pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

            builder->setPipelineConfig(pipelineConfig);
        },
        [scene = &scene, system]
        (GPU::RenderGraphRegistry* registry, const Parameter& params, GPU::CommandBucket* commandBucket) {
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

            GPU::Descriptor set0Descriptors[5];

            GPU::Descriptor& voxelGIMatrixDescriptor = set0Descriptors[0];
            voxelGIMatrixDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
            voxelGIMatrixDescriptor.uniformInfo = { registry->getBuffer(params.voxelizeMatrixes), 0 };

            GPU::Descriptor& voxelGIDataDescriptor = set0Descriptors[1];
            voxelGIDataDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
            voxelGIDataDescriptor.uniformInfo = { registry->getBuffer(params.voxelGIData), 0 };

            GPU::Descriptor& voxelAlbedoDescriptor = set0Descriptors[2];
            voxelAlbedoDescriptor.type = GPU::DescriptorType::STORAGE_IMAGE;
            voxelAlbedoDescriptor.storageImageInfo = { registry->getTexture(params.voxelAlbedo) };

            GPU::Descriptor& voxelNormalDescriptor = set0Descriptors[3];
            voxelNormalDescriptor.type = GPU::DescriptorType::STORAGE_IMAGE;
            voxelNormalDescriptor.storageImageInfo = { registry->getTexture(params.voxelNormal) };

            GPU::Descriptor& voxelEmissiveDescriptor = set0Descriptors[4];
            voxelEmissiveDescriptor.type = GPU::DescriptorType::STORAGE_IMAGE;
            voxelEmissiveDescriptor.storageImageInfo = { registry->getTexture(params.voxelEmissive) };

            GPU::ShaderArgSetDesc set0Desc;
            set0Desc.bindingCount = 5;
            set0Desc.bindingDescriptions = set0Descriptors;
            GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, set0Desc);

            commandBucket->reserve(scene->meshEntities.size());

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

            Runtime::TaskID commandCreateTask = Runtime::System::Get().parallelForTaskCreate(
                0, scene->meshEntities.size(), 256,
                [scene = scene, &jobData](int index) {
                    SOUL_PROFILE_ZONE_WITH_NAME(
                        "Record G-Buffer Generation Commands");
                    GPU::RenderGraphRegistry* registry = jobData.registry;
                    GPU::CommandBucket* commandBucket = jobData.commandBucket;
                    const Parameter& data = jobData.param;
                    GPU::ShaderArgSetID set0 = jobData.set0;
                    GPU::SamplerID samplerID = jobData.samplerID;

                    const DeferredPipeline::MeshEntity& meshEntity = scene->meshEntities[index];
                    const DeferredPipeline::SceneMaterial& material = scene->materials[meshEntity.materialID];
                    GPU::Descriptor materialMapDescriptor[6] = {};
                    materialMapDescriptor[0].type = GPU::DescriptorType::SAMPLED_IMAGE;
                    materialMapDescriptor[0].sampledImageInfo.samplerID = samplerID;
                    materialMapDescriptor[0].sampledImageInfo.textureID = registry->getTexture(
                        data.stubTexture);
                    if (material.useAlbedoTex) {
                        materialMapDescriptor[0].sampledImageInfo.textureID = registry->getTexture(
                            data.materialTextures[material.albedoTexID]);
                    }

                    materialMapDescriptor[1].type = GPU::DescriptorType::SAMPLED_IMAGE;
                    materialMapDescriptor[1].sampledImageInfo.samplerID = samplerID;
                    materialMapDescriptor[1].sampledImageInfo.textureID = registry->getTexture(
                        data.stubTexture);
                    if (material.useNormalTex) {
                        materialMapDescriptor[1].sampledImageInfo.textureID = registry->getTexture(
                            data.materialTextures[material.normalTexID]);
                    }

                    materialMapDescriptor[2].type = GPU::DescriptorType::SAMPLED_IMAGE;
                    materialMapDescriptor[2].sampledImageInfo.samplerID = samplerID;
                    materialMapDescriptor[2].sampledImageInfo.textureID = registry->getTexture(
                        data.stubTexture);
                    if (material.useMetallicTex) {
                        materialMapDescriptor[2].sampledImageInfo.textureID = registry->getTexture(
                            data.materialTextures[material.metallicTexID]);
                    }

                    materialMapDescriptor[3].type = GPU::DescriptorType::SAMPLED_IMAGE;
                    materialMapDescriptor[3].sampledImageInfo.samplerID = samplerID;
                    materialMapDescriptor[3].sampledImageInfo.textureID = registry->getTexture(
                        data.stubTexture);
                    if (material.useRoughnessTex) {
                        materialMapDescriptor[3].sampledImageInfo.textureID = registry->getTexture(
                            data.materialTextures[material.roughnessTexID]);
                    }

                    materialMapDescriptor[4].type = GPU::DescriptorType::SAMPLED_IMAGE;
                    materialMapDescriptor[4].sampledImageInfo.samplerID = samplerID;
                    materialMapDescriptor[4].sampledImageInfo.textureID = registry->getTexture(
                        data.stubTexture);
                    if (material.useAOTex) {
                        materialMapDescriptor[4].sampledImageInfo.textureID = registry->getTexture(
                            data.materialTextures[material.aoTexID]);
                    }

                    materialMapDescriptor[5].type = GPU::DescriptorType::SAMPLED_IMAGE;
                    materialMapDescriptor[5].sampledImageInfo.samplerID = samplerID;
                    materialMapDescriptor[5].sampledImageInfo.textureID = registry->getTexture(
                        data.stubTexture);
                    if (material.useEmissiveTex) {
                        materialMapDescriptor[5].sampledImageInfo.textureID = registry->getTexture(
                            data.materialTextures[material.emissiveTexID]);
                    }

                    GPU::Descriptor materialBufferDescriptor = {};
                    materialBufferDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
                    materialBufferDescriptor.uniformInfo.bufferID = registry->getBuffer(
                        data.material);
                    materialBufferDescriptor.uniformInfo.unitIndex = meshEntity.materialID;

                    GPU::ShaderArgSetDesc set1Desc;
                    set1Desc.bindingCount = 1;
                    set1Desc.bindingDescriptions = &materialBufferDescriptor;
                    GPU::ShaderArgSetID set1 = registry->getShaderArgSet(
                        1, set1Desc);

                    GPU::ShaderArgSetDesc set2Desc;
                    set2Desc.bindingCount = 6;
                    set2Desc.bindingDescriptions = materialMapDescriptor;
                    GPU::ShaderArgSetID set2 = registry->getShaderArgSet(
                        2, set2Desc);

                    GPU::Descriptor modelDescriptor = {};
                    modelDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
                    modelDescriptor.uniformInfo.bufferID = registry->getBuffer(
                        data.model);
                    modelDescriptor.uniformInfo.unitIndex = index;
                    GPU::ShaderArgSetDesc set3Desc;
                    set3Desc.bindingCount = 1;
                    set3Desc.bindingDescriptions = &modelDescriptor;
                    GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, set3Desc);

                    GPU::Descriptor rotationDescriptor = {};
                    rotationDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
                    rotationDescriptor.uniformInfo = { registry->getBuffer(data.rotation), uint32(index) };
                    GPU::ShaderArgSetDesc set4Desc;
                    set4Desc.bindingCount = 1;
                    set4Desc.bindingDescriptions = &rotationDescriptor;
                    GPU::ShaderArgSetID set4 = registry->getShaderArgSet(4, set4Desc);

                    const DeferredPipeline::Mesh& mesh = scene->meshes[meshEntity.meshID];
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
                    command->shaderArgSets[4] = set4;

                });

            Runtime::System::Get().taskRun(commandCreateTask);
            Runtime::System::Get().taskWait(commandCreateTask);

        });
}