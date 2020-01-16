#include "core/dev_util.h"
#include "core/math.h"

#include "editor/data.h"
#include "editor/system.h"
#include "editor/intern/entity.h"

#include "extern/imgui.h"
#include "extern/icon/IconsIonicons.h"
#include "extern/icon/IconsMaterialDesign.h"

#include <cstdio>

namespace Soul {
	namespace Editor {
		
		static void _drawEntityNodeRecursive(Entity* entity, EntityID* selectedEntityID) {
			
			ImGuiTreeNodeFlags nodeFlags = (entity->entityID == *selectedEntityID) ? ImGuiTreeNodeFlags_Selected : 0;
			nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

			char treeNodeLabel[sizeof(entity->name) + 10];
			const char* logo = nullptr;
			switch (entity->entityID.type) {
				case EntityType_GROUP: {
					logo = ICON_II_FOLDER;
					break;
				}
				case EntityType_MESH: {
					logo = ICON_II_CUBE;
					break;
				}
				case EntityType_DIRLIGHT: {
					logo = ICON_II_ANDROID_SUNNY;
					break;
				}
				case EntityType_POINTLIGHT: {
					logo = ICON_II_LIGHTBULB;
					break;
				}
				case EntityType_SPOTLIGHT: {
					logo = ICON_MD_HIGHLIGHT;
					break;
				}
			}

			sprintf(treeNodeLabel, "%s %s##%d:%d", logo, entity->name, entity->entityID.type, entity->entityID.index);

			if (entity->entityID.type == EntityType_GROUP) {
				
				bool node_open = ImGui::TreeNodeEx(treeNodeLabel, nodeFlags);
				if (ImGui::IsItemClicked()) {
					*selectedEntityID = entity->entityID;
				}
				if (node_open) {
					if (entity->entityID.type == EntityType_GROUP) {
						GroupEntity* groupEntity = (GroupEntity*)entity;
						Entity* child = groupEntity->first;
						while (child != nullptr) {
							_drawEntityNodeRecursive(child, selectedEntityID);
							child = child->next;
						}
					}
					ImGui::TreePop();
				}
			}
			else {
				nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx(treeNodeLabel, nodeFlags);
				if (ImGui::IsItemClicked()) {
					*selectedEntityID = entity->entityID;
				}
			}
			
		}

		void EntityListPanel::tick(Database* db) {
			
			ImGui::SetNextWindowBgAlpha(0.25f);
			ImGui::Begin("Entity List Panel", nullptr, ImGuiWindowFlags_MenuBar);

			enum OPEN_POPUP {
				OPEN_POPUP_NONE,
				OPEN_POPUP_CREATE_DIRLIGHT_FAIL,

				OPEN_POPUP_COUNT
			} openPopup = OPEN_POPUP_NONE;

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu(ICON_II_ANDROID_ADD " Add")) {

					Transform defaultTransform;
					defaultTransform.position = Vec3f(0.0f, 0.0f, 0.0f);
					defaultTransform.scale = Vec3f(1.0f, 1.0f, 1.0f);
					defaultTransform.rotation = quaternionIdentity();

					Entity* selectedEntity = EntityPtr(&db->world, db->selectedEntity);
					GroupEntity* parent = db->selectedEntity.type == EntityType_GROUP ? (GroupEntity*)selectedEntity : selectedEntity->parent;

					if (ImGui::MenuItem(ICON_II_ANDROID_SUNNY " Directional Light")) {
						if (db->world.dirLightEntities.size() == Render::MAX_DIR_LIGHT + 1) 
						{
							openPopup = OPEN_POPUP_CREATE_DIRLIGHT_FAIL;
						}
						else {
							DirLightEntityCreate(&db->world, parent, "Directional Light", defaultTransform, Render::DirectionalLightSpec());
						}
					}
					if (ImGui::MenuItem(" " ICON_II_LIGHTBULB " Pointlight"))
					{
						if (db->world.pointLightEntities.size() == Render::MAX_POINT_LIGHT + 1)
						{
							// open popup to tell error
						}
						else
						{
							PointLightEntityCreate(&db->world, parent, "Pointlight", defaultTransform, Render::PointLightSpec());
						}
					}
					if (ImGui::MenuItem(ICON_MD_HIGHLIGHT " Spotlight"))
					{
						if (db->world.spotLightEntities.size() == Render::MAX_SPOT_LIGHT + 1)
						{
							// open popup to tell error
						}
						else
						{
							SpotLightEntityCreate(&db->world, parent, "Spotlight", defaultTransform, Render::SpotLightSpec());
						}
					}
					if (ImGui::MenuItem(ICON_II_CUBE " Mesh")) {

					}
					
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			if (ImGui::BeginPopupModal("Create Directional Light Fail"))
			{
				ImGui::Text("Cannot create more than %d directional light", Render::MAX_DIR_LIGHT);
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			switch (openPopup) {
			case OPEN_POPUP_CREATE_DIRLIGHT_FAIL:
				ImGui::OpenPopup("Create Directional Light Fail");
				break;
			default:
				break;
			}

			GroupEntity* root = &db->world.groupEntities[0];
			Entity* child = root->first;
			while (child != nullptr) {
				_drawEntityNodeRecursive(child, &db->selectedEntity);
				child = child->next;
			}
			ImGui::End();

		}

	}
}