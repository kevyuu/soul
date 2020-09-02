#include "voxel_light_inject_render_module.h"

using namespace Soul;

void VoxelLightInjectRenderModule::init(GPU::System* system) {
    const char* compSrc = LoadFile("shaders/voxel_light_inject.comp.glsl");
    GPU::ShaderDesc compShaderDesc;
    compShaderDesc.name = "Voxel Light Inject compute shader";
    compShaderDesc.source = compSrc;
    compShaderDesc.sourceSize = strlen(compSrc);
    _compShaderID = system->shaderCreate(compShaderDesc, GPU::ShaderStage::COMPUTE);
}

VoxelLightInjectRenderModule::Parameter VoxelLightInjectRenderModule::addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& data, const DeferredPipeline::Scene& scene) {
    return renderGraph->addComputePass<Parameter>(
        "Voxel Light Inject Pass",
        [this, &data](GPU::ComputeNodeBuilder* builder, Parameter* parameter) -> void {
            parameter->voxelAlbedo = builder->addInShaderTexture(data.voxelAlbedo, 0, 0);
            parameter->voxelNormal = builder->addInShaderTexture(data.voxelNormal, 0, 1);
            parameter->voxelEmissive = builder->addInShaderTexture(data.voxelEmissive, 0, 2);
            parameter->voxelLight = builder->addOutShaderTexture(data.voxelLight, 0, 3);

            parameter->voxelGIData = builder->addInShaderBuffer(data.voxelGIData, 0, 4);
            parameter->lightData = builder->addInShaderBuffer(data.lightData, 0, 5);

            builder->setPipelineConfig({ this->_compShaderID });
        },
        [system, &scene](GPU::RenderGraphRegistry* registry, const Parameter& parameter, GPU::CommandBucket* commandBucket) -> void {
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

            GPU::Descriptor descriptors[6] = {
                GPU::Descriptor::SampledImage(registry->getTexture(parameter.voxelAlbedo), samplerID),
                GPU::Descriptor::SampledImage(registry->getTexture(parameter.voxelNormal), samplerID),
                GPU::Descriptor::SampledImage(registry->getTexture(parameter.voxelEmissive), samplerID),
                GPU::Descriptor::StorageImage(registry->getTexture(parameter.voxelLight),0),
                GPU::Descriptor::Uniform(registry->getBuffer(parameter.voxelGIData), 0),
                GPU::Descriptor::Uniform(registry->getBuffer(parameter.lightData), 0)
            };
            GPU::ShaderArgSetID argSet0ID = registry->getShaderArgSet(0, { 6, descriptors });

            commandBucket->reserve(1);
            using DrawCommand = GPU::Command::Dispatch;
            DrawCommand* command = commandBucket->put<DrawCommand>(0, 0);
            command->groupCount = { scene.voxelGIConfig.resolution / 8, scene.voxelGIConfig.resolution / 8, scene.voxelGIConfig.resolution / 8 };
            command->shaderArgSets[0] = argSet0ID;
        });
}