#include "core/debug.h"
#include "core/math.h"

#include "render/system.h"
#include "editor/system.h"

#include <GLFW/glfw3.h>
#include "extern/glad.h"
#include "extern/imgui.h"
#include "extern/imgui_impl_glfw.h"
#include "extern/imgui_impl_opengl3.h"
#include "extern/imguifilesystem.h"
#include "extern/ImGuizmo.h"
#include "extern/imgui_pie.h"
#include "extern/icon/IconsIonicons.h"
#include "extern/icon/IconsMaterialDesign.h"

#include "editor/intern/entity.h"

namespace Soul {
	namespace Editor {

		static void _ApplyImguiStyle() {
			ImGui::StyleColorsDark();
			auto& style = ImGui::GetStyle();

			const auto font_size = 16.0f;
			const auto roundness = 2.0f;

			const auto text = ImVec4(0.76f, 0.77f, 0.8f, 1.0f);
			const auto white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			const auto black = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			const auto background_very_dark = ImVec4(0.08f, 0.086f, 0.094f, 1.0f);
			const auto background_dark = ImVec4(0.117f, 0.121f, 0.145f, 1.0f);
			const auto background_medium = ImVec4(0.26f, 0.26f, 0.27f, 1.0f);
			const auto background_light = ImVec4(0.37f, 0.38f, 0.39f, 1.0f);
			const auto highlight_blue = ImVec4(0.172f, 0.239f, 0.341f, 1.0f);
			const auto highlight_blue_hovered = ImVec4(0.202f, 0.269f, 0.391f, 1.0f);
			const auto highlight_blue_active = ImVec4(0.382f, 0.449f, 0.561f, 1.0f);
			const auto bar_background = ImVec4(0.078f, 0.082f, 0.09f, 1.0f);
			const auto bar = ImVec4(0.164f, 0.180f, 0.231f, 1.0f);
			const auto bar_hovered = ImVec4(0.411f, 0.411f, 0.411f, 1.0f);
			const auto bar_active = ImVec4(0.337f, 0.337f, 0.368f, 1.0f);

			// Spatial
			style.WindowBorderSize = 1.0f;
			style.FrameBorderSize = 0.0f;
			//style.WindowMinSize		= ImVec2(160, 20);
			style.FramePadding = ImVec2(5, 5);
			style.ItemSpacing = ImVec2(6, 5);
			//style.ItemInnerSpacing	= ImVec2(6, 4);
			style.Alpha = 1.0f;
			style.WindowRounding = roundness;
			style.FrameRounding = roundness;
			style.PopupRounding = roundness;
			//style.IndentSpacing		= 6.0f;
			//style.ItemInnerSpacing	= ImVec2(2, 4);
			//style.ColumnsMinSpacing	= 50.0f;
			//style.GrabMinSize			= 14.0f;
			style.GrabRounding = roundness;
			style.ScrollbarSize = 20.0f;
			style.ScrollbarRounding = roundness;

			ImVec4 windowBackground = ImVec4(background_dark.x, background_dark.y, background_dark.z, 0.15f);

			// Colors
			style.Colors[ImGuiCol_Text] = text;
			//style.Colors[ImGuiCol_TextDisabled]			= ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
			style.Colors[ImGuiCol_WindowBg] = windowBackground;
			//style.Colors[ImGuiCol_ChildBg]				= ImVec4(0.20f, 0.22f, 0.27f, 0.58f);
			style.Colors[ImGuiCol_Border] = black;
			//style.Colors[ImGuiCol_BorderShadow]			= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_FrameBg] = bar;
			style.Colors[ImGuiCol_FrameBgHovered] = highlight_blue;
			style.Colors[ImGuiCol_FrameBgActive] = highlight_blue_hovered;
			style.Colors[ImGuiCol_TitleBg] = background_very_dark;
			//style.Colors[ImGuiCol_TitleBgCollapsed]		= ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
			style.Colors[ImGuiCol_TitleBgActive] = bar;
			style.Colors[ImGuiCol_MenuBarBg] = background_very_dark;
			style.Colors[ImGuiCol_ScrollbarBg] = bar_background;
			style.Colors[ImGuiCol_ScrollbarGrab] = bar;
			style.Colors[ImGuiCol_ScrollbarGrabHovered] = bar_hovered;
			style.Colors[ImGuiCol_ScrollbarGrabActive] = bar_active;
			style.Colors[ImGuiCol_CheckMark] = highlight_blue_hovered;
			style.Colors[ImGuiCol_SliderGrab] = highlight_blue_hovered;
			style.Colors[ImGuiCol_SliderGrabActive] = highlight_blue_active;
			style.Colors[ImGuiCol_Button] = bar_active;
			style.Colors[ImGuiCol_ButtonHovered] = highlight_blue;
			style.Colors[ImGuiCol_ButtonActive] = highlight_blue_active;
			style.Colors[ImGuiCol_Header] = highlight_blue; // selected items (tree, menu bar etc.)
			style.Colors[ImGuiCol_HeaderHovered] = highlight_blue_hovered; // hovered items (tree, menu bar etc.)
			style.Colors[ImGuiCol_HeaderActive] = highlight_blue_active;
			style.Colors[ImGuiCol_Separator] = background_light;
			//style.Colors[ImGuiCol_SeparatorHovered]		= ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
			//style.Colors[ImGuiCol_SeparatorActive]		= ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			style.Colors[ImGuiCol_ResizeGrip] = background_medium;
			style.Colors[ImGuiCol_ResizeGripHovered] = highlight_blue;
			style.Colors[ImGuiCol_ResizeGripActive] = highlight_blue_hovered;
			style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.7f, 0.77f, 1.0f);
			//style.Colors[ImGuiCol_PlotLinesHovered]		= ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogram] = highlight_blue; // Also used for progress bar
			style.Colors[ImGuiCol_PlotHistogramHovered] = highlight_blue_hovered;
			style.Colors[ImGuiCol_TextSelectedBg] = highlight_blue;
			style.Colors[ImGuiCol_PopupBg] = background_dark;
			style.Colors[ImGuiCol_DragDropTarget] = background_light;
			//style.Colors[ImGuiCol_ModalWindowDarkening]	= ImVec4(0.20f, 0.22f, 0.27f, 0.73f);

