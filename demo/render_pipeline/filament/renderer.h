#pragma once

#include "data.h"
#include "../../data.h"
#include "render_module/shadow_map.h"
#include "render_module/lighting_pass.h"
#include "render_module/structure_pass.h"
#include "render_module/depth_mipmap.h"

#include <chrono>

using namespace soul;

namespace soul_fila {

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
			gpu_system_(gpuSystem), program_registry_(soul::runtime::get_context_allocator(), gpuSystem), 
			scene_(gpuSystem, &program_registry_), epoch(std::chrono::steady_clock::now()) {}

		virtual void init();
		virtual Demo::Scene* getScene() { return (Demo::Scene*)&scene_; }
		virtual gpu::TextureNodeID computeRenderGraph(gpu::RenderGraph* renderGraph);

	private:

		void prepare_render_data();

		RenderData render_data_;
		gpu::System* gpu_system_;
		GPUProgramRegistry program_registry_;
		ShadowMapGenPass shadow_map_pass_;
		LightingPass lighting_pass_;
		StructurePass structure_pass_;
		DepthMipmapPass depth_mipmap_pass_;
		
		Scene scene_;

		using clock = std::chrono::steady_clock;
		using Epoch = clock::time_point;
		using duration = clock::duration;

		Epoch epoch;
	};

	void Cull(Renderables& renderables, const Frustum& frustum, soul_size bit);
}
