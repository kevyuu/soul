#include "menu_bar.h"
#include "core/dev_util.h"

#include "../data.h"
#include "data.h"
#include "imguifilesystem.h"

namespace Demo {
	namespace UI {
		void MenuBar::render(Store* store) {
			enum ACTION {
				ACTION_NONE,
				ACTION_IMPORT_GLTF,

				ACTION_EDIT_UI_STYLE,

				ACTION_COUNT
			};

			ACTION action = ACTION_NONE;

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::BeginMenu("Import")) {
						if (ImGui::MenuItem("Import GLTF")) {
							action = ACTION_IMPORT_GLTF;
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				else if (ImGui::BeginMenu("Setting")) {
					if (ImGui::MenuItem("Edit UI Style")) {
						action = ACTION_EDIT_UI_STYLE;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (ImGui::BeginPopupModal("Import GLTF", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				const bool browseGLTFFile = ImGui::Button("Browse##gltf");
				ImGui::SameLine();
				ImGui::InputText("GLTF File", gltfFilePath, sizeof(gltfFilePath));
				static ImGuiFs::Dialog dlg;
				const char* gltfChosenPath = dlg.chooseFileDialog(browseGLTFFile);
				if (strlen(gltfChosenPath) > 0) {
					SOUL_ASSERT(0, strlen(gltfChosenPath) < sizeof(gltfFilePath), "File path too long");
					strcpy(gltfFilePath, gltfChosenPath);
				}

				if (ImGui::Button("OK", ImVec2(120, 0))) {
					store->scene->importFromGLTF(gltfFilePath);
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				ImGui::EndPopup();

			}

			if (ImGui::BeginPopupModal("Edit UI Style", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				auto& style = ImGui::GetStyle();
				ImGui::EndPopup();
			}

			switch (action) {
			case ACTION_NONE:
				break;
			case ACTION_IMPORT_GLTF:
				ImGui::OpenPopup("Import GLTF");
				break;
			case ACTION_EDIT_UI_STYLE:
				ImGui::OpenPopup("Edit UI Style");
				break;
			default:
				SOUL_ASSERT(0, false, "Invalid menu bar action");
				break;
			}

		}
	}
}