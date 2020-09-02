#pragma once

#include "core/type.h"
#include "gpu/gpu.h"
#include "utils.h"

#include "render_pipeline/deferred/data.h"

using namespace Soul;

class ShadowMapGenRenderModule
{
private:
	GPU::ShaderID _vertShaderID;
	GPU::ShaderID _fragShaderID;

public:
	struct Parameter
	{
		GPU::BufferNodeID modelBuffer;
		GPU::BufferNodeID shadowMatrixesBuffer;
		Array<GPU::BufferNodeID> vertexBuffers;
		Array<GPU::BufferNodeID> indexBuffers;
		GPU::TextureNodeID depthTarget;
	};
	
	void init(GPU::System* system);

	Parameter addPass(GPU::System* system, GPU::RenderGraph* renderGraph, const Parameter& data, const DeferredPipeline::Scene& scene);
	
};