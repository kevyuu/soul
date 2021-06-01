#include "renderer.h"
#include "core/dev_util.h"

using namespace Soul;

namespace SoulFila {

	GPU::TextureNodeID Renderer::computeRenderGraph(GPU::RenderGraph* renderGraph) {
		Vec2ui32 sceneResolution = _scene.getViewport();

		GPU::RGTextureDesc renderTargetDesc;
		renderTargetDesc.width = sceneResolution.x;
		renderTargetDesc.height = sceneResolution.y;
		renderTargetDesc.depth = 1;
		renderTargetDesc.clear = true;
		renderTargetDesc.clearValue = {};
		renderTargetDesc.format = GPU::TextureFormat::RGBA8;
		renderTargetDesc.mipLevels = 1;
		renderTargetDesc.type = GPU::TextureType::D2;
		GPU::TextureNodeID finalRenderTarget = renderGraph->createTexture("FinalRender Target", renderTargetDesc);
		

		return finalRenderTarget;
	}
}