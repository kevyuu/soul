#include "panels.h"
#include "data.h"
namespace Demo {
	namespace UI {

		bool ScenePanel::render(Store* store) {
			bool dummy = true;
			ImGui::Begin(name, &dummy, ImGuiWindowFlags_NoScrollbar);
			ImVec2 sceneWindowSize = ImGui::GetWindowSize();
			float sceneAspectRatio = sceneResolution.x / (float)sceneResolution.y;
			ImVec2 sceneImageSize = { fmin(sceneWindowSize.x, sceneAspectRatio * sceneWindowSize.y), fmin(sceneWindowSize.y, sceneWindowSize.x / sceneAspectRatio) };
			ImGui::SetCursorPos(ImVec2((sceneWindowSize.x - sceneImageSize.x) * 0.5f, (sceneWindowSize.y - sceneImageSize.y) * 0.5f));

			if (!SoulImTexture(textureID).getTextureNodeID().is_null())
			{
				ImGui::Image(textureID, sceneImageSize);
			}
			bool isMouseHovered = ImGui::IsWindowHovered();
			ImGui::End();
			return isMouseHovered;
		}
	}
}