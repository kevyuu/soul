#pragma once

#include <imgui/imgui.h>
#include "imgui_impl_glfw.h"

#include "ui/data.h"
#include "../data.h"

class GLFWwindow;

namespace Demo {

	class Scene;

	namespace UI {

		void _ApplyTheme() {
			ImGuiStyle& style = ImGui::GetStyle();
			ImVec4* colors = style.Colors;

			static constexpr float WINDOW_BACKGROUND = 35.0f / 255.0f;
			static constexpr float CHILD_BG = 40.0f / 255.0f;
			static constexpr float TEXT = 232.0f / 255.0f;
			static constexpr float TEXT_DISABLED = 192.0f / 255.0f;
			static constexpr float BORDER = 30.0f / 255.0f;
			static constexpr float FRAME_BG = 30.0f / 255.0f;
			static constexpr float FRAME_BG_HOVERED = 35.0f / 255.0f;
			static constexpr float TITLE_BG = 35.0f / 255.0f;
			static constexpr float TITLE_BG_ACTIVE = 40.0f / 255.0f;


			static const ImVec4 SELECTABLE_BG_ACTIVE = ImVec4(59.0f / 255.0f, 86.0f / 255.0f, 137.0f / 255.0f, 1.0f);
			static const ImVec4 SELECTABLE_BG_HOVERED = ImVec4(106.0f / 255.0f, 106.0f / 255.0f, 106.0f / 255.0f, 1.0f);
			static const ImVec4 SELECTABLE_BG = ImVec4(89.0f / 255.0f, 89.0f / 255.0f, 89.0f / 255.0f, 1.0f);

			colors[ImGuiCol_Text] = ImVec4(TEXT, TEXT, TEXT, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(TEXT_DISABLED, TEXT_DISABLED, TEXT_DISABLED, 1.00f);
			colors[ImGuiCol_ChildBg] = ImVec4(CHILD_BG, CHILD_BG, CHILD_BG, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(WINDOW_BACKGROUND, WINDOW_BACKGROUND, WINDOW_BACKGROUND, 1.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colors[ImGuiCol_Border] = ImVec4(BORDER, BORDER, BORDER, 1.0f);
			colors[ImGuiCol_BorderShadow] = ImVec4(BORDER, BORDER, BORDER, 1.0f);
			colors[ImGuiCol_FrameBg] = ImVec4(FRAME_BG, FRAME_BG, FRAME_BG, 0.54f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(FRAME_BG_HOVERED, FRAME_BG_HOVERED, FRAME_BG_HOVERED, 0.40f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
			colors[ImGuiCol_TitleBg] = ImVec4(TITLE_BG, TITLE_BG, TITLE_BG, 1.00f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(TITLE_BG_ACTIVE, TITLE_BG_ACTIVE, TITLE_BG_ACTIVE, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(TITLE_BG, TITLE_BG, TITLE_BG, 1.000f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
			colors[ImGuiCol_Button] = SELECTABLE_BG;
			colors[ImGuiCol_ButtonHovered] = SELECTABLE_BG_HOVERED;
			colors[ImGuiCol_ButtonActive] = SELECTABLE_BG_ACTIVE;

			colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
			colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);

			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

			style.PopupRounding = 3;

			style.WindowPadding = ImVec2(6, 4);
			style.FramePadding = ImVec2(6, 4);
			style.ItemSpacing = ImVec2(6, 4);
			style.ItemInnerSpacing = ImVec2(6, 4);

			style.ScrollbarSize = 18;

			style.WindowBorderSize = 1;
			style.ChildBorderSize = 1;
			style.PopupBorderSize = 1;
			style.FrameBorderSize = false;

			style.WindowRounding = 0;
			style.ChildRounding = 4;
			style.FrameRounding = 4;
			style.ScrollbarRounding = 4;
			style.GrabRounding = 4;

#ifdef IMGUI_HAS_DOCK 
			style.TabBorderSize = false;
			style.TabRounding = 6;

			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
			colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
			colors[ImGuiCol_TabActive] = ImVec4(66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.00f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(43.0f / 255.0f, 43.0f / 255.0f, 43.0f / 255.0f, 1.00f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(66.0f / 255.0f, 66.0f / 255.0f, 66.0f / 255.0f, 1.00f);
			
			colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}
#endif
		}

		void Init(GLFWwindow* window) {

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			_ApplyTheme();

			ImGui_ImplGlfw_InitForVulkan(window, true);
		}

		void _DockBegin() {
			ImGuiIO& io = ImGui::GetIO();
			ImGui::SetNextWindowPos(ImVec2(0, 20));
			ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - 20));
			ImGuiWindowFlags leftDockWindowFlags = ImGuiWindowFlags_None;
			leftDockWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			leftDockWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			ImGui::Begin("Left Dock", nullptr, leftDockWindowFlags);
			ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
			ImGuiID dockspace_id = ImGui::GetID("Left Dock");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		void _DockEnd() {
			ImGui::End();
		}

		void Render(Store* store) {
			// Start the Dear ImGui frame
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGuiContext* g = ImGui::GetCurrentContext();

			ImGui::GetIO().Fonts->TexID = store->fontTex.getImTextureID();

			store->menuBar.render(store);
			
			_DockBegin();


			Vec2ui32 sceneResolution = store->scene->getViewport();
			store->scenePanel.setResolution(sceneResolution);
			store->scenePanel.setTexture(store->sceneTex.getImTextureID());
			bool isMouseHoverScene = store->scenePanel.render(store);

			store->metricPanel.render(store);
			store->scene->renderPanels();
			
			ImGui::ShowDemoWindow();

			_DockEnd();

			Input input = {};
			const ImGuiIO& io = ImGui::GetIO();
			input.deltaTime = io.DeltaTime;

			if (!io.WantCaptureKeyboard) {
				input.keyAlt = io.KeyAlt;
				input.keyCtrl = io.KeyCtrl;
				input.keyShift = io.KeyShift;
				input.keySuper = io.KeySuper;
				memcpy(input.keysDown, io.KeysDown, sizeof(io.KeysDown));
			}

			if (isMouseHoverScene) {
				memcpy(input.mouseDown, io.MouseDown, sizeof(io.MouseDown));
				input.mouseWheel = io.MouseWheel;
				input.mouseDelta = Soul::Vec2f(io.MouseDelta.x, io.MouseDelta.y);
				for (uint64 buttonIdx = 0; buttonIdx < Input::MOUSE_BUTTON_COUNT; buttonIdx++) {
					input.mouseDragging[buttonIdx] = ImGui::IsMouseDragging(buttonIdx);
				}
			}

			if (store->scene != nullptr) {
				store->scene->update(input);
			}

			ImGui::Render();

		}

	}
}
