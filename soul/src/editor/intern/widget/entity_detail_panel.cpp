#include "extern/imgui.h"

#include "core/debug.h"
#include "core/math.h"

#include "editor/data.h"
#include "editor/system.h"
#include "editor/intern/entity.h"

namespace Soul {
	namespace Editor {

		void EntityDetailPanel::tick(Database* db) {
			ImGui::Begin("Entity Property");

			EntityID selectedEntityID = db->selectedEntity;

			if (selectedEntityID == db->world.rootEntityID) {
				ImGui::Text("No Entity Selected");
				ImGui::End();
				return;
			}

			switch (selectedEntityID.type) {
			case EntityType_MESH: {
					MeshEntity& meshEntity = db->world.meshEntities[selectedEntityID.index];

					ImGui::InputText("Name", meshEntity.name, sizeof(meshEntity.name));
					
					Transform localTransform = meshEntity.localTransform;
					ImGui::Text("Local Transform");
					ImGui::InputFloat3("Position##local", (float*)&localTransform.position);
					ImGui::InputFloat3("Scale##local", (float*)&localTransform.scale);
					ImGui::InputFloat4("Rotation##local", (float*)&localTransform.rotation);
					if (localTransform != meshEntity.localTransform) {
						MeshEntitySetLocalTransform(&db->world, &meshEntity, localTransform);
					}

					Transform worldTransform = meshEntity.worldTransform;
					ImGui::Text("World Transform");
					ImGui::InputFloat3("Position##world", (float*)&worldTransform.position);
					ImGui::InputFloat3("Scale##world", (float*)&worldTransform.scale);
					ImGui::InputFloat4("Rotation##world", (float*)&worldTransform.rotation);
					if (worldTransform != meshEntity.worldTransform) {
						MeshEntitySetWorldTransform(&db->world, &meshEntity, worldTransform);
					}

					ImGui::Separator();
					Material& material = db->world.materials[meshEntity.materialID];
					ImGui::Text("Material");
					ImGui::Text("Name : %d", material.name);
					ImGui::InputFloat3("Albedo", (float*)&material.albedo);
					ImGui::SliderFloat("Metallic", (float*)&material.metallic, 0.0f, 1.0f);
					ImGui::SliderFloat("Roughness", (float*)&material.roughness, 0.0f, 1.0f);
					ImGui::Checkbox("Use albedo tex", &material.useAlbedoTex);
					ImGui::Checkbox("Use metallic tex", &material.useMetallicTex);
					ImGui::Checkbox("Use roughness tex", &material.useRoughnessTex);
					ImGui::Checkbox("Use ao tex", &material.useAOTex);

					const char* textureChannels[Render::TexChannel_COUNT];
					textureChannels[Render::TexChannel_RED] = "Red";
					textureChannels[Render::TexChannel_GREEN] = "Green";
					textureChannels[Render::TexChannel_BLUE] = "Blue";
					textureChannels[Render::TexChannel_ALPHA] = "Alpha";

					ImGui::Combo("Metallic Tex Channel", (int*)&material.metallicTextureChannel, textureChannels, IM_ARRAYSIZE(textureChannels));
					ImGui::Combo("Roughness Tex Channel", (int*)&material.roughnessTextureChannel, textureChannels, IM_ARRAYSIZE(textureChannels));
					ImGui::Combo("AO Tex Channel", (int*)&material.aoTextureChannel, textureChannels, IM_ARRAYSIZE(textureChannels));

					PoolArray<Texture>& textures = db->world.textures;

					Render::MaterialSpec spec = {
						textures[material.albedoTexID].rid,
						textures[material.normalTexID].rid,
						textures[material.metallicTexID].rid,
						textures[material.roughnessTexID].rid,
						textures[material.aoTexID].rid,

						material.useAlbedoTex,
						material.useNormalTex,
						material.useMetallicTex,
						material.useRoughnessTex,
						material.useAOTex,

						material.albedo,
						material.metallic,
						material.roughness,

						material.metallicTextureChannel,
						material.roughnessTextureChannel,
						material.aoTextureChannel
					};

					db->world.renderSystem.materialUpdate(material.rid, spec);

					break;
			}
			case EntityType_GROUP: {
					GroupEntity& groupEntity = db->world.groupEntities[selectedEntityID.index];
					ImGui::InputText("Name", groupEntity.name, sizeof(groupEntity.name));

					Transform localTransform = groupEntity.localTransform;
					ImGui::Text("Local Transform");
					ImGui::InputFloat3("Position##local", (float*)&localTransform.position);
					ImGui::InputFloat3("Scale##local", (float*)&localTransform.scale);
					ImGui::InputFloat4("Rotation##local", (float*)&localTransform.rotation);
					if (localTransform != groupEntity.localTransform) {
						GroupEntitySetLocalTransform(&db->world, &groupEntity, localTransform);
					}

					Transform worldTransform = groupEntity.worldTransform;
					ImGui::Text("World Transform");
					ImGui::InputFloat3("Position##world", (float*)&worldTransform.position);
					ImGui::InputFloat3("Scale##world", (float*)&worldTransform.scale);
					ImGui::InputFloat4("Rotation##world", (float*)&worldTransform.rotation);
					if (worldTransform != groupEntity.worldTransform) {
						GroupEntitySetWorldTransform(&db->world, &groupEntity, worldTransform);
					}

					break;
			}
			case EntityType_DIRLIGHT: {
					DirLightEntity& dirLightEntity = db->world.dirLightEntities[selectedEntityID.index];

					ImGui::InputText("Name", dirLightEntity.name, sizeof(dirLightEntity.name));

					Transform localTransform = dirLightEntity.localTransform;
					ImGui::Text("Local Transform");
					ImGui::InputFloat3("Position##local", (float*)&localTransform.position);
					ImGui::InputFloat3("Scale##local", (float*)&localTransform.scale);
					ImGui::InputFloat4("Rotation##local", (float*)&localTransform.rotation);
					if (localTransform != dirLightEntity.localTransform) {
						DirLightEntitySetLocalTransform(&db->world, &dirLightEntity, localTransform);
					}

					Transform worldTransform = dirLightEntity.worldTransform;
					ImGui::Text("World Transform");
					ImGui::InputFloat3("Position##world", (float*)&worldTransform.position);
					ImGui::InputFloat3("Scale##world", (float*)&worldTransform.scale);
					ImGui::InputFloat4("Rotation##world", (float*)&worldTransform.rotation);
					if (worldTransform != dirLightEntity.worldTransform) {
						DirLightEntitySetWorldTransform(&db->world, &dirLightEntity, worldTransform);
					}

					ImGui::Separator();
					Vec3f direction = dirLightEntity.spec.direction;
					ImGui::InputFloat3("Direction", (float*)&direction);
					if (direction != dirLightEntity.spec.direction) {
						DirLightEntitySetDirection(&db->world, &dirLightEntity, direction);
					}

					ImGui::InputFloat3("Color", (float*) &dirLightEntity.spec.color);
					db->world.renderSystem.dirLightSetColor(dirLightEntity.lightRID, dirLightEntity.spec.color);
					
					ImGui::InputFloat("Bias", &dirLightEntity.spec.bias);
					db->world.renderSystem.dirLightSetBias(dirLightEntity.lightRID, dirLightEntity.spec.bias);

					ImGui::InputFloat3("Cascade split", (float*) &dirLightEntity.spec.split);
					float* split = dirLightEntity.spec.split;
					db->world.renderSystem.dirLightSetCascadeSplit(dirLightEntity.lightRID, split[0], split[1], split[2]);
					
					ImGui::Text("Shadow Map Resolution");
					ImGui::InputInt("##ShadowMapResolution", &dirLightEntity.spec.shadowMapResolution);
					if (ImGui::Button("Update")) {
						db->world.renderSystem.dirLightSetShadowMapResolution(dirLightEntity.lightRID, dirLightEntity.spec.shadowMapResolution);
					}

					break;

			}
			default:
				SOUL_ASSERT(0, false, "Invalid entity type | Entity type = %d", selectedEntityID.type);
				break;
			}

			ImGui::End();
		}

	}
}