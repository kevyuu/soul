#include "panels.h"
#include "data.h"

namespace Demo {
	namespace UI {

		void MetricPanel::render(Store* store) {
			if (ImGui::Begin("Metric"))
			{
				ImGuiIO& io = ImGui::GetIO();
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
				ImGui::End();
			}
		}
	}
}
