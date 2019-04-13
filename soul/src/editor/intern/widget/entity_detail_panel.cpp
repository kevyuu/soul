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

			Entity* entity = EntityPtr(&db->world, selectedEntityID);

			ImGui::InputText("Name",	entity->name, sizeof(entity->name));

			Transform localTransform =entity->localTransform;
			ImGui::Text("Local Transform");
			ImGui::InputFloat3("Position##local", (float*)&localTransform.position);
			ImGui::InputFloat3("Scale##local", (float*)&localTransform.scale);
			ImGui::InputFloat4("Rotation##local", (float*)&localTransform.rotation);
			if (localTransform != entity->localTransform) {
				EntitySetLocalTransform(&db->world, entity, localTransform);
			}

			Transform worldTransform = entity->worldTransform;
			ImGui::Text("World Transform");
			ImGui::InputFloat3("Position##world", (float*)&worldTransform.position);
			ImGui::InputFloat3("Scale##world", (float*)&worldTransform.scale);
			ImGui::InputFloat4("Rotation##world", (float*)&worldTransform.rotation);
			if (worldTransform != entity->worldTransform) {
				EntitySetWorldTransform(&db->world, entity, worldTransform);
			}

			ImGui::Separator();

			Render::System& renderSystem = db->world.renderSystem;

			switch (selectedEntityID.type) {
			case EntityType_MESH: {
					MeshEntity& meshEntity = *(MeshEntity*)entity;

					Material& material = db->world.materials[meshEntity.materialID];
					ImGui::Text("Material");
					ImGui::Text("Name : %d", material.name);
					ImGui::InputFloat3("Albedo", (float*)&material.albedo);
					ImGui::SliderFloat("Metallic", (float*)&material.metallic, 0.0f, 1.0f);
					ImGui::SliderFloat("Roughness", (float*)&material.roughness, 0.0f, 1.0f);
					ImGui::InputFloat3("Emissive", (float*)&material.emissive);

					ImGui::Checkbox("Use albedo tex", &material.useAlbedoTex);
					ImGui::Checkbox("Use metallic tex", &material.useMetallicTex);
					ImGui::Checkbox("Use roughness tex", &material.useRoughnessTex);
					ImGui::Checkbox("Use ao tex", &material.useAOTex);
					ImGui::Checkbox("Use emissive tex", &material.useEmissiveTex);

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
						textures[material.emissiveTexID].rid,

						material.useAlbedoTex,
						material.useNormalTex,
						material.useMetallicTex,
						material.useRoughnessTex,
						material.useAOTex,
						material.useEmissiveTex,

						material.albedo,
						material.metallic,
						material.roughness,
						material.emissive,

						material.metallicTextureChannel,
						material.roughnessTextureChannel,
						material.aoTextureChannel
					};

					db->world.renderSystem.materialUpdate(material.rid, spec);

					break;
			}
			case EntityType_GROUP: {
					// do nothing
					break;
			}
			case EntityType_DIRLIGHT: {
				DirLightEntity& dirLightEntity = *(DirLightEntity*)entity;

				Vec3f direction = dirLightEntity.spec.direction;
				ImGui::InputFloat3("Direction", (float*)&direction);
				if (direction != dirLightEntity.spec.direction) {
					DirLightEntitySetDirection(&db->world, &dirLightEntity, direction);
				}

				ImGui::InputFloat3("Color", (float*)&dirLightEntity.spec.color);
				db->world.renderSystem.dirLightSetColor(dirLightEntity.rid, dirLightEntity.spec.color);

				ImGui::InputFloat("Bias", &dirLightEntity.spec.bias);
				db->world.renderSystem.dirLightSetBias(dirLightEntity.rid, dirLightEntity.spec.bias);

				ImGui::InputFloat3("Cascade split", (float*)&dirLightEntity.spec.split);
				float* split = dirLightEntity.spec.split;
				db->world.renderSystem.dirLightSetCascadeSplit(dirLightEntity.rid, split[0], split[1], split[2]);

				ImGui::Text("Shadow Map Resolution");
				ImGui::InputInt("##ShadowMapResolution", &dirLightEntity.spec.shadowMapResolution);
				if (ImGui::Button("Update")) {
					db->world.renderSystem.dirLightSetShadowMapResolution(dirLightEntity.rid, dirLightEntity.spec.shadowMapResolution);
				}

				break;
			}
			case EntityType_POINTLIGHT: {
				PointLightEntity* pointLightEntity = (PointLightEntity*)entity;
				Render::PointLightRID rid = pointLightEntity->rid;

				ImGui::InputFloat("Bias", &pointLightEntity->spec.bias);
				renderSystem.pointLightSetBias(rid, pointLightEntity->spec.bias);

				ImGui::InputFloat3("Color", (float*)&pointLightEntity->spec.color);
				renderSystem.pointLightSetColor(rid, pointLightEntity->spec.color);

				ImGui::InputFloat("Max Distance", &pointLightEntity->spec.maxDistance);
				renderSystem.pointLightSetMaxDistance(rid, pointLightEntity->spec.maxDistance);

				break;
			}
			case EntityType_SPOTLIGHT:{
				SpotLightEntity* spotLightEntity = (SpotLightEntity*)entity;
				Render::SpotLightRID rid = spotLightEntity->rid;

				Vec3f direction = spotLightEntity->spec.direction;
				ImGui::InputFloat3("Direction", (float*)&direction);
				SpotLightEntitySetDirection(&db->world, spotLightEntity, direction);

				ImGui::InputFloat3("Color", (float*)&spotLightEntity->spec.color);
				renderSystem.spotLightSetColor(rid, spotLightEntity->spec.color);

				ImGui::InputFloat("Bias", &spotLightEntity->spec.bias);
				renderSystem.spotLightSetBias(rid, spotLightEntity->spec.bias);

				ImGui::SliderAngle("Inner angle", &spotLightEntity->spec.angleInner);
				renderSystem.spotLightSetAngleInner(rid, spotLightEntity->spec.angleInner);

				ImGui::SliderAngle("Outer angle", &spotLightEntity->spec.angleOuter);
				renderSystem.spotLightSetAngleOuter(rid, spotLightEntity->spec.angleOuter);

				ImGui::InputFloat("Max Distance", &spotLightEntity->spec.maxDistance);
				renderSystem.spotLightSetMaxDistance(rid, spotLightEntity->spec.maxDistance);

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