#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"

#include "render_pipeline/deferred/data.h"

using namespace Soul;

class VoxelGIDebugRenderModule {
private:
    GPU::ShaderID _vertShaderID;
    GPU::ShaderID _fragShaderID;
    GPU::ShaderID _geomShaderID;

public:
    struct Parameter
    {
        GPU::BufferNodeID voxelGIData;
        GPU::BufferNodeID cameraData;
        GPU::TextureNodeID voxelAlbedo;
        GPU::TextureNodeID voxelNormal;
        GPU::TextureNodeID voxelEmissive;
        GPU::TextureNodeID voxelLight;
        GPU::TextureNodeID renderTarget;
        GPU::TextureNodeID depthTarget;
    };

    void init(GPU::System* system);
    Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& inputParams, const DeferredPipeline::Scene& scene);
};