			auto& io = ImGui::GetIO();
			io.Fonts->AddFontFromFileTTF("assets/calibribold.ttf", font_size);

			static const ImWchar icons_ranges[] = { ICON_MIN_II, ICON_MAX_II, 0 };
			ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
			io.Fonts->AddFontFromFileTTF("assets/ionicons.ttf", 16.0f, &icons_config, icons_ranges);

			static const ImWchar materialDesignIconRanges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
			io.Fonts->AddFontFromFileTTF("assets/materialdesign.ttf", 16.0f, &icons_config, materialDesignIconRanges);

		}

		void System::init()
		{
			if (!glfwInit())
			{
				SOUL_LOG_ERROR("Failed to init glfw");
				return;
			}
				
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

			_db.window = glfwCreateWindow(1920, 1080, "Soul Sandbox", NULL, NULL);
			if (!_db.window)
			{
				SOUL_LOG_ERROR("Failed to create window");
				glfwTerminate();
				return;
			}

			/* Make the window's context current */
			glfwMakeContextCurrent(_db.window);

			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				SOUL_LOG_ERROR("Failed to initialize GLAD");
				return;
			}

			// Setup Dear ImGui binding
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

			ImGui_ImplGlfw_InitForOpenGL(_db.window, true);
			const char* glsl_version = "#version 150";
			ImGui_ImplOpenGL3_Init(glsl_version);

			// Setup style
			ImGui::StyleColorsDark();

			// initialize camera
			Render::Camera& camera = _db.world.camera;
			camera.position = Soul::Vec3f(0.0f, 0.0f, 0.0f);
			camera.direction = Soul::Vec3f(0.0f, 0.0f, -1.0f);
			camera.up = Soul::Vec3f(0.0f, 1.0f, 0.0f);
			camera.perspective.fov = Soul::PI / 4;
			camera.perspective.aspectRatio = 1920 / 1080.0f;
			camera.perspective.zNear = 0.1f;
			camera.perspective.zFar = 30.0f;
			camera.projection = Soul::mat4Perspective(camera.perspective.fov,
				camera.perspective.aspectRatio,
				camera.perspective.zNear,
				camera.perspective.zFar);


			// Initialize buffer and allocate 0 as sentinel
			_db.world.groupEntities.reserve(3000);
			PoolID rootIndex = _db.world.groupEntities.add({});
			_db.world.rootEntityID.index = rootIndex;
			_db.world.rootEntityID.type = EntityType_GROUP;
			GroupEntity* rootEntity = _db.world.groupEntities.ptr(rootIndex);
			rootEntity->entityID = _db.world.rootEntityID;
			rootEntity->first = nullptr;
			strcpy(rootEntity->name, "Root");
			rootEntity->prev = nullptr;
			rootEntity->next = nullptr;
			rootEntity->localTransform = transformIdentity();
			rootEntity->worldTransform = transformIdentity();
			
			_db.world.meshEntities.reserve(10000);
			_db.world.meshEntities.add({});

			_db.world.dirLightEntities.reserve(Render::MAX_DIR_LIGHT);

			_db.world.spotLightEntities.reserve(Render::MAX_SPOT_LIGHT);

