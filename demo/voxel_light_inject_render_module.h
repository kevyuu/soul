#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"

#include "render_pipeline/deferred/data.h"

using namespace Soul;

class VoxelLightInjectRenderModule {
private:
    GPU::ShaderID _compShaderID;
public:

    struct Parameter {
        GPU::TextureNodeID voxelAlbedo;
        GPU::TextureNodeID voxelNormal;
        GPU::TextureNodeID voxelEmissive;
        GPU::TextureNodeID voxelLight;

        GPU::BufferNodeID voxelGIData;
        GPU::BufferNodeID lightData;
    };

    void init(GPU::System* system);

    Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& data, const DeferredPipeline::Scene& scene);
};