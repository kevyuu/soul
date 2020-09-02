#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"

#include "render_pipeline/deferred/data.h"

using namespace Soul;

class GBufferGenRenderModule
{
private:
    GPU::ShaderID _vertShaderID;
    GPU::ShaderID _fragShaderID;

public:
    struct Parameter
    {
        Array<GPU::BufferNodeID> vertexBuffers;
        Array<GPU::BufferNodeID> indexBuffers;
        Array<GPU::TextureNodeID> sceneTextures;
        GPU::BufferNodeID camera;
        GPU::BufferNodeID light;
        GPU::BufferNodeID material;
        GPU::BufferNodeID model;
        GPU::TextureNodeID renderTargets[4];
        GPU::TextureNodeID depthTarget;
        GPU::TextureNodeID shadowMap;
        GPU::TextureNodeID stubTexture;

        ~Parameter() {
            sceneTextures.cleanup();
            indexBuffers.cleanup();
            vertexBuffers.cleanup();
        }
    };

    void init(GPU::System* system);
    Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& inputParams, const DeferredPipeline::Scene& scene);

};