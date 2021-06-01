#pragma once

#include "data.h"
#include "../../data.h"

using namespace Soul;

namespace SoulFila {

	namespace BindingPoints {
		constexpr uint8 PER_VIEW = 0;    // uniforms/samplers updated per view
		constexpr uint8 PER_RENDERABLE = 1;    // uniforms/samplers updated per renderable
		constexpr uint8 PER_RENDERABLE_BONES = 2;    // bones data, per renderable
		constexpr uint8 LIGHTS = 3;    // lights data array
		constexpr uint8 SHADOW = 4;    // punctual shadow data
		constexpr uint8 PER_MATERIAL_INSTANCE = 5;    // uniforms/samplers updates per material
		constexpr uint8 COUNT = 6;
		// These are limited by Program::UNIFORM_BINDING_COUNT (currently 6)
	}

	static constexpr uint8 MATERIAL_UNIFORM_SET = 0;
	static constexpr uint8 MATERIAL_SAMPLER_SET = 1;

	class Renderer : public Demo::Renderer {
	public:

		explicit Renderer(GPU::System* gpuSystem) : _gpuSystem(gpuSystem), _scene(gpuSystem) {}

		virtual void init();
		virtual Demo::Scene* getScene() { return (Demo::Scene*)&_scene; }
		virtual GPU::TextureNodeID computeRenderGraph(GPU::RenderGraph* renderGraph);

	private:
		Scene _scene;
		GPU::System* _gpuSystem;
	};
}
