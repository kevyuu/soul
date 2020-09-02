#pragma once

#include "core/math.h"
#include "core/array.h"
#include "core/dev_util.h"

#include "gpu/gpu.h"

using namespace Soul;
namespace Demo {

	class Scene {	
	public:
		virtual void importFromGLTF(const char* path) = 0;
		virtual void cleanup() = 0;
		virtual bool handleInput() = 0;
		virtual Vec2ui32 getViewport() = 0;
		virtual void setViewport(Vec2ui32 viewport) = 0;
	};

	class Renderer {
	public:
		virtual void init() = 0;
		virtual Scene* getScene() = 0;
		virtual GPU::TextureNodeID computeRenderGraph(GPU::RenderGraph* renderGraph) = 0;
	};

}
