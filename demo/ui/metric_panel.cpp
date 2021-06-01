#include "panels.h"
#include "data.h"

namespace Demo {
	namespace UI {

		void MetricPanel::render(Store* store) {
			ImGui::Begin("Metric");
			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::Text("Imgui time %.3lf", ImGui::GetTime());
			ImGui::Text("Imgui delta %.3lf", io.DeltaTime);
			ImGui::End();
		}
	}
}
