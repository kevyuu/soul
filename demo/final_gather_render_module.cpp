#include "final_gather_render_module.h"

using namespace Soul;

void FinalGatherRenderModule::init(GPU::System* system) {
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

    GPU::ProgramDesc programDesc;
    programDesc.shaderIDs[GPU::ShaderStage::VERTEX] = _vertShaderID;
    programDesc.shaderIDs[GPU::ShaderStage::FRAGMENT] = _fragShaderID;
    _programID = system->programRequest(programDesc);
}

FinalGatherRenderModule::Parameter FinalGatherRenderModule::addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const FinalGatherRenderModule::Parameter& parameter, Vec2ui32 sceneResolution) {
    Parameter passParameter = renderGraph->addGraphicPass<Parameter>(
        "Final Gather Pass",
        [this, &parameter, sceneResolution]
        (GPU::GraphicNodeBuilder* builder, Parameter* params) -> void {
            for (int i = 0; i < 4; i++) {
                params->renderMap[i] = builder->addInputAttachment(parameter.renderMap[i], GPU::SHADER_STAGE_FRAGMENT);
            }

            GPU::ColorAttachmentDesc colorAttchDesc;
            colorAttchDesc.clear = true;
            colorAttchDesc.clearValue.color.float32 = { 0.0f, 0.0f, 1.0f, 1.0f };
            params->renderTarget = builder->addColorAttachment(parameter.renderTarget, colorAttchDesc);

            params->vertexBuffer = builder->addVertexBuffer(parameter.vertexBuffer);

            builder->setRenderTargetDimension({ sceneResolution.x, sceneResolution.y });

        },
        [this, sceneResolution](GPU::RenderGraphRegistry* registry, const Parameter& params, GPU::RenderCommandBucket* commandBucket) -> void {

            GPU::PipelineStateDesc pipelineDesc;
            pipelineDesc.inputBindings[0].stride = sizeof(Vec2f);
            pipelineDesc.inputAttributes[0] = {
                0, 0
            };
            pipelineDesc.inputLayout.topology = GPU::Topology::TRIANGLE_STRIP;
            pipelineDesc.viewport = { 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
            pipelineDesc.scissor = { false, 0, 0, uint16(sceneResolution.x), uint16(sceneResolution.y) };
            pipelineDesc.colorAttachmentCount = 1;
            pipelineDesc.programID = _programID;
            pipelineDesc.raster.cullMode = GPU::CullMode::NONE;
            GPU::PipelineStateID pipelineStateID = registry->getPipelineState(pipelineDesc);

            GPU::Descriptor set0Descriptors[4] = {};
            for (int i = 0; i < 4; i++) {
                GPU::Descriptor& descriptor = set0Descriptors[i];
                descriptor = GPU::Descriptor::InputAttachment(registry->getTexture(params.renderMap[i]), GPU::SHADER_STAGE_FRAGMENT);
            }
            GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, { 4, set0Descriptors });

            commandBucket->reserve(1);
            using DrawCommand = GPU::RenderCommandDrawVertex;
            DrawCommand* command = commandBucket->put<DrawCommand>(0, 0);
            command->pipelineStateID = pipelineStateID;
            command->vertexBufferID = registry->getBuffer(params.vertexBuffer);
            command->vertexCount = 4;
            command->shaderArgSetIDs[0] = set0;
        });
    return passParameter;
}