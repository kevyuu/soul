#include "core/type.h"
#include "core/math.h"
#include "render/type.h"
#include "render/system.h"
#include "render/intern/glext.h"
#include "asset_import.h"
#include "sandbox/type.h"

#include "extern/imgui.h"
#include "extern/imgui_impl_glfw.h"
#include "extern/imgui_impl_opengl3.h"
#include "extern/imguifilesystem.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <map>


void SettingWindow(SceneData* sceneData) {

	Soul::RenderSystem *renderSystem = &sceneData->renderSystem;

	ImGui::Begin("Setting Window");
	
	if (ImGui::CollapsingHeader("Directional light")) {

		DirLightConfig* dirLightConfig = &sceneData->dirLightConfig;

		ImGui::SliderFloat3("Sun Direction", (float*)&dirLightConfig->dir, -1.0f, 1.0f);
		renderSystem->dirLightSetDirection(sceneData->sunRID, dirLightConfig->dir);
		
		ImGui::SliderFloat3("Color", (float*)&dirLightConfig->color, 0.0f, 1.0f);
		ImGui::InputFloat("Energy", (float*)&dirLightConfig->energy, 0.0f, 100000.0f);

		renderSystem->dirLightSetColor(sceneData->sunRID, dirLightConfig->color * dirLightConfig->energy);

		ImGui::InputInt("Shadow Map Resolution", &dirLightConfig->resolution);
		dirLightConfig->resolution = Soul::nextPowerOfTwo(dirLightConfig->resolution);
		renderSystem->dirLightSetShadowMapResolution(sceneData->sunRID, dirLightConfig->resolution);

		Soul::Camera& camera = sceneData->camera;
		float cameraRange = camera.perspective.zFar - camera.perspective.zNear;
		int distance[3];

		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f * 0.635f);

		ImGui::SliderFloat("##Split 1", &dirLightConfig->split[0], 0.0f, 1.0f);
		distance[0] = dirLightConfig->split[0] * cameraRange;
		ImGui::SameLine();
		ImGui::InputInt("Split 1", &distance[0]);
		dirLightConfig->split[0] = distance[0] / cameraRange;

		ImGui::SliderFloat("##Split 2", &dirLightConfig->split[1], 0.0f, 1.0f);
		distance[1] = dirLightConfig->split[1] * cameraRange;
		ImGui::SameLine();
		ImGui::InputInt("Split 2", &distance[1]);
		dirLightConfig->split[1] = distance[1] / cameraRange;
		
		ImGui::SliderFloat("##Split 3", &dirLightConfig->split[2], 0.0f, 1.0f);
		distance[2] = dirLightConfig->split[2] * cameraRange;
		ImGui::SameLine();
		ImGui::InputInt("Split 3", &distance[2]);
		dirLightConfig->split[2] = distance[2] / cameraRange;
		
		ImGui::SliderFloat("##Bias", &dirLightConfig->bias, 0.0f, 1.0f);
		ImGui::SameLine();
		ImGui::InputFloat("Bias", &dirLightConfig->bias);
		ImGui::PopItemWidth();

		renderSystem->dirLightSetCascadeSplit(sceneData->sunRID,
			dirLightConfig->split[0],
			dirLightConfig->split[1],
			dirLightConfig->split[2]);
		renderSystem->dirLightSetBias(sceneData->sunRID, dirLightConfig->bias);

	}

	if (ImGui::CollapsingHeader("Environment")) {

		static Soul::Vec3f color = Soul::Vec3f(1.0f, 1.0f, 1.0f);
		ImGui::SliderFloat3("Ambient color", (float*)&color, 0.0f, 1.0f);
		renderSystem->envSetAmbientColor(color);

		static float energy = 0.1f;
		ImGui::InputFloat("Ambient energy", &energy);
		renderSystem->envSetAmbientEnergy(energy);

		const bool uploadPanorama = ImGui::Button("Upload panorama");
		static ImGuiFs::Dialog mtlDialog;
		const char* panoramaChosenPath = mtlDialog.chooseFileDialog(uploadPanorama);
		if (strlen(panoramaChosenPath) > 0) {
			int panoramaPathLen = strlen(panoramaChosenPath);
			SOUL_ASSERT(0, panoramaPathLen < 999, "File path too long");
			strcpy(sceneData->mtlDirPath, panoramaChosenPath);
			sceneData->mtlDirPath[panoramaPathLen] = '\0';

			stbi_set_flip_vertically_on_load(true);
			int width, height, nrComponents;
			float *data = stbi_loadf("C:/Dev/soul/soul/assets/syferfontein_0d_clear_16k.hdr", &width, &height, &nrComponents, 0);

			GLuint hdrTexture;
			glGenTextures(1, &hdrTexture);
			glBindTexture(GL_TEXTURE_2D, hdrTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			renderSystem->envSetPanorama(hdrTexture);

			stbi_image_free(data);
		}

	}

	if (ImGui::CollapsingHeader("Voxel GI")) {

		ImGui::SliderFloat3("Center", (float*)&sceneData->renderConfig.voxelGIConfig.center, 0.0f, 1.0f);
		ImGui::InputFloat("Half Span", &sceneData->renderConfig.voxelGIConfig.halfSpan);
		ImGui::InputInt("Resolution", (int*)&sceneData->renderConfig.voxelGIConfig.resolution);
		ImGui::InputFloat("Voxel Bias", (float*)&sceneData->renderConfig.voxelGIConfig.bias);
		ImGui::InputFloat("Diffuse multiplier", (float*)&sceneData->renderConfig.voxelGIConfig.diffuseMultiplier);
		ImGui::InputFloat("Specular multiplier", (float*)&sceneData->renderConfig.voxelGIConfig.specularMultiplier);

		if (ImGui::Button("Update")) {
			renderSystem->voxelGIUpdateConfig(sceneData->renderConfig.voxelGIConfig);
			renderSystem->voxelGIVoxelize();
		}
	}

	if (ImGui::CollapsingHeader("Shadow Atlas Config")) {

		Soul::RenderSystem::ShadowAtlasConfig& shadowAtlasConfig = sceneData->renderConfig.shadowAtlasConfig;

		ImGui::InputInt("Resolution", &shadowAtlasConfig.resolution);
		shadowAtlasConfig.resolution = Soul::nextPowerOfTwo(shadowAtlasConfig.resolution);

		int subdivSqrtCount = shadowAtlasConfig.subdivSqrtCount[0];
		ImGui::InputInt("Subdiv 1 Dimension", &subdivSqrtCount);
		shadowAtlasConfig.subdivSqrtCount[0] = subdivSqrtCount;

		subdivSqrtCount = shadowAtlasConfig.subdivSqrtCount[1];
		ImGui::InputInt("Subdiv 2 Dimension", &subdivSqrtCount);
		shadowAtlasConfig.subdivSqrtCount[1] = subdivSqrtCount;

		subdivSqrtCount = shadowAtlasConfig.subdivSqrtCount[2];
		ImGui::InputInt("Subdiv 3 Dimension", &subdivSqrtCount);
		shadowAtlasConfig.subdivSqrtCount[2] = subdivSqrtCount;

		subdivSqrtCount = shadowAtlasConfig.subdivSqrtCount[3];
		ImGui::InputInt("Subdiv 4 Dimension", &subdivSqrtCount);
		shadowAtlasConfig.subdivSqrtCount[3] = subdivSqrtCount;

		if (ImGui::Button("Update")) {
			renderSystem->shadowAtlasUpdateConfig(shadowAtlasConfig);
		}

	}

	if (ImGui::Button("Reload shader")) {
		renderSystem->shaderReload();
	}

	ImGui::End();
}

