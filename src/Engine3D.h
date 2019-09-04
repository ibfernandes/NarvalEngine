#pragma once
#include "ResourceManager.h"
#include "GameStateManager.h"
#include "InputManager.h"
#include "FastNoise.h"
#include "Renderer.h"
#include "defines.h"
#include "Camera.h"
#include "CloudSystem.h"
#include "Texture3D.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"



class Engine3D{
	public:
		GameStateManager GSM;
		Engine3D();
		~Engine3D();
		double startTime;
		double const targetUPSTime = 1.0/TARGET_UPS;
		double previousUpdateTime;
		double previousRenderTime;
		int UPSCount = 0;
		int FPSCount = 0;

		int const GL_CONTEXT_VERSION_MAJOR = 4;
		int const GL_CONTEXT_VERSION_MINOR = 3;
		int const IS_VSYNC_ON = 1;
		GLint WIDTH = 1024, HEIGHT = 512;
		GLFWwindow *window;

		Renderer renderer;
		std::string currentShader = "cloudscape";
		CloudSystem cloudSystem;
		Camera camera;
		glm::mat4 staticCam;

		bool showMainMenu = true;
		bool *p_open = &showMainMenu;
		ImGuiWindowFlags window_flags = 0;

		glm::vec3 absorption = 0.0f * glm::vec3(0.01f);
		glm::vec3 scattering = glm::vec3(0.25, 0.5, 1.0);
		glm::vec3 lightPosition = glm::vec3(-0.5,1.0, 3.5);
		glm::vec3 lightColor =  glm::vec3(1.0, 1.0, 1.0);
		glm::vec3 materialColor =  glm::vec3(1.0, 1.0, 1.0);

		float lightMaxRadius =  1000.0f;
		float densityCoef =  0.75;
		float ambientStrength =  0.1;
		int phaseFunctionOption = 0;
		float g = 0.9;
		float numberOfSteps = 64;
		float param1 = 14.9;
		float param2 = 0.1;
		float param3 = 1.2;
		bool gammaCorrection = false;
		ImFont* robotoFont;

		void initInputManager() {
			glfwSetKeyCallback(window, InputManager::key_callback_handler);

			//Instantiate and init self
			InputManager::getSelf();
		}

		void initCamera() {
			glm::vec3 position(0, 0, 0);
			camera.setPosition(position);
			glm::vec3 front = { 0.0f, 0.0f, 1.0f };
			glm::vec3 up = { 0.0f, 1.0f, 0.0f };
			glm::vec3 side = glm::cross(front, up);
			staticCam = glm::lookAt(position, position + front, up);
		}

		void initImgui() {
			// Setup Dear ImGui binding
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init();

			// Setup style
			ImGui::StyleColorsDark();
			ImGuiStyle * style = &ImGui::GetStyle();

			style->WindowPadding = ImVec2(15, 15);
			style->WindowRounding = 5.0f;
			style->FramePadding = ImVec2(5, 5);
			style->FrameRounding = 4.0f;
			style->ItemSpacing = ImVec2(12, 8);
			style->ItemInnerSpacing = ImVec2(8, 6);
			style->IndentSpacing = 25.0f;
			style->ScrollbarSize = 15.0f;
			style->ScrollbarRounding = 9.0f;
			style->GrabMinSize = 5.0f;
			style->GrabRounding = 3.0f;

			style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
			style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
			style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
			style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
			style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
			style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.0f);
			style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
			style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
			style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
			style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
			style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
			style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
			style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
			style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
			style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
			style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
			style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
			style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
			style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
			style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
			style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
			style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
			style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
			style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
			style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
			style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
			style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

			std::string path = RESOURCES_DIR "fonts/roboto/Roboto-Regular.ttf";
			robotoFont = io.Fonts->AddFontFromFileTTF(path.c_str(), 14);
			
			
		}

		void init() {
			startGLFW();
			initInputManager();
			initCamera();
			cloudSystem.generateCloudNoiseTextures();

			glEnable(GL_TEXTURE_2D);
			//glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);
			glCullFace(GL_BACK);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			initImgui();
		}

		int startGLFW() {
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_CONTEXT_VERSION_MAJOR);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_CONTEXT_VERSION_MINOR);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);


			window = glfwCreateWindow(WIDTH, HEIGHT, "Narval Engine - alpha", nullptr, nullptr);
			if (window == nullptr){
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				return -1;
			}

			glfwMakeContextCurrent(window);
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				return -1;
			}

			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			glViewport(0, 0, width, height);
			glfwSwapInterval(IS_VSYNC_ON);
		}

		void mainLoop() {
			while (!glfwWindowShouldClose(window)) {

				if (glfwGetTime() - startTime >= 1.0) {
					std::cout << "------------------------" << std::endl;
					std::cout << "UPS: " << UPSCount << std::endl;
					std::cout << "FPS: " << FPSCount << std::endl;
					startTime = glfwGetTime();
					UPSCount = 0;
					FPSCount = 0;
				}

				update();
				render();
			}
		}

		void update() {
			if (glfwGetTime() - previousUpdateTime >= targetUPSTime) {
				glfwGetWindowSize(window, &WIDTH, &HEIGHT);
				camera.update();

				previousUpdateTime = glfwGetTime();
				GSM.update();
				updateUI();
				UPSCount++;
			}
			glfwPollEvents();
		}

		void renderSky() {
			glm::mat4 model(1);
			glm::mat4 proj(1);
			model = glm::translate(model, { 0, 0, 1 });
			model = glm::scale(model, { 1, 1, 1 });
			float nearPlane = 1;
			float farPlane = 60000;
			float projAngle = 45;
			proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);


			ResourceManager::getSelf()->getShader(currentShader).use();
			//should be done only once
			ResourceManager::getSelf()->getShader(currentShader).setInteger("perlinWorley", 0);
			glActiveTexture(GL_TEXTURE0);
			cloudSystem.perlinWorley3.bind();

			ResourceManager::getSelf()->getShader(currentShader).setInteger("worley3", 1);
			glActiveTexture(GL_TEXTURE1);
			cloudSystem.worley3.bind();

			ResourceManager::getSelf()->getShader(currentShader).setInteger("curl3", 2);
			glActiveTexture(GL_TEXTURE2);
			cloudSystem.curl3.bind();

			ResourceManager::getSelf()->getShader(currentShader).setInteger("weather", 3);
			glActiveTexture(GL_TEXTURE3);
			ResourceManager::getSelf()->getTexture2D("weather").bind();

			//float time = ((sin(glm::radians(glfwGetTime()*100)) + 1) / 2)*1;
			//float time = 1;
			float time = glfwGetTime() / 2;
			//std::cout << time << std::endl;

			glm::vec3 camPos = *(camera.getPosition());

			ResourceManager::getSelf()->getShader(currentShader).setFloat("time", time);
			//ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setMat4("staticCam", staticCam);

			glm::vec4 sphereCenter = model * glm::vec4(0.5, 0.5, 0.5, 1);

			ResourceManager::getSelf()->getShader(currentShader).setVec3("sphereCenter", sphereCenter.x, sphereCenter.y, sphereCenter.z);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("sphereRadius", 0.4f);

			glm::mat4 invCam = glm::inverse(*camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setMat4("inv_view", invCam);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			glm::mat4 invProj = glm::inverse(proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("inv_proj", invProj);
			ResourceManager::getSelf()->getShader(currentShader).setVec2("screenRes", WIDTH, HEIGHT);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("sunlightDirection", 0, 1, 0);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", camPos.x, camPos.y, camPos.z);

			time = ((sin(glm::radians(glfwGetTime() * 100)) + 1) / 2) * 1;
			ResourceManager::getSelf()->getShader(currentShader).setFloat("crispness", 30 * 0.2);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("curliness", 0.1);

			renderer.render(ResourceManager::getSelf()->getModel("quadTest"));

			/*model = glm::mat4(1);
			model = glm::translate(model, { 0, 0, 5.5 });
			model = glm::scale(model, { 0.2, 0.2, 0.2 });
			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));*/
		}
		
		void updateUI() {
			currentShader = "volume";
			ResourceManager::getSelf()->getShader(currentShader).use();

			ResourceManager::getSelf()->getShader(currentShader).setVec3("absorption", absorption.x, absorption.y, absorption.z);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("scattering", scattering.x, scattering.y, scattering.z);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("lightColor", lightColor.x, lightColor.y, lightColor.z);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("materialColor", materialColor.x, materialColor.y, materialColor.z);

			ResourceManager::getSelf()->getShader(currentShader).setFloat("densityCoef", densityCoef);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("lightMaxRadius", lightMaxRadius);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("ambientStrength", ambientStrength);

			ResourceManager::getSelf()->getShader(currentShader).setFloat("phaseFunctionOption", phaseFunctionOption);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("numberOfSteps", numberOfSteps);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("g", g);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("gammaCorrection", gammaCorrection);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("param1", param1);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("param2", param2);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("param3", param3);
		}

		void renderImGUI() {
			// Show GUI
			// Start the ImGui frame (For Each new frame)
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			//ImGui::PushFont(robotoFont);

			ImGui::SetNextWindowPos(ImVec2(4, 4), ImGuiCond_Once); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
			ImGui::SetNextWindowSize(ImVec2(400, 550), ImGuiCond_Once);
			ImGui::Begin("Volumetric Menu", p_open, window_flags);
			ImGui::DragFloat3("Absorption (1/m)", &absorption[0], 0.001, 0.001, 5.0);
			ImGui::DragFloat3("Scattering (1/m)", &scattering[0], 0.001, 0.001, 5.0);
			ImGui::DragFloat3("Light Pos (m)", &lightPosition[0], 0.05, -30.0, 30.0);
			ImGui::DragFloat3("L. color", &lightColor[0], 0.01, 0.0, 30.0);

			ImGui::DragFloat("Density Multiplier", &densityCoef, 0.01, 0.0, 30.0);
			ImGui::DragFloat("Ambient Strength", &ambientStrength, 0.01, 0.0, 20.0);
			ImGui::DragFloat("g", &g, 0.01, -1.0, 1.0);
			ImGui::DragFloat("N. of Steps", &numberOfSteps, 1, 0.0, 256);

			ImGui::DragFloat("param1", &param1, 0.1, -100, 100);
			ImGui::DragFloat("param2", &param2, 0.1, -100, 100);
			ImGui::DragFloat("param3", &param3, 0.1, -100, 100);

			ImGui::Checkbox("Gamma Correction", &gammaCorrection);

			const char* items[] = { "Isotropic", "Rayleigh", "Henyey-Greenstein"};
			ImGui::Combo("Phase Function", &phaseFunctionOption, items, IM_ARRAYSIZE(items));

			ImGui::End();
			
			// Imgui Render Calls (For Each end frame)
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		void renderNoiseTexture() {
			glm::mat4 model(1);
			glm::mat4 proj(1);
			float nearPlane = 1;
			float farPlane = 60000;
			float projAngle = 45;
			proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);
			proj = glm::ortho(0.f, -(float)WIDTH, 0.f, (float)HEIGHT, -1.f, 1.f);

			std::string currentShader = "screentex";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::translate(model, { WIDTH / 2, HEIGHT / 2, 0 });
			model = glm::scale(model, { 512, 512, 1 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);

			ResourceManager::getSelf()->getShader(currentShader).setInteger("volume", 0);
			glActiveTexture(GL_TEXTURE0);
			cloudSystem.perlinWorley3.bind();

			renderer.render(ResourceManager::getSelf()->getModel("quadTest"));
		}

		void renderGradientBackground() {
			glDisable(GL_DEPTH_TEST);

			glm::mat4 model(1);
			glm::mat4 proj(1);
			float nearPlane = 1;
			float farPlane = 60000;
			float projAngle = 45;
			proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);
			proj = glm::ortho(0.f, -(float)WIDTH, 0.f, (float)HEIGHT, -1.f, 1.f);

			std::string currentShader = "gradientBackground";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::translate(model, { WIDTH / 2, HEIGHT / 2, 0 });
			model = glm::scale(model, { 512, 512, 1 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);


			renderer.render(ResourceManager::getSelf()->getModel("quadTest"));

			glEnable(GL_DEPTH_TEST);
		}

		void renderVolume() {
			glm::mat4 model(1);
			glm::mat4 proj(1);
			float nearPlane = 1;
			float farPlane = 60000;
			float projAngle = 45;
			proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);

			std::string currentShader = "monocolor";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::translate(model, lightPosition);
			model = glm::scale(model, { 0.1, 0.1, 0.1 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", 1,0,0,1);

			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));


			currentShader = "volume";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::mat4(1);
			model = glm::translate(model, { -1.2, -0.5, 3 });
			model = glm::scale(model, { 1, 1, 1 });

			ResourceManager::getSelf()->getShader(currentShader).setInteger("volume", 0);
			glActiveTexture(GL_TEXTURE0);
			cloudSystem.perlinWorley3.bind();

			float time = ((sin(glm::radians(glfwGetTime() * 100)) + 1) / 2) * 1;
			float continuosTime = glfwGetTime()/6;
			ResourceManager::getSelf()->getShader(currentShader).setFloat("time", time);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("continuosTime", continuosTime);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
			glm::vec3 camPos = *(camera.getPosition());
			ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", camPos.x, camPos.y, camPos.z);

			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));

		}

		void render() {
			glViewport(0, 0, WIDTH, HEIGHT);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderGradientBackground();
			renderVolume();
			//renderSky();

			previousRenderTime = glfwGetTime();
			GSM.render();
			//render2DVolume();
			renderImGUI();

			FPSCount++;
			glfwSwapBuffers(window);
		}
};

