#pragma once

#include "data.h"
#include "../../data.h"
#include "render_module/shadow_map.h"
#include "render_module/lighting_pass.h"
#include "render_module/structure_pass.h"
#include "render_module/depth_mipmap.h"

#include <chrono>

using namespace soul;

namespace SoulFila {

	struct BindingPoint {
		uint8 set;
		uint8 binding;
	};

	static constexpr BindingPoint FRAME_UNIFORM_BINDING_POINT = { 0, 0 };
	static constexpr BindingPoint LIGHT_UNIFORM_BINDING_POINT = { 0, 1 };
	static constexpr BindingPoint SHADOW_UNIFORM_BINDING_POINT = { 0, 2 };
	static constexpr BindingPoint FROXEL_RECORD_UNIFORM_BINDING_POINT = { 0, 3 };

	static constexpr uint8 FRAME_SAMPLER_SET = 0;
	static constexpr uint8 FRAME_SAMPLER_START_BINDING = 4;

	static constexpr BindingPoint MATERIAL_UNIFORM_BINDING_POINT = { 1, 0 };

	static constexpr uint8 MATERIAL_SAMPLER_SET = 2;

	static constexpr BindingPoint RENDERABLE_UNIFORM_BINDING_POINT = { 3, 0 };
	static constexpr BindingPoint RENDERABLE_BONE_UNIFORM_BINDING_POINT = { 3, 1 };

	class Renderer : public Demo::Renderer {
	public:

		explicit Renderer(gpu::System* gpuSystem) : 
			gpuSystem(gpuSystem), programRegistry(soul::runtime::get_context_allocator(), gpuSystem), 
			scene(gpuSystem, &programRegistry), epoch(std::chrono::steady_clock::now()) {}

		virtual void init();
		virtual Demo::Scene* getScene() { return (Demo::Scene*)&scene; }
		virtual gpu::TextureNodeID computeRenderGraph(gpu::RenderGraph* renderGraph);

	private:

		void prepareRenderData();

		RenderData renderData;
		gpu::System* gpuSystem;
		GPUProgramRegistry programRegistry;
		ShadowMapGenPass shadowMapPass;
		LightingPass lightingPass;
		StructurePass structurePass;
		DepthMipmapPass depthMipmapPass;
		
		Scene scene;

		using clock = std::chrono::steady_clock;
		using Epoch = clock::time_point;
		using duration = clock::duration;

		Epoch epoch;
	};

	void Cull(Renderables& renderables, const Frustum& frustum, soul_size bit);
}