void MenuBar(SceneData* sceneData) {

	Soul::RenderSystem* renderSystem = &sceneData->renderSystem;

	if (ImGui::BeginPopupModal("Import Obj and MTL", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const bool browseObjFile = ImGui::Button("Browse##obj");
		ImGui::SameLine();
		ImGui::InputText("Obj File", sceneData->objFilePath, 1000);
		static ImGuiFs::Dialog dlg;
		const char* objChosenPath = dlg.chooseFileDialog(browseObjFile);
		if (strlen(objChosenPath) > 0) {
			SOUL_ASSERT(0, strlen(objChosenPath) < 999, "File path too long");
			strcpy(sceneData->objFilePath, objChosenPath);
		}

		const bool browseMTLDir = ImGui::Button("Browse##mtl");
		ImGui::SameLine();
		ImGui::InputText("MTL Dir", sceneData->mtlDirPath, 1000);
		static ImGuiFs::Dialog mtlDialog;
		const char* mtlChosenPath = mtlDialog.chooseFolderDialog(browseMTLDir);
		if (strlen(mtlChosenPath) > 0) {
			int mtlPathLen = strlen(mtlChosenPath);
			SOUL_ASSERT(0, mtlPathLen < 998, "Directory path too long");
			strcpy(sceneData->mtlDirPath, mtlChosenPath);
			sceneData->mtlDirPath[mtlPathLen] = '/';
			sceneData->mtlDirPath[mtlPathLen + 1] = '\0';
		}

		if (ImGui::Button("OK", ImVec2(120, 0))) {

			ImportObjMtlAssets(sceneData, sceneData->objFilePath, sceneData->mtlDirPath);
			renderSystem->voxelGIVoxelize();

			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Import GLTF", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		const bool browseGLTFFile = ImGui::Button("Browse##gltf");
		ImGui::SameLine();
		ImGui::InputText("GLTF File", sceneData->gltfFilePath, 1000);
		static ImGuiFs::Dialog dlg;
		const char* gltfChosenPath = dlg.chooseFileDialog(browseGLTFFile);
		if (strlen(gltfChosenPath) > 0) {
			SOUL_ASSERT(0, strlen(gltfChosenPath) < 999, "File path too long");
			strcpy(sceneData->gltfFilePath, gltfChosenPath);
		}

		if (ImGui::Button("OK", ImVec2(120, 0))) {

			ImportGLTFAssets(sceneData, sceneData->gltfFilePath);
			renderSystem->voxelGIVoxelize();

			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();

	}

	enum ACTION {
		ACTION_NONE,
		ACTION_IMPORT_OBJ_AND_MTL,
		ACTION_IMPORT_GLTF
	};

	ACTION action = ACTION_NONE;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::BeginMenu("Import")) {
				if (ImGui::MenuItem("Import Obj and MTL")) {
					action = ACTION_IMPORT_OBJ_AND_MTL;
				}
				else if (ImGui::MenuItem("Import GLTF")) {
					action = ACTION_IMPORT_GLTF;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (action == ACTION_IMPORT_OBJ_AND_MTL) {
		ImGui::OpenPopup("Import Obj and MTL");
	}
	else if (action == ACTION_IMPORT_GLTF) {
		ImGui::OpenPopup("Import GLTF");
	}
}

void MaterialWindow(SceneData* sceneData) {
	ImGui::Begin("Material list");

	Soul::Array<UIMaterial>& materials = sceneData->materials;
	Soul::Array<UITexture>& textures = sceneData->textures;
	Soul::RenderSystem& renderSystem = sceneData->renderSystem;
	
	for (int i = 1; i < materials.getSize(); i++) {
		UIMaterial& material = materials[i];
		if (ImGui::CollapsingHeader(material.name)) {
			
			ImGui::Checkbox("Use albedo texture", &(material.useAlbedoTex));
			ImGui::Checkbox("Use normal texture", &(material.useNormalTex));
			ImGui::Checkbox("Use metalic texture", &(material.useMetallicTex));
			ImGui::Checkbox("Use roughness texture", &(material.useRoughnessTex));
			ImGui::Checkbox("Use ao texture", &(material.useAOTex));
			ImGui::InputFloat3("Albedo color", (float*) &material.albedo);
			ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);

			const char* textureChannels[] = { "Red", "Green", "Blue", "Alpha" };
			
			ImGui::Combo("Metallic Texture Channel", (int*)&material.metallicTextureChannel, textureChannels, IM_ARRAYSIZE(textureChannels));
			ImGui::Combo("Roughness Texture Channel", (int*)&material.roughnessTextureChannel, textureChannels, IM_ARRAYSIZE(textureChannels));
			ImGui::Combo("AO Texture Channel", (int*)&material.aoTextureChannel, textureChannels, IM_ARRAYSIZE(textureChannels));
			Soul::MaterialSpec spec = {
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

			renderSystem.materialUpdate(material.rid, spec);

		}
	}

	ImGui::End();
}

void MeshWindow(SceneData* sceneData) {
	ImGui::Begin("Mesh List");
	Soul::Array<UIMesh>& meshes = sceneData->meshes;

	for (int i = 1; i < meshes.getSize(); i++) {
		UIMesh& mesh = meshes[i];
		char title[1024];
		sprintf(title, "Object %d : %s", i, mesh.name);
		if (ImGui::CollapsingHeader(title)) {

			ImGui::InputText("Name", mesh.name, 512);
			ImGui::Text("RID : %d", mesh.rid);

			ImGui::InputFloat3("Position", (float*)&mesh.position);
			ImGui::InputFloat3("Scale", (float*)&mesh.scale);
			ImGui::InputFloat4("Rotation", (float*)&mesh.rotation);

			sceneData->renderSystem.meshSetTransform(mesh.rid, mesh.position, mesh.scale, mesh.rotation);

		}
	}

	ImGui::End();
}



int main() {


	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Soul Sandbox", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	std::cout << "Make context current" << std::endl;
	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 150";
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Setup style
	ImGui::StyleColorsDark();

	SceneData sceneData;
	Soul::RenderSystem& renderSystem = sceneData.renderSystem;
	Soul::RenderSystem::Config& renderConfig = sceneData.renderConfig;
	Soul::Camera& camera = sceneData.camera;
	sceneData.textures.init(10000);
	sceneData.textures.pushBack({ 0 });
	sceneData.materials.init(10000);
	sceneData.materials.pushBack({ 0 });
	sceneData.meshes.init(10000);
	sceneData.meshes.pushBack({ 0 });

	
	int resWidth, resHeight;
	glfwGetFramebufferSize(window, (int*)&resWidth, &resHeight);
	renderConfig.targetWidthPx = resWidth;
	renderConfig.targetHeightPx = resHeight;
	renderConfig.voxelGIConfig.center = Soul::Vec3f(0.0f, 0.0f, 0.0f);
	renderConfig.voxelGIConfig.halfSpan = 1800.0f;
	renderConfig.voxelGIConfig.resolution = 128;
	renderConfig.shadowAtlasConfig.resolution = 8192;
	renderConfig.shadowAtlasConfig.subdivSqrtCount[0] = 1;
	renderConfig.shadowAtlasConfig.subdivSqrtCount[1] = 1;
	renderConfig.shadowAtlasConfig.subdivSqrtCount[2] = 2;
	renderConfig.shadowAtlasConfig.subdivSqrtCount[3] = 2;

	renderSystem.init(renderConfig);	

	camera.position = Soul::Vec3f(0.0f, 0.0f, 0.0f);
	camera.direction = Soul::Vec3f(0.0f, 0.0f, 1.0f);
	camera.up = Soul::Vec3f(0.0f, 1.0f, 0.0f);
	camera.perspective.fov = Soul::PI / 4;
	camera.perspective.aspectRatio = 1920 / 1080.0f;
	camera.perspective.zNear = 0.1f;
	camera.perspective.zFar = 4000.0f;
	camera.projection = Soul::mat4Perspective(camera.perspective.fov,
		camera.perspective.aspectRatio,
		camera.perspective.zNear,
		camera.perspective.zFar);

	Soul::DirectionalLightSpec lightSpec;
	lightSpec.direction = Soul::Vec3f(0.03f, -1.0f, 0.35f);
	lightSpec.color = Soul::Vec3f(1.0f, 1.0f, 1.0f) * 100.0f;
	lightSpec.shadowMapResolution = Soul::TR_4096;
	sceneData.sunRID = renderSystem.dirLightCreate(lightSpec);

	sceneData.dirLightConfig.dir = lightSpec.direction;
	sceneData.dirLightConfig.color = Soul::Vec3f(1.0f, 1.0f, 1.0f);
	sceneData.dirLightConfig.energy = 100.0f;
	sceneData.dirLightConfig.resolution = 4096;
	sceneData.dirLightConfig.split[0] = 0.1f;
	sceneData.dirLightConfig.split[1] = 0.2f;
	sceneData.dirLightConfig.split[2] = 0.5f;
	sceneData.dirLightConfig.bias = lightSpec.bias;

	while (!glfwWindowShouldClose(window)) {
		
		SOUL_PROFILE_RANGE_PUSH("Frame");

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		int resWidth, resHeight;
		glfwGetFramebufferSize(window, (int*)&camera.viewportWidth, (int*)&camera.viewportHeight);

		ImGuiIO& io = ImGui::GetIO();

		if (!io.WantCaptureMouse and ImGui::IsMouseDown(1)) {
			static float translationSpeed = 5.0f;

			float cameraSpeedInc = 0.1f;
			translationSpeed += (cameraSpeedInc * translationSpeed * io.MouseWheel);

			if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
				translationSpeed *= 0.9f;
			}
			if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
				translationSpeed *= 1.1f;
			}

			

			if (ImGui::IsMouseDragging(1)) {

				Soul::Vec3f cameraRight = cross(camera.up, camera.direction) * -1.0f;

				{
					Soul::Mat4 rotate = mat4Rotate(cameraRight, -2.0f * io.MouseDelta.y / camera.viewportHeight * Soul::PI);
					camera.direction = rotate * camera.direction;
					camera.up = rotate * camera.up;
				}

				{
					Soul::Mat4 rotate = mat4Rotate(Soul::Vec3f(0.0f, 1.0f, 0.0f), -2.0f * io.MouseDelta.x / camera.viewportWidth *Soul::PI);

					if (camera.direction != Soul::Vec3f(0.0f, 1.0f, 0.0f))
						camera.direction = rotate * camera.direction;
					if (camera.up != Soul::Vec3f(0.0f, 1.0f, 0.0f))
						camera.up = rotate * camera.up;
				}

			}

			Soul::Vec3f right = Soul::unit(cross(camera.direction, camera.up));
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				camera.position += Soul::unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				camera.position -= Soul::unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				camera.position -= Soul::unit(right) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				camera.position += Soul::unit(right) * translationSpeed;
			}

		}

		MenuBar(&sceneData);
		SettingWindow(&sceneData);
		MaterialWindow(&sceneData);
		MeshWindow(&sceneData);

		ImGui::Begin("Demo Scene Metric");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			1000.0f / ImGui::GetIO().Framerate,
			ImGui::GetIO().Framerate);

		ImGui::Text("Position : (%.3f,%.3f,%.3f)", camera.position.x, camera.position.y, camera.position.z);

		ImGui::End();

		ImGui::ShowDemoWindow();

		renderSystem.render(camera);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		SOUL_PROFILE_RANGE_POP();
	}

	renderSystem.shutdown();
	glfwTerminate();
	return 0;

}