			_db.world.materials.reserve(1000);
			_db.world.materials.add({});

			_db.world.textures.reserve(1000);
			_db.world.textures.add({});


			// Initialize render system
			int resWidth, resHeight;
			glfwGetFramebufferSize(_db.window, (int*)&resWidth, &resHeight);
			
			Render::System::Config renderConfig;
			renderConfig.targetWidthPx = resWidth;
			renderConfig.targetHeightPx = resHeight;
			renderConfig.voxelGIConfig.center = Vec3f(0.0f, 0.0f, 0.0f);
			renderConfig.voxelGIConfig.halfSpan = 15.0f;
			renderConfig.voxelGIConfig.resolution = 128;
			renderConfig.shadowAtlasConfig.resolution = Render::TexReso_8192;
			renderConfig.shadowAtlasConfig.subdivSqrtCount[0] = 1;
			renderConfig.shadowAtlasConfig.subdivSqrtCount[1] = 1;
			renderConfig.shadowAtlasConfig.subdivSqrtCount[2] = 2;
			renderConfig.shadowAtlasConfig.subdivSqrtCount[3] = 2;
			_db.world.renderSystem.init(renderConfig);

			_db.world.renderConfig.voxelGIConfig = renderConfig.voxelGIConfig;
			_db.world.renderConfig.shadowAtlasConfig = renderConfig.shadowAtlasConfig;
			_db.world.renderConfig.envConfig.ambientColor = Vec3f(0.0f, 0.0f, 0.0f);
			_db.world.renderConfig.envConfig.ambientEnergy = 0.0f;

			// TODO: Add directional light as an entity
			// Create Directional Light
			Render::DirectionalLightSpec dirLightSpec = {};
			dirLightSpec.bias = 0.005f;
			dirLightSpec.color = Vec3f(1.0f, 1.0f, 1.0f) * 10;
			dirLightSpec.direction = Vec3f(0.03f, -1.0f, 0.35f);
			dirLightSpec.shadowMapResolution = Render::TexReso_4096;
			dirLightSpec.split[0] = 0.1f;
			dirLightSpec.split[1] = 0.2f;
			dirLightSpec.split[2] = 0.5f;

			Transform dirLightTransform;
			dirLightTransform.position = Vec3f(0, 0, 0);
			dirLightTransform.scale = Vec3f(1, 1, 1);
			dirLightTransform.rotation = quaternionIdentity();

			DirLightEntityCreate(&_db.world, rootEntity, "Directional Light", dirLightTransform, dirLightSpec);

