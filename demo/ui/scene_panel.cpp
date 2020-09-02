#include "panels.h"
#include "data.h"
namespace Demo {
	namespace UI {

		void ScenePanel::render(Store* store) {
			ImGui::Begin(name);
			ImVec2 sceneWindowSize = ImGui::GetWindowSize();
			float sceneAspectRatio = sceneResolution.x / (float)sceneResolution.y;
			ImVec2 sceneImageSize = { fmin(sceneWindowSize.x, sceneAspectRatio * sceneWindowSize.y), fmin(sceneWindowSize.y, sceneWindowSize.x / sceneAspectRatio) };
			ImGui::SetCursorPos(ImVec2((sceneWindowSize.x - sceneImageSize.x) * 0.5f, (sceneWindowSize.y - sceneImageSize.y) * 0.5f));
			ImGui::Image(textureID, sceneImageSize);
			ImGui::End();
		}
	}
}