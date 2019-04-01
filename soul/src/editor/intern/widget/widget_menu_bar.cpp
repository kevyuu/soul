#include "extern/imgui.h"
#include "extern/imguifilesystem.h"

#include "core/debug.h"

#include "editor/data.h"
#include "editor/intern/action.h"

namespace Soul {
	namespace Editor {

		void MenuBar::tick(Database* db) {
			
			enum ACTION {
				ACTION_NONE,
				ACTION_IMPORT_GLTF,

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
				else if (ImGui::BeginMenu("Voxelize")) {
					db->world.renderSystem.voxelGIVoxelize();
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (ImGui::BeginPopupModal("Import GLTF", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				const bool browseGLTFFile = ImGui::Button("Browse##gltf");
				ImGui::SameLine();
				ImGui::InputText("GLTF File", gltfFilePath, sizeof(gltfFilePath));
				ImGui::Checkbox("Set mesh position to aabb center", &setMeshPositionToAABBCenter);
				static ImGuiFs::Dialog dlg;
				const char* gltfChosenPath = dlg.chooseFileDialog(browseGLTFFile);
				if (strlen(gltfChosenPath) > 0) {
					SOUL_ASSERT(0, strlen(gltfChosenPath) < sizeof(gltfFilePath), "File path too long");
					strcpy(gltfFilePath, gltfChosenPath);
				}

				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ActionImportGLTFAsset(&db->world, gltfFilePath, setMeshPositionToAABBCenter);
					setMeshPositionToAABBCenter = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); setMeshPositionToAABBCenter = false; }
				ImGui::EndPopup();

			}

			switch (action) {
			case ACTION_NONE:
				break;
			case ACTION_IMPORT_GLTF:
				ImGui::OpenPopup("Import GLTF");
				break;
			default:
				SOUL_ASSERT(0, false, "Invalid menu bar action");
				break;
			}

		}

	}
}