#pragma once

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "core/dev_util.h"

#include "gpu/gpu.h"

namespace Demo {
	using namespace Soul;

	struct Input {

		static constexpr uint32 MOUSE_BUTTON_LEFT = ImGuiMouseButton_Left;
		static constexpr uint32 MOUSE_BUTTON_RIGHT = ImGuiMouseButton_Right;
		static constexpr uint32 MOUSE_BUTTON_MIDDLE = ImGuiMouseButton_Middle;
		static constexpr uint32 MOUSE_BUTTON_COUNT = ImGuiMouseButton_COUNT;

		static constexpr uint32 KEY_GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT;

		bool mouseDown[5] = {};
		bool mouseDragging[5] = {};
		float mouseWheel = 0.0f;
		Soul::Vec2f mouseDelta;

		bool keyCtrl = false;
		bool keyShift = false;
		bool keyAlt = false;
		bool keySuper = false;
		bool keysDown[512] = {};

		float deltaTime = 0.0f;

	};

	class Scene {	
	public:
		Scene() = default;
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;
		virtual ~Scene() = default;

		virtual void importFromGLTF(const char* path) = 0;
		virtual void cleanup() = 0;
		virtual bool update(const Input& input) = 0;
		virtual void renderPanels() = 0;
		virtual Vec2ui32 getViewport() = 0;
		virtual void setViewport(Vec2ui32 viewport) = 0;
	};

	class Renderer {
	public:
		Renderer() = default;
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;
		virtual ~Renderer() = default;

		virtual void init() = 0;
		virtual Scene* getScene() = 0;
		virtual GPU::TextureNodeID computeRenderGraph(GPU::RenderGraph* renderGraph) = 0;
	};

}
