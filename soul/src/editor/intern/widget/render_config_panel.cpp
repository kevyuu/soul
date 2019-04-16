#include "extern/imgui.h"
#include "extern/imguifilesystem.h"
#include "extern/stb_image.h"

#include "editor/data.h"

namespace Soul {
	namespace Editor {

		void RenderConfigPanel::tick(Database* db) {
			
			RenderConfig& renderConfig = db->world.renderConfig;

			ImGui::Begin("Render Config");

			if (ImGui::CollapsingHeader("Shadow")) {

				ImGui::InputInt("Resolution", &renderConfig.shadowAtlasConfig.resolution);

				int subdivCount[4] = {
					renderConfig.shadowAtlasConfig.subdivSqrtCount[0],
					renderConfig.shadowAtlasConfig.subdivSqrtCount[1],
					renderConfig.shadowAtlasConfig.subdivSqrtCount[2],
					renderConfig.shadowAtlasConfig.subdivSqrtCount[3]
				};
				ImGui::InputInt4("Subdiv", subdivCount);
				for (int i = 0; i < 4; i++) {
					renderConfig.shadowAtlasConfig.subdivSqrtCount[i] = subdivCount[i];
				}

				if (ImGui::Button("Update##Shadow")) {
					db->world.renderSystem.shadowAtlasUpdateConfig(renderConfig.shadowAtlasConfig);
				}
				
			}

			if (ImGui::CollapsingHeader("Voxel GI")) {
				Render::VoxelGIConfig& voxelGIConfig = renderConfig.voxelGIConfig;
				ImGui::InputFloat3("Center", (float*) &voxelGIConfig.center);
				ImGui::InputFloat("Half Span", (float*)&voxelGIConfig.halfSpan);
				ImGui::InputFloat("Bias", (float*)&voxelGIConfig.bias);
				ImGui::InputFloat("Diffuse multiplier", (float*)&voxelGIConfig.diffuseMultiplier);
				ImGui::InputFloat("Specular multiplier", (float*)&voxelGIConfig.specularMultiplier);
				ImGui::InputInt("Resolution", (int*)&voxelGIConfig.resolution);

				if (ImGui::Button("Update##VoxelGI")) {
					db->world.renderSystem.voxelGIUpdateConfig(voxelGIConfig);
				}
			}

			if (ImGui::CollapsingHeader("Environment")) {

				bool changePanorama = ImGui::Button("Change Panorama");

				static ImGuiFs::Dialog dlg;
				const char* panoramaChosenPath = dlg.chooseFileDialog(changePanorama);
				if (strlen(panoramaChosenPath) > 0) {
					int panoramaFilePathLen = strlen(panoramaChosenPath);
					SOUL_ASSERT(0, strlen(panoramaChosenPath) < sizeof(renderConfig.envConfig.panoramaFilePath), "File path too long");
					strcpy(renderConfig.envConfig.panoramaFilePath, panoramaChosenPath);
					renderConfig.envConfig.panoramaFilePath[panoramaFilePathLen] = '\0';

					stbi_set_flip_vertically_on_load(true);
					int width, height, nrComponents;
					float *data = stbi_loadf(panoramaChosenPath, &width, &height, &nrComponents, 0);

					db->world.renderSystem.envSetPanorama(data, width, height);

					stbi_set_flip_vertically_on_load(false);
					stbi_image_free(data);

				}

				ImGui::InputFloat3("Ambient Color", (float*)&renderConfig.envConfig.ambientColor);
				ImGui::InputFloat("Ambient Energy", (float*)&renderConfig.envConfig.ambientEnergy);
				ImGui::InputFloat("Emissive Scale", (float*)&renderConfig.envConfig.emissiveScale);
				
				db->world.renderSystem.envSetAmbientColor(renderConfig.envConfig.ambientColor);
				db->world.renderSystem.envSetAmbientEnergy(renderConfig.envConfig.ambientEnergy);
				db->world.renderSystem.envSetEmissiveScale(renderConfig.envConfig.emissiveScale);

			}

			if (ImGui::CollapsingHeader("Camera Setting"))
			{
				Render::Camera& camera = db->world.camera;
				ImGui::InputFloat("Camera Z Near", &db->world.camera.perspective.zNear);
				ImGui::InputFloat("Camera Z Far", &db->world.camera.perspective.zFar);
				ImGui::Checkbox("Exposure from Camera Setting", &camera.exposureFromSetting);
				if (db->world.camera.exposureFromSetting) {
					ImGui::InputFloat("Aperture", &camera.aperture);
					ImGui::InputFloat("Shutter Speed", &camera.shutterSpeed);
					ImGui::InputFloat("Sensitivity", &camera.sensitivity);
					camera.updateExposure();
				}
				ImGui::InputFloat("Exposure", &camera.exposure);
			}

			if (ImGui::CollapsingHeader("Post Process"))
			{
				if (ImGui::CollapsingHeader("Glow"))
				{
					ImGui::InputFloat("Threshold", &renderConfig.postProcessConfig.glowConfig.threshold);
					ImGui::InputFloat("Intensity", &renderConfig.postProcessConfig.glowConfig.intensity);
					for (int i = 0; i < sizeof(renderConfig.postProcessConfig.glowConfig.useLevel); i++)
					{
						char label[12];
						sprintf(label, "Use level %d", i);
						ImGui::Checkbox(label, &renderConfig.postProcessConfig.glowConfig.useLevel[i]);
					}
					db->world.renderSystem.postProcessUpdateGlow(renderConfig.postProcessConfig.glowConfig);
				}
			}

			if (ImGui::Button("Shader Reload")) {
				db->world.renderSystem.shaderReload();
			}

			ImGui::End();

		}

	}
}