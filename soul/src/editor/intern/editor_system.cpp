#include "core/dev_util.h"
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
#include "editor/intern/demo/demo.h"

namespace Soul {
	namespace Editor {

		static void _ApplyImguiStyle() {

			auto& style = ImGui::GetStyle();
			style.ScrollbarSize = 8;
			style.GrabMinSize = 8;
			style.FrameBorderSize = 1;
			style.TabBorderSize = 1;
			style.WindowRounding = 0;
			style.TabRounding = 0;

			const auto font_size = 16.0f;

			ImVec4* colors = ImGui::GetStyle().Colors;
			colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.94f, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
			colors[ImGuiCol_Border] = ImVec4(0.196f, 0.196f, 0.445f, 0.500f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 0.40f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.38f, 0.38f, 0.38f, 0.67f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(1.000f, 0.000f, 0.392f, 0.310f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_Button] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(1.000f, 0.000f, 0.392f, 0.310f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.80f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
			colors[ImGuiCol_Separator] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
			colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.78f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 0.00f, 0.43f, 0.25f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.00f, 0.43f, 0.67f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.00f, 0.43f, 0.95f);
			colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
			colors[ImGuiCol_TabActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
			colors[ImGuiCol_DockingPreview] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(0.96f, 0.26f, 0.98f, 0.35f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.43f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

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

			_db.world.pointLightEntities.reserve(Render::MAX_POINT_LIGHT);

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
			renderConfig.shadowAtlasConfig.subdivSqrtCount[1] = 2;
			renderConfig.shadowAtlasConfig.subdivSqrtCount[2] = 8;
			renderConfig.shadowAtlasConfig.subdivSqrtCount[3] = 8;
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
			ImVec4* colors = ImGui::GetStyle().Colors;



			_db.demo = nullptr;
		}

		void System::shutdown()
		{
			
			_db.world.groupEntities.cleanup();
			_db.world.meshEntities.cleanup();
			_db.world.dirLightEntities.cleanup();
			_db.world.pointLightEntities.cleanup();
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
			
			if (!_db.widget.menuBar.hide) {
				// Left Dock
				_LeftDockBegin();
				_db.widget.entityListPanel.tick(&_db);
				_db.widget.renderConfigPanel.tick(&_db);
				ImGui::ShowDemoWindow();
				_DockEnd();

				// Right Dock
				_RightDockBegin();
				_db.widget.entityDetailPanel.tick(&_db);
				_DockEnd();

				
			}
			
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

			if (_db.demo != nullptr) {
				_db.demo->tick(&_db);
			}

			_db.world.renderSystem.render(camera);

			
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(_db.window);

		}

	}
}