			_db.selectedEntity = {};

			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			_ApplyImguiStyle();
		}

		void System::shutdown()
		{
			
			_db.world.groupEntities.cleanup();
			_db.world.meshEntities.cleanup();
			_db.world.dirLightEntities.cleanup();
			_db.world.spotLightEntities.cleanup();
			_db.world.materials.cleanup();
			_db.world.textures.cleanup();

			_db.world.renderSystem.shutdown();

			glfwDestroyWindow(_db.window);
			glfwTerminate();

		}

		void System::run() 
		{
			while (!glfwWindowShouldClose(_db.window)) {
				tick();
			}
		}

		static void _LeftDockBegin() {
			ImGuiIO io = ImGui::GetIO();
			ImGui::SetNextWindowPos(ImVec2(0, 20));
			ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x / 4, io.DisplaySize.y - 20));
			ImGuiWindowFlags leftDockWindowFlags = ImGuiWindowFlags_None;
			leftDockWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			leftDockWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			leftDockWindowFlags |= ImGuiWindowFlags_NoBackground;
			ImGui::Begin("Left Dock", nullptr, leftDockWindowFlags);
			ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
			dockspaceFlags |= ImGuiDockNodeFlags_PassthruDockspace;
			ImGuiID dockspace_id = ImGui::GetID("Left Dock");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		static void _RightDockBegin() {
			ImGuiIO io = ImGui::GetIO();
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 3 / 4, 20));
			ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x / 4, io.DisplaySize.y - 20));
			ImGui::SetNextWindowBgAlpha(0.25f);
			ImGuiWindowFlags leftDockWindowFlags = 0;
			leftDockWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			leftDockWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			ImGui::Begin("Right Dock", nullptr, leftDockWindowFlags);
			ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
			dockspaceFlags |= ImGuiDockNodeFlags_PassthruDockspace;
			ImGuiID dockspace_id = ImGui::GetID("Right Dock");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		static void _DockEnd() {
			ImGui::End();
		}

		void System::tick()
		{
			glfwPollEvents();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();
			ImGuizmo::Enable(true);

			ImGuiIO& io = ImGui::GetIO();

			Render::Camera& camera = _db.world.camera;
			int resWidth, resHeight;
			glfwGetFramebufferSize(_db.window, (int*)&camera.viewportWidth, (int*)&camera.viewportHeight);

			if (!io.WantCaptureMouse && ImGui::IsMouseDown(2)) {
				static float translationSpeed = 1.0f;
					
				float cameraSpeedInc = 0.1f;
				translationSpeed += (cameraSpeedInc * translationSpeed * io.MouseWheel);
				

				if (glfwGetKey(_db.window, GLFW_KEY_M) == GLFW_PRESS) {
					translationSpeed *= 0.9f;
				}
				if (glfwGetKey(_db.window, GLFW_KEY_N) == GLFW_PRESS) {
					translationSpeed *= 1.1f;
				}

				if (ImGui::IsMouseDragging(2)) {

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
				if (glfwGetKey(_db.window, GLFW_KEY_W) == GLFW_PRESS) {
					camera.position += Soul::unit(camera.direction) * translationSpeed;
				}
				if (glfwGetKey(_db.window, GLFW_KEY_S) == GLFW_PRESS) {
					camera.position -= Soul::unit(camera.direction) * translationSpeed;
				}
				if (glfwGetKey(_db.window, GLFW_KEY_A) == GLFW_PRESS) {
					camera.position -= Soul::unit(right) * translationSpeed;
				}
				if (glfwGetKey(_db.window, GLFW_KEY_D) == GLFW_PRESS) {
					camera.position += Soul::unit(right) * translationSpeed;
				}
			}

			if (!io.WantCaptureKeyboard && glfwGetKey(_db.window, GLFW_KEY_1) == GLFW_PRESS) {
				Entity* entity = EntityPtr(&_db.world, _db.selectedEntity);
				camera.position = entity->worldTransform * Vec3f(0.0f, 0.0f, 10.0f);
				camera.direction = unit(entity->worldTransform.position - camera.position);
				camera.up = rotate(entity->worldTransform.rotation, Vec3f(0.0f, 1.0f, 0.0f));
			}
			camera.view = mat4View(camera.position, camera.position + camera.direction, camera.up);
			
			_db.widget.menuBar.tick(&_db);

			// Left Dock
			_LeftDockBegin();
			_db.widget.entityListPanel.tick(&_db);
			_db.widget.renderConfigPanel.tick(&_db);
			_DockEnd();

			// Right Dock
			_RightDockBegin();
			_db.widget.entityDetailPanel.tick(&_db);
			_DockEnd();

			_db.widget.manipulator.tick(&_db);

			if (ImGui::IsMouseClicked(1))
			{
				ImGui::OpenPopup("PieMenu");
			}

			if (BeginPiePopup("PieMenu", 1))
			{
				if (_db.selectedEntity != _db.world.rootEntityID) {
					if (PieMenuItem("Delete")) {
						SOUL_ASSERT(0, _db.selectedEntity != _db.world.rootEntityID, "Cannot delete root entity.");
						EntityDelete(&_db.world, _db.selectedEntity);
						_db.selectedEntity = { 0, 0 };
					}
					if (PieMenuItem("Deselect")) {
						_db.selectedEntity = { 0, 0 };
					}

					if (BeginPieMenu("Mode")) {
						if (PieMenuItem("World", _db.widget.manipulator.operation != ImGuizmo::WORLD)) {
							_db.widget.manipulator.mode = ImGuizmo::WORLD;
						}
						if (PieMenuItem("Local", _db.widget.manipulator.operation != ImGuizmo::LOCAL)) {
							_db.widget.manipulator.mode = ImGuizmo::LOCAL;
						}
						EndPieMenu();
					}

					if (BeginPieMenu("Operation"))
					{
						if (PieMenuItem("Translate", _db.widget.manipulator.operation != ImGuizmo::TRANSLATE))
						{
							_db.widget.manipulator.operation = ImGuizmo::TRANSLATE;
						}
						if (PieMenuItem("Rotate", _db.widget.manipulator.operation != ImGuizmo::ROTATE))
						{
							_db.widget.manipulator.operation = ImGuizmo::ROTATE;
						}
						if (PieMenuItem("Scale", _db.widget.manipulator.operation != ImGuizmo::SCALE))
						{
							_db.widget.manipulator.operation = ImGuizmo::SCALE;
						}
						EndPieMenu();
					}
				}
				else 
				{

				}
				
				EndPiePopup();
			}

			if (_db.selectedEntity.type == EntityType_MESH) {
				MeshEntity* meshEntity = (MeshEntity*)EntityPtr(&_db.world, _db.selectedEntity);
				_db.world.renderSystem.wireframePush(meshEntity->meshRID);
			}

			_db.world.renderSystem.render(camera);

			ImGui::ShowDemoWindow();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(_db.window);

		}

	}
}