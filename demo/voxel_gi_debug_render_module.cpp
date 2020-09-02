#include "voxel_gi_debug_render_module.h"

using namespace Soul;

void VoxelGIDebugRenderModule::init(GPU::System* system) {
    const char* vertSrc = LoadFile("shaders/voxel_gi_debug.vert.glsl");
    GPU::ShaderDesc vertShaderDesc;
    vertShaderDesc.name = "Voxel GI debug vertex shader";
    vertShaderDesc.source = vertSrc;
    vertShaderDesc.sourceSize = strlen(vertSrc);
    _vertShaderID = system->shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

    const char* geomSrc = LoadFile("shaders/voxel_gi_debug.geom.glsl");
    GPU::ShaderDesc geomShaderDesc;
    geomShaderDesc.name = "Voxel GI debug geometry shader";
    geomShaderDesc.source = geomSrc;
    geomShaderDesc.sourceSize = strlen(geomSrc);
    _geomShaderID = system->shaderCreate(geomShaderDesc, GPU::ShaderStage::GEOMETRY);

    const char* fragSrc = LoadFile("shaders/voxel_gi_debug.frag.glsl");
    GPU::ShaderDesc fragShaderDesc;
    fragShaderDesc.name = "Voxel GI debug fragment shader";
    fragShaderDesc.source = fragSrc;
    fragShaderDesc.sourceSize = strlen(fragSrc);
    _fragShaderID = system->shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);
}

VoxelGIDebugRenderModule::Parameter VoxelGIDebugRenderModule::addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& inputParams, const DeferredPipeline::Scene& scene) {
    return renderGraph->addGraphicPass<Parameter>(
        "Voxel debug pass",
        [this, &inputParams, scene]
    (GPU::GraphicNodeBuilder* builder, Parameter* params) -> void {

            params->voxelGIData = builder->addInShaderBuffer(inputParams.voxelGIData, 0, 0);
            params->cameraData = builder->addInShaderBuffer(inputParams.cameraData, 0, 1);
            params->voxelLight = builder->addInShaderTexture(inputParams.voxelLight, 0, 2);
            params->voxelAlbedo = builder->addInShaderTexture(inputParams.voxelAlbedo, 0, 3);
            params->voxelNormal = builder->addInShaderTexture(inputParams.voxelNormal, 0, 4);
            params->voxelEmissive = builder->addInShaderTexture(inputParams.voxelEmissive, 0, 5);

            GPU::ColorAttachmentDesc colorAttchDesc;
            colorAttchDesc.blendEnable = false;
            colorAttchDesc.clear = true;
            colorAttchDesc.clearValue.color.float32 = { 1.0f, 0.0f, 0.0f, 1.0f };
            params->renderTarget = builder->addColorAttachment(inputParams.renderTarget, colorAttchDesc);

            GPU::DepthStencilAttachmentDesc depthAttchDesc;
            depthAttchDesc.clear = true;
            depthAttchDesc.clearValue.depthStencil = { 1.0f, 0 };
            depthAttchDesc.depthWriteEnable = true;
            depthAttchDesc.depthTestEnable = true;
            depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
            params->depthTarget = builder->setDepthStencilAttachment(inputParams.depthTarget, depthAttchDesc);

            const DeferredPipeline::Camera& camera = scene.camera;
            Vec2ui32 reso = { camera.viewportWidth, camera.viewportHeight };
            GPU::GraphicPipelineConfig pipelineConfig;
            pipelineConfig.inputLayout.topology = GPU::Topology::POINT_LIST;
            pipelineConfig.viewport = { 0, 0, uint16(reso.x), uint16(reso.y) };
            pipelineConfig.scissor = { false, 0, 0, uint16(reso.x), uint16(reso.y) };
            pipelineConfig.framebuffer = { uint16(reso.x), uint16(reso.y) };
            pipelineConfig.vertexShaderID = this->_vertShaderID;
            pipelineConfig.fragmentShaderID = this->_fragShaderID;
            pipelineConfig.geometryShaderID = this->_geomShaderID;
            pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

            builder->setPipelineConfig(pipelineConfig);

        },
        [scene, system]
        (GPU::RenderGraphRegistry* registry, const Parameter& params, GPU::CommandBucket* commandBucket) {
            GPU::Descriptor set0Descriptors[6] = {
                GPU::Descriptor::Uniform(registry->getBuffer(params.voxelGIData), 0),
                GPU::Descriptor::Uniform(registry->getBuffer(params.cameraData), 0),
                GPU::Descriptor::StorageImage(registry->getTexture(params.voxelLight), 0),
                GPU::Descriptor::StorageImage(registry->getTexture(params.voxelAlbedo), 0),
                GPU::Descriptor::StorageImage(registry->getTexture(params.voxelNormal), 0),
                GPU::Descriptor::StorageImage(registry->getTexture(params.voxelEmissive), 0),
            };

            GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, { 6, set0Descriptors });

            uint32 voxelReso = scene.voxelGIConfig.resolution;
            commandBucket->reserve(1);
            using DrawCommand = GPU::Command::DrawVertex;
            DrawCommand* command = commandBucket->put<DrawCommand>(0, 0);
            command->vertexBufferID = GPU::BUFFER_ID_NULL;
            command->vertexCount = voxelReso * voxelReso * voxelReso;
            command->shaderArgSets[0] = set0;

        });
}