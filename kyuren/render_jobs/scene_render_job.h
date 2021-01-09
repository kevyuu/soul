#pragma once
#include "../data.h"
#include "gpu/data.h"

class SceneRenderJob : public KyuRen::RenderJob {
private:
	const KyuRen::Scene* _scene;
	Soul::GPU::System* _gpuSystem;

public:

	void init(const KyuRen::Scene*, Soul::GPU::System*);
	void execute(Soul::GPU::RenderGraph* renderGraph, const KyuRen::RenderJobInputs&, KyuRen::RenderJobOutputs&);

};