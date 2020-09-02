#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"
#include "render_pipeline/deferred/data.h"

using namespace Soul;

class VoxelizeRenderModule {
private:
    GPU::ShaderID _vertShaderID;
    GPU::ShaderID _fragShaderID;
    GPU::ShaderID _geomShaderID;

public:
    struct Parameter
    {
        GPU::TextureNodeID stubTexture;
        Array<GPU::BufferNodeID> vertexBuffers;
        Array<GPU::BufferNodeID> indexBuffers;
        GPU::BufferNodeID model;
        GPU::BufferNodeID rotation;
        GPU::BufferNodeID voxelGIData;
        GPU::TextureNodeID voxelAlbedo;
        GPU::TextureNodeID voxelNormal;
        GPU::TextureNodeID voxelEmissive;
        GPU::BufferNodeID material;
        Array<GPU::TextureNodeID> materialTextures;
        GPU::BufferNodeID voxelizeMatrixes;
    };

    void init(GPU::System* system);

    Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& inputParams, const DeferredPipeline::Scene& scene);
};