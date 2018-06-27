#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#include "core/type.h"
#include "soul_core.h"
#include "core/math.h"
#include "render/type.h"
#include "render/system.h"
#include "render/intern/util.h"
#include "extern/imgui.h"
#include "extern/imgui_impl_glfw.h"
#include "extern/imgui_impl_opengl3.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <chrono>

void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

namespace Soul {
	GLFWwindow* window;
	RenderSystem renderSystem;
	Camera camera;
	RenderRID sunRID;

	struct SPMFormat
	{
		const char* header = "SPMF";
		byte nameLength;
		char* name;
		uint32 attributes;
		uint32 vertexBufferSize;
		byte* vertexData;
		uint32 indexBufferSize;
		byte* indexData;
		const char* footer = "1234";
	};

	char* LoadFile(const char* filepath) {
		FILE *f = fopen(filepath, "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);  //same as rewind(f);

		char *string = (char*) malloc(fsize + 1);
		fread(string, fsize, 1, f);
		fclose(f);

		string[fsize] = 0;
		std::cout << string << std::endl;
		return string;
	}

	byte* ReadBytes(FILE* file, byte* buffer, uint32 size)
	{
		fread(buffer, 1, size, file);
		return buffer;
	}

	void LoadMesh(const char* path, Vertex* vertexBuffer, uint32* indexBuffer, uint32* vertexCount, uint32* indexCount)
	{

		SPMFormat format;

		FILE* f = fopen(path, "rb");
		{
			byte header[4];
			ReadBytes(f, header, 4);
		}

		{
			byte buffer[1];
			ReadBytes(f, buffer, 1);
			format.nameLength = *buffer;
		}

		{
			format.name = new char[format.nameLength + 1];
			ReadBytes(f, (byte*)format.name, format.nameLength);
			format.name[format.nameLength] = '\0';
		}

		{
			byte buffer[4];
			ReadBytes(f, buffer, 4);
			format.attributes = *(uint32*)buffer;
		}

		{
			byte buffer[4];
			ReadBytes(f, buffer, 4);
			format.vertexBufferSize = *(uint32*)buffer;
		}

		{
			format.vertexData = new byte[format.vertexBufferSize];
			ReadBytes(f, format.vertexData, format.vertexBufferSize);
		}

		{
			byte buffer[4];
			ReadBytes(f, buffer, 4);
			format.indexBufferSize = *(uint32*)buffer;
		}

		{
			format.indexData = new byte[format.indexBufferSize];
			ReadBytes(f, format.indexData, format.indexBufferSize);
		}

		{
			byte footer[4];
			ReadBytes(f, footer, 4);
		}
		
		fclose(f);

		*vertexCount = format.vertexBufferSize / (14 * sizeof(float));
		float* vertexData = (float*) format.vertexData;
		uint32 vertexSize = 14;
		for (int i = 0; i < *vertexCount; i++) {
			uint32 base = i * vertexSize;
			vertexBuffer[i].pos = Vec3f(vertexData[base + 0], vertexData[base + 1], vertexData[base + 2]);
			vertexBuffer[i].normal = unit(Vec3f(vertexData[base + 3], vertexData[base + 4], vertexData[base + 5]));
			vertexBuffer[i].texUV = { vertexData[base + 6], vertexData[base + 7] };
			vertexBuffer[i].binormal = { vertexData[base + 8], vertexData[base + 9], vertexData[base + 10] };
			vertexBuffer[i].tangent = { vertexData[base + 11], vertexData[base + 12], vertexData[base + 13] };
		}

		*indexCount = format.indexBufferSize / sizeof(uint32);
		uint32* indexData = (uint32*)format.indexData;
		for (int i = 0; i < *indexCount; i++) {
			indexBuffer[i] = indexData[i];
			// std::cout << indexBuffer[i] << std::endl;
		}
	}

	void LoadMaterialBallScene() {
		TextureSpec texSpec;
		texSpec.pixelFormat = PF_RGBA;
		texSpec.minFilter = GL_LINEAR_MIPMAP_LINEAR;
		texSpec.magFilter = GL_LINEAR;
		int numChannels;
		stbi_set_flip_vertically_on_load(true);

		{
			Vertex vertexes[10000];
			uint32 vertexCount;
			uint32 indexes[10000];
			uint32 indexCount;
			LoadMesh("../soul/assets/Sphere.spm", vertexes, indexes, &vertexCount, &indexCount);

            std::string material[5] = {
                    "gold",
                    "grass",
                    "plastic",
                    "rusted_iron",
                    "wall"
            };

			for (int i = 0; i < 5; i++) {

			    std::string prefix = "../soul/assets/";
			    std::string albedo = "";
			    albedo += prefix;
			    albedo += material[i];
			    albedo += "/albedo.png";
			    std::string ao = "";
			    ao += prefix;
			    ao += material[i];
			    ao += "/ao.png";
			    std::string metallic = "";
			    metallic += prefix;
			    metallic += material[i];
			    metallic += "/metallic.png";
			    std::string normal = "";
			    normal += prefix;
			    normal += material[i];
			    normal += "/normal.png";
			    std::string roughness = "";
			    roughness += prefix;
			    roughness += material[i];
			    roughness += "/roughness.png";

                unsigned char* rustedIronAlbedoRaw = stbi_load(albedo.c_str(), &texSpec.width, &texSpec.height, &numChannels, 0);
                RenderRID albedoID = renderSystem.textureCreate(texSpec, rustedIronAlbedoRaw, numChannels);
                stbi_image_free(rustedIronAlbedoRaw);

                unsigned char* rustedIronMetallicRaw = stbi_load(metallic.c_str(), &texSpec.width, &texSpec.height, &numChannels, 0);
                RenderRID metallicID = renderSystem.textureCreate(texSpec, rustedIronMetallicRaw, numChannels);
                stbi_image_free(rustedIronMetallicRaw);

                unsigned char* rustedIronNormalRaw = stbi_load(normal.c_str(), &texSpec.width, &texSpec.height, &numChannels, 0);
                RenderRID normalID = renderSystem.textureCreate(texSpec, rustedIronNormalRaw, numChannels);
                stbi_image_free(rustedIronNormalRaw);

                unsigned char* rustedIronRoughnessRaw = stbi_load(roughness.c_str(), &texSpec.width, &texSpec.height, &numChannels, 0);
                RenderRID roughnessID = renderSystem.textureCreate(texSpec, rustedIronRoughnessRaw, numChannels);
                stbi_image_free(rustedIronRoughnessRaw);

                unsigned char* aoRaw = stbi_load(ao.c_str(), &texSpec.width, &texSpec.height, &numChannels, 0);
                RenderRID aoID = renderSystem.textureCreate(texSpec, aoRaw, numChannels);
                stbi_image_free(aoRaw);

                MaterialSpec materialSpec = {
                        albedoID,
                        normalID,
                        metallicID,
                        roughnessID,
                        aoID,
                        0
                };
                RenderRID materialID = renderSystem.createMaterial(materialSpec);
				MeshSpec meshSpec = {
						mat4Translate(Vec3f(0, 0, i * -10)),
						vertexes,
						indexes,
						vertexCount,
						indexCount,
						materialID
				};
				renderSystem.meshCreate(meshSpec);
			}
		}

	}

	void LoadSponzaScene() {

		stbi_set_flip_vertically_on_load(true);

		std::string models[] = { "arch", "backplate", "walls1", "walls2", "walls3", "ceiling", "column1", "column2",
						   		 "column3", "curtain_blue", "curtain_green", "curtain_red", "details",
								 "fabric_blue", "fabric_green", "fabric_red", "floor", "floor2",
								 "chain", "lion", "pole", "roof", "vase", "vase_round", "vase_hanging"};

		int modelCount = sizeof(models) / sizeof(*models);

		for (int i = 0; i < modelCount; i++) {
			std::string baseDir = "C:/Dev/soul/soul/assets/sponza/";

			std::string meshFilename = baseDir + models[i] + "/model.spm";
			std::string albedoFilename = baseDir + models[i] + "/albedo.tga";
			std::string metallicFilename = baseDir + models[i] + "/metallic.tga";
			std::string roughnessFilename = baseDir + models[i] + "/roughness.tga";
			std::string normalFilename = baseDir + models[i] + "/normal.tga";

			Vertex *vertexes = (Vertex*)malloc(sizeof(Vertex) * 100000);
			uint32 vertexCount;
			uint32 *indexes = (uint32*)malloc(sizeof(uint32) * 100000);
			uint32 indexCount;
			LoadMesh(meshFilename.c_str(), vertexes, indexes, &vertexCount, &indexCount);

			TextureSpec texSpec;
			texSpec.pixelFormat = PF_RGBA;
			int numChannel;

			unsigned char* albedoRaw = stbi_load(albedoFilename.c_str(),
					&texSpec.width, &texSpec.height, &numChannel, 0);
			texSpec.minFilter = GL_LINEAR_MIPMAP_LINEAR;
			texSpec.magFilter = GL_LINEAR;
			RenderRID albedoID = renderSystem.textureCreate(texSpec, albedoRaw, numChannel);
			if (albedoRaw == nullptr) std::cout<<"Albedo file does not exist"<<std::endl;
			stbi_image_free(albedoRaw);

			RenderUtil::GLErrorCheck("LoadSponzaScene::After Albedo");

			unsigned char* metallicRaw = stbi_load(metallicFilename.c_str(),
					&texSpec.width, &texSpec.height, &numChannel, 0);
			if (models[i] == "floor") {
				for (int j = 0; j < texSpec.height; j++) {
					for (int k = 0; k < texSpec.width; k++) {
						metallicRaw[j * texSpec.width + k] = 50;
					}
				}
			}
			RenderRID metallicID = renderSystem.textureCreate(texSpec, metallicRaw, numChannel);
			if (metallicRaw == nullptr) std::cout<<"Metallic file does not exist"<<std::endl;
			stbi_image_free(metallicRaw);

			RenderUtil::GLErrorCheck("LoadSponzaScene::AfterMetallic");


			unsigned char* roughnessRaw = stbi_load(roughnessFilename.c_str(),
					&texSpec.width, &texSpec.height, &numChannel, 0);
			std::cout << "Num Channel Roughness: " << numChannel << std::endl;
			if (models[i] == "floor") {
				for (int j = 0; j < texSpec.height; j++) {
					for (int k = 0; k < texSpec.width; k++) {
						roughnessRaw[j * texSpec.width + k] = 70;
					}
				}
			}
			RenderRID roughnessID = renderSystem.textureCreate(texSpec, roughnessRaw, numChannel);
			if (roughnessRaw == nullptr) std::cout<<"Roughness map does not exist"<<std::endl;
			stbi_image_free(roughnessRaw);
			RenderUtil::GLErrorCheck("LoadSponzaScene::AfterRoughness");

			unsigned char* normalRaw = stbi_load(normalFilename.c_str(),
					&texSpec.width, &texSpec.height, &numChannel, 0);
			std::cout << "Num Channel Normal: " << numChannel << std::endl;
			/*if (models[i] == "floor") {
				for (int j = 0; j < texSpec.height; j++) {
					for (int k = 0; k < texSpec.width; k++) {
						normalRaw[j * texSpec.width * 4 + k * 4] = 0;
						normalRaw[j * texSpec.width * 4 + k * 4 + 1] = 0;
						normalRaw[j * texSpec.width * 4 + k * 4 + 2] = 1;
					}
				}
			}*/
			RenderRID normalID = renderSystem.textureCreate(texSpec, normalRaw, numChannel);
			if (normalRaw == nullptr) std::cout<<"Normal map does not exist"<<std::endl;
			stbi_image_free(normalRaw);
			RenderUtil::GLErrorCheck("LoadSponzaScene::AfterNormal");

			unsigned char* aoRaw = stbi_load("C:/Dev/soul/soul/assets/sponza/ao.png",
					&texSpec.width, &texSpec.height, &numChannel, 0);
			if (aoRaw == nullptr) std::cout<<"Ao map does not exist"<<std::endl;
			RenderRID aoID = renderSystem.textureCreate(texSpec, aoRaw, numChannel);
			stbi_image_free(aoRaw);
			RenderUtil::GLErrorCheck("LoadSponzaScene::AfterAO");

			MaterialSpec materialSpec = {
					albedoID,
					normalID,
					metallicID,
					roughnessID,
					aoID,
					0
			};
			RenderRID materialID = renderSystem.createMaterial(materialSpec);
			MeshSpec meshSpec = {
					mat4Translate(Vec3f(0, 0, 0)) * mat4Rotate(Vec3f(1.0f, 0.0f, 0.0f), - PI / 2),
					vertexes,
					indexes,
					vertexCount,
					indexCount,
					materialID
			};
			renderSystem.meshCreate(meshSpec);

			RenderUtil::GLErrorCheck("LoadSponzaScene::CreateMesh");

			free(vertexes);
			free(indexes);
		}


	}

	void framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		// make sure the viewport matches the new window dimensions; note that width and
		// height will be significantly larger than specified on retina displays.
		glViewport(0, 0, width, height);
		std::cout<<"framebuffer_size_callback"<<" "<<width<<" "<<height<<std::endl;
	}

	int Init() {

		if (!glfwInit())
			return -1;

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

		

		/* Create a windowed mode window and its OpenGL context */
		window = glfwCreateWindow(640 * 2, 480 * 2, "Soul Engine", NULL, NULL);
		if (!window)
		{
			glfwTerminate();
			return -1;
		}

		std::cout << "Make context current" << std::endl;
		/* Make the window's context current */
		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return -1;
		}

		GLint flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
		{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(glDebugOutput, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
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


		RenderSystem::Config renderConfig;
		int resWidth, resHeight;
		glfwGetFramebufferSize(window, (int*)&resWidth, &resHeight);
		renderConfig.targetWidthPx = resWidth;
		renderConfig.targetHeightPx = resHeight;
		renderConfig.voxelGIConfig.center = Vec3f(0.0f, 0.0f, 0.0f);
		renderConfig.voxelGIConfig.halfSpan = 1800.0f;
		renderConfig.voxelGIConfig.resolution = 256;
		renderSystem.init(renderConfig);

		camera.position = Vec3f(0.0f, 0.0f, 0.0f);
		camera.direction = Vec3f(0.0f, 0.0f, 1.0f);
		camera.up = Vec3f(0.0f, 1.0f, 0.0f);
		camera.perspective.fov = PI / 4;
		camera.perspective.aspectRatio = 640.0f / 480.0f;
		camera.perspective.zNear = 0.1f;
		camera.perspective.zFar = 4000.0f;
		camera.projection = mat4Perspective(camera.perspective.fov,
											camera.perspective.aspectRatio,
											camera.perspective.zNear,
											camera.perspective.zFar);

		LoadSponzaScene();

		// LoadMaterialBallScene();

        {
            stbi_set_flip_vertically_on_load(true);
            int width, height, nrComponents;
            float *data = stbi_loadf("C:/Dev/soul/soul/assets/newport_loft.hdr", &width, &height, &nrComponents, 0);

            GLuint hdrTexture;
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            renderSystem.setPanorama(hdrTexture);

            stbi_image_free(data);
        }


		DirectionalLightSpec lightSpec;
		lightSpec.direction = Vec3f(0.03f, -1.0f, 0.35f);
		lightSpec.color = Vec3f(1.0f, 1.0f, 1.0f) * 100;
		lightSpec.shadowMapResolution = TR_4096;
		sunRID = renderSystem.createDirectionalLight(lightSpec);

		RenderUtil::GLErrorCheck("Init::end");

		return 0;
	}

	int MainLoop() {
		RenderUtil::GLErrorCheck("MainLoop::before_loop");
		while (!glfwWindowShouldClose(window))
		{

			RenderUtil::GLErrorCheck("MainLoop::begin");

			/* Poll for and process events */
			glfwPollEvents();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			int resWidth, resHeight;
			glfwGetFramebufferSize(window, (int*)&camera.viewport_width, (int*)&camera.viewport_height);

			static float translationSpeed = 5.0f;
			if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
				translationSpeed *= 0.9f;
			}
			if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
				translationSpeed *= 1.1f;
			}
			
			Vec3f right = unit(cross(camera.direction, camera.up));
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				camera.position += unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				camera.position -= unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				camera.position -= unit(right) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				camera.position += unit(right) * translationSpeed;
			}

			Vec3f cameraRight = cross(camera.up, camera.direction) * -1.0f;
			float rotateSpeed = 0.01f;
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
				camera.direction = mat4Rotate(camera.up, rotateSpeed * PI) * camera.direction;
			}
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
				camera.direction = mat4Rotate(camera.up, -1 * rotateSpeed * PI) * camera.direction;
			}
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				Mat4 rotate = mat4Rotate(cameraRight, rotateSpeed * PI);
				camera.direction = rotate * camera.direction;
				camera.up = rotate * camera.up;
			}
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				Mat4 rotate = mat4Rotate(cameraRight, -1 * rotateSpeed * PI);
				camera.direction = rotate * camera.direction;
				camera.up = rotate * camera.up;
			}

			/* Render here */
			renderSystem.render(camera);

			ImGui::Begin("Demo Scene Metric");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);

			ImGui::Text("Position : (%.3f,%.3f,%.3f)", camera.position.x, camera.position.y, camera.position.z);

            static Vec3f sunDir = Vec3f(0.03f, -1.0f, 0.35f);
            ImGui::SliderFloat3("Sun Direction", (float*) &sunDir, -1.0f, 1.0f);
            renderSystem.setDirLightDirection(sunRID, sunDir);

            static Vec3f ambientColor = Vec3f(1.0f, 1.0f, 1.0f);
            static float ambientEnergy = 0.03f;
            ImGui::SliderFloat("Ambient energy", &ambientEnergy, 0.0f, 0.1f);
            ImGui::SliderFloat3("Ambient color", (float*) &ambientColor, 0.0f, 1.0f);
            renderSystem.setAmbientEnergy(ambientEnergy);
            renderSystem.setAmbientColor(ambientColor);

			ImGui::End();
			
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

		}
		return 0;
	}

	int Terminate() {
		glfwTerminate();
		return 0;
	}
}