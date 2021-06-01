#pragma once

#include "data.h"
#include "../../data.h"

using namespace Soul;

namespace SoulFila {

	struct BindingPoint {
		uint8 set;
		uint8 binding;
	};

	static constexpr BindingPoint FRAME_UNIFORM_BINDING_POINT = { 0, 0 };
	static constexpr BindingPoint LIGHT_UNIFORM_BINDING_POINT = { 0, 1 };
	static constexpr BindingPoint SHADOW_UNIFORM_BINDING_POINT = { 0, 2 };

	static constexpr uint8 FRAME_SAMPLER_SET = 0;
	static constexpr uint8 FRAME_SAMPLER_START_BINDING = 3;

	static constexpr BindingPoint MATERIAL_UNIFORM_BINDING_POINT = { 1, 0 };

	static constexpr uint8 MATERIAL_SAMPLER_SET = 2;


	static constexpr BindingPoint RENDERABLE_UNIFORM_BINDING_POINT = { 3, 0 };
	static constexpr BindingPoint RENDERABLE_BONE_UNIFORM_BINDING_POINT = { 3, 1 };


	class Renderer : public Demo::Renderer {
	public:

		explicit Renderer(GPU::System* gpuSystem) : 
			_gpuSystem(gpuSystem), _programRegistry(Soul::Runtime::GetContextAllocator(), gpuSystem), 
			_scene(gpuSystem, &_programRegistry) {}

		virtual void init();
		virtual Demo::Scene* getScene() { return (Demo::Scene*)&_scene; }
		virtual GPU::TextureNodeID computeRenderGraph(GPU::RenderGraph* renderGraph);

	private:
		GPU::System* _gpuSystem;
		GPUProgramRegistry _programRegistry;
		Scene _scene;
		Soul::GPU::TextureID _stubTexture;
	};
}
