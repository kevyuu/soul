#pragma once

#include "data.h"
#include "../../data.h"

#include "../../shadow_map_gen_render_module.h"
#include "../../final_gather_render_module.h"
#include "../../voxelize_render_module.h"
#include "../../voxel_gi_debug_render_module.h"
#include "../../gbuffer_gen_render_module.h"
#include "../../voxel_light_inject_render_module.h"

using namespace Soul;

namespace DeferredPipeline {
	class Renderer : public Demo::Renderer {
	public:

		Renderer(GPU::System* gpuSystem) : _gpuSystem(gpuSystem), _scene(gpuSystem) {}
		
		virtual void init();
		virtual Demo::Scene* getScene() { return (Demo::Scene*) & _scene; }
		virtual GPU::TextureNodeID computeRenderGraph(GPU::RenderGraph* renderGraph);

	private:
		Scene _scene;
		GPU::System* _gpuSystem;

		ShadowMapGenRenderModule shadowMapGenRenderModule;
		GBufferGenRenderModule gBufferGenRenderModule;
		FinalGatherRenderModule finalGatherRenderModule;
		VoxelizeRenderModule voxelizeRenderModule;
		VoxelGIDebugRenderModule voxelGIDebugRenderModule;
		VoxelLightInjectRenderModule voxelLightInjectRenderModule;

		GPU::TextureID stubTexture;
		GPU::BufferID quadBuffer;
	};
}
