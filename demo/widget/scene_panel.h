#pragma once

#include "core/type.h"
#include "core/math.h"

#include "imgui/imgui.h"

struct ScenePanel {

	Soul::Vec2ui32 sceneResolution;

	ScenePanel(Soul::Vec2ui32 sceneResoultion): sceneResolution(sceneResoultion) {}

	void update(const char* name, ImTextureID sceneTextureID) {
		ImGui::Begin(name);
		ImVec2 sceneWindowSize = ImGui::GetWindowSize();
		float sceneAspectRatio = sceneResolution.x / (float) sceneResolution.y;
		ImVec2 sceneImageSize = { fmin(sceneWindowSize.x, sceneAspectRatio * sceneWindowSize.y), fmin(sceneWindowSize.y, sceneWindowSize.x / sceneAspectRatio) };
		ImGui::SetCursorPos(ImVec2((sceneWindowSize.x - sceneImageSize.x) * 0.5f, (sceneWindowSize.y - sceneImageSize.y) * 0.5f));
		ImGui::Image(sceneTextureID, sceneImageSize);
		ImGui::End();
	}

};