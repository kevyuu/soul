#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"

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

    void init(GPU::System* system);
    Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& parameter, Vec2ui32 sceneResolution);

};