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
		int lastFPS = 0;
		int lastUPS = 0;

		int const GL_CONTEXT_VERSION_MAJOR = 4;
		int const GL_CONTEXT_VERSION_MINOR = 3;
		int const IS_VSYNC_ON = 1;
		GLint WIDTH = 1600, HEIGHT = 900;
		GLFWwindow *window;

		Renderer renderer;
		std::string currentShader = "cloudscape";
		CloudSystem cloudSystem;
		Camera camera;
		glm::mat4 staticCam;

		bool showMainMenu = true;
		bool *p_open = &showMainMenu;
		ImGuiWindowFlags window_flags = 0;

		glm::vec3 absorption = glm::vec3(0.001f);
		glm::vec3 scattering = glm::vec3(0.25, 0.5, 1.0);
		glm::vec3 lightPosition = glm::vec3(0.0, 0.0, 4.0);
		glm::vec3 lightColor =  glm::vec3(1.0, 1.0, 1.0);
		glm::vec3 materialColor =  glm::vec3(1.0, 1.0, 1.0);

		glm::vec3 gradientDownNight =  glm::vec3( 57/255.0f, 65/255.0f, 73/255.0f);
		glm::vec3 gradientUpNight = glm::vec3(19 / 155.0f, 21 / 255.0f, 23 / 255.0f);
		glm::vec3 gradientDownDay =  glm::vec3( 236/255.0f, 240/255.0f, 241/255.0f);
		glm::vec3 gradientUpDay =  glm::vec3(186/255.0f, 199/255.0f, 200/255.0f);

		float lightMaxRadius =  1000.0f;
		float densityCoef =  0.75;
		float ambientStrength =  0.1;
		int phaseFunctionOption = 2;
		float g = 0.9;
		float numberOfSteps = 128;
		float shadowSteps = 16;
		float param1 = 14.9;
		float param2 = 0.1;
		float param3 = 1.2;
		float theta = 1.2;
		float SPP = 16.0;
		bool gammaCorrection = false;
		bool pathTracing = false;
		bool lockScattering = true;
		bool nightMode = false;
		bool animate = false;
		bool enableShadow = true;
		ImFont* robotoFont;

		void initInputManager() {
			glfwSetKeyCallback(window, InputManager::key_callback_handler);
			InputManager::getSelf();
		}

		void initCamera() {
			glm::vec3 position(0, 0, -6);
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
			//cloudSystem.generateCloudNoiseTextures();

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
					//std::cout << "------------------------" << std::endl;
					//std::cout << "UPS: " << UPSCount << std::endl;
					//std::cout << "FPS: " << FPSCount << std::endl;
					lastFPS = FPSCount;
					lastUPS= UPSCount;

					startTime = glfwGetTime();
					UPSCount = 0;
					FPSCount = 0;
				}

				update();
				//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				//render();
				//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
			ResourceManager::getSelf()->getShader(currentShader).setFloat("shadowSteps", shadowSteps);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("g", g);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("gammaCorrection", gammaCorrection);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("param1", param1);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("param2", param2);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("param3", param3);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("SPP", SPP);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("enablePathTracing", pathTracing);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("enableAnimation", animate);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("enableShadow", enableShadow);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("theta", theta);

			if (nightMode) {
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientUp", gradientUpNight.x, gradientUpNight.y, gradientUpNight.z);
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientDown", gradientDownNight.x, gradientDownNight.y, gradientDownNight.z);
			}else {
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientUp", gradientUpDay.x, gradientUpDay.y, gradientUpDay.z);
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientDown", gradientDownDay.x, gradientDownDay.y, gradientDownDay.z);
			}
		}

		void renderImGUI() {
			// Show GUI
			// Start the ImGui frame (For Each new frame)
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);
			ImGui::Begin("Performance", p_open, window_flags);
				ImGui::Text("FPS:   %d", lastFPS);
				ImGui::Text("UPS:   %d", lastUPS);
			ImGui::End();

			ImGui::SetNextWindowPos(ImVec2(40, 170), ImGuiCond_Once); 
			ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Once);
			ImGui::Begin("Volume Properties", p_open, window_flags);
				ImGui::DragFloat3("Absorption (1/m)", &absorption[0], 0.001f, 0.001f, 5.0f);
				ImGui::DragFloat3("Scattering (1/m)", &scattering[0], 0.001f, 0.001f, 5.0f);
				if (lockScattering) {
					scattering[1] = scattering[0];
					scattering[2] = scattering[0];
				}
				ImGui::Checkbox("Lock Scattering", &lockScattering);

				ImGui::DragFloat("Density Multiplier", &densityCoef, 0.01, 0.0, 30.0);
			ImGui::End();
			
			ImGui::SetNextWindowPos(ImVec2(40, 410), ImGuiCond_Once); 
			ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
			ImGui::Begin("Light Properties", p_open, window_flags);
				ImGui::DragFloat3("Light Pos (m)", &lightPosition[0], 0.1, 0.1, 0.1);
				ImGui::DragFloat3("L. color", &lightColor[0], 0.01, 0.0, 30.0);

				ImGui::DragFloat("Ambient Strength", &ambientStrength, 0.1, 0.0, 100.0);
				ImGui::DragFloat("g", &g, 0.01, -1.0, 1.0);
				ImGui::DragFloat("N. of Steps", &numberOfSteps, 1, 0.0, 256);
				ImGui::DragFloat("Shadow Steps", &shadowSteps, 1, 0.0, 256);
				ImGui::Checkbox("Enable Shadow", &enableShadow);

				ImGui::Checkbox("Path Tracing", &pathTracing);
				ImGui::DragFloat("SPP", &SPP, 1.0, 0, 10000);

				ImGui::Checkbox("Night Mode", &nightMode);

				const char* items[] = { "Isotropic", "Rayleigh", "Henyey-Greenstein" };
				ImGui::Combo("Phase Function", &phaseFunctionOption, items, IM_ARRAYSIZE(items));
			ImGui::End();

			ImGui::SetNextWindowPos(ImVec2(40, 810), ImGuiCond_Once); 
			ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);
			ImGui::Begin("Test", p_open, window_flags);
			
				ImGui::DragFloat("Theta", &theta, 0.1, 0.0, 100.0);
			
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
			proj = glm::ortho(0.f, (float)WIDTH, 0.f, (float)HEIGHT, -8.f, 8.f);

			std::string currentShader = "gradientBackground";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::translate(model, { WIDTH / 2, HEIGHT / 2, 0 });
			model = glm::scale(model, { WIDTH, HEIGHT, 1 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);

			if (nightMode) {
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientUp", gradientUpNight.x, gradientUpNight.y, gradientUpNight.z);
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientDown", gradientDownNight.x, gradientDownNight.y, gradientDownNight.z);
			}
			else {
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientUp", gradientUpDay.x, gradientUpDay.y, gradientUpDay.z);
				ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientDown", gradientDownDay.x, gradientDownDay.y, gradientDownDay.z);
			}

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

			//Light position
			std::string currentShader = "monocolor";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::translate(model, lightPosition);
			model = glm::scale(model, { 0.1, 0.1, 0.1 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", 1,0,0,1);

			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));

			//Origin position
			model = glm::mat4(1);
			model = glm::translate(model, glm::vec3(0,0,0));
			model = glm::scale(model, { 0.1, 0.1, 0.1 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", 1, 1, 1, 1);

			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));

			currentShader = "volume";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::mat4(1);
			model = glm::translate(model, { -0.5, -0.5, -0.5 });
			//model = glm::scale(model, { 2, 2, 2 });

			ResourceManager::getSelf()->getShader(currentShader).setInteger("volume", 0);
			glActiveTexture(GL_TEXTURE0);
			ResourceManager::getSelf()->getTexture3D("cloud").bind();

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

