#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"

#include "scene.h"

using namespace Soul;

class FinalGatherRenderModule {
private:
    GPU::ShaderID _vertShaderID;
    GPU::ShaderID _fragShaderID;

public:
    struct Parameter
    {
        GPU::TextureNodeID renderMap[4];
        GPU::TextureNodeID renderTarget;
        GPU::BufferNodeID vertexBuffer;
    };

    void init(GPU::System* system)
    {
        const char* vertSrc = LoadFile("shaders/final_gather.vert.glsl");
        GPU::ShaderDesc vertShaderDesc;
        vertShaderDesc.name = "Final gather vertex shader";
        vertShaderDesc.source = vertSrc;
        vertShaderDesc.sourceSize = strlen(vertSrc);
        _vertShaderID = system->shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

        const char* fragSrc = LoadFile("shaders/final_gather.frag.glsl");
        GPU::ShaderDesc fragShaderDesc;
        fragShaderDesc.name = "Final gather fragment shader";
        fragShaderDesc.source = fragSrc;
        fragShaderDesc.sourceSize = strlen(fragSrc);
        _fragShaderID = system->shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);
    }

    Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& parameter, Vec2ui32 sceneResolution) {
        Parameter passParameter = renderGraph->addGraphicPass<Parameter>(
            "Final Gather Pass",
            [this, &parameter, sceneResolution]
            (GPU::GraphicNodeBuilder* builder, Parameter* params) -> void {
                for (int i = 0; i < 4; i++) {
                    params->renderMap[i] = builder->addInputAttachment(parameter.renderMap[i], 0, i);
                }

                GPU::ColorAttachmentDesc colorAttchDesc;
                colorAttchDesc.blendEnable = false;
                colorAttchDesc.clear = true;
                colorAttchDesc.clearValue.color.float32 = { 0.0f, 0.0f, 1.0f, 1.0f };
                params->renderTarget = builder->addColorAttachment(parameter.renderTarget, colorAttchDesc);

                params->vertexBuffer = builder->addVertexBuffer(parameter.vertexBuffer);

                GPU::GraphicPipelineConfig pipelineConfig;
                pipelineConfig.inputLayout.topology = GPU::Topology::TRIANGLE_STRIP;
                pipelineConfig.viewport = { 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
                pipelineConfig.scissor = { false, 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
                pipelineConfig.framebuffer = { uint16(sceneResolution.x), uint16(sceneResolution.y) };
                pipelineConfig.vertexShaderID = this->_vertShaderID;
                pipelineConfig.fragmentShaderID = this->_fragShaderID;
                pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

                builder->setPipelineConfig(pipelineConfig);
            },
            [](GPU::RenderGraphRegistry* registry, const Parameter& params, GPU::CommandBucket* commandBucket) -> void {

                GPU::Descriptor set0Descriptors[4] = {};
                for (int i = 0; i < 4; i++) {
                    GPU::Descriptor& descriptor = set0Descriptors[i];
                    descriptor.type = GPU::DescriptorType::INPUT_ATTACHMENT;
                    descriptor.inputAttachmentInfo.textureID = registry->getTexture(params.renderMap[i]);
                }

                GPU::ShaderArgSetDesc set0Desc = {};
                set0Desc.bindingCount = 4;
                set0Desc.bindingDescriptions = set0Descriptors;
                GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, set0Desc);

                commandBucket->reserve(1);
                using DrawCommand = GPU::Command::DrawVertex;
                DrawCommand* command = commandBucket->put<DrawCommand>(0, 0);
                command->vertexBufferID = registry->getBuffer(params.vertexBuffer);
                command->vertexCount = 4;
                command->shaderArgSets[0] = set0;
            });
        return passParameter;
    }
};