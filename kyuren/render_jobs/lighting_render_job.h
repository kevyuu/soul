#pragma once
#include "../data.h"
#include "gpu/data.h"

class LightingRenderJob : public KyuRen::RenderJob {
private:
	Soul::GPU::ShaderID _vertShaderID;
	Soul::GPU::ShaderID _fragShaderID;
	const KyuRen::Scene* _scene;
	Soul::GPU::System* _gpuSystem;

public:

	void init(const KyuRen::Scene*, Soul::GPU::System*);
	void execute(Soul::GPU::RenderGraph* renderGraph, const KyuRen::RenderJobInputs&, KyuRen::RenderJobOutputs&);
};