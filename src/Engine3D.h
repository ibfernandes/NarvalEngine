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
#include "VolumeScene.h"
#include "ModelScene.h"
#include "FBO.h"
#include <stdlib.h>
#include <time.h> 
#include "SphericalHarmonics.h"

class Engine3D{
	public:
		GameStateManager GSM;
		Engine3D();
		~Engine3D();
		double startTime;
		double const targetUPSTime = 1.0/TARGET_UPS;
		double previousUpdateTime;
		double previousRenderTime;
		float accumulator = 0;
		float alphaInterpolator = 1;
		int UPSCount = 0;
		int FPSCount = 0;
		int lastFPS = 0;
		int lastUPS = 0;

		int const GL_CONTEXT_VERSION_MAJOR = 4;
		int const GL_CONTEXT_VERSION_MINOR = 3;
		int const IS_VSYNC_ON = 0;
		GLint WIDTH = 1280, HEIGHT = 720;
		GLFWwindow *window;

		Renderer renderer;
		Camera camera;
		glm::mat4 staticCam;

		bool showMainMenu = true;
		bool *p_open = &showMainMenu;
		ImGuiWindowFlags window_flags = 0;
		glm::mat4 proj;
		glm::mat4 model = glm::mat4(1);
		
		bool nightMode = false;
		int currentScene = 0;
		const char *scenes[2] = { "Volume model", "3D model" };
		static const int MAX_GRAPH_POINTS = 100;
		float FPSgraphPoints[MAX_GRAPH_POINTS];
		float UPSgraphPoints[MAX_GRAPH_POINTS];
		ImFont* robotoFont;

		void initInputManager() {
			glfwSetKeyCallback(window, InputManager::key_callback_handler);
			InputManager::getSelf();
		}

		void initCamera() {
			glm::vec3 position(0, 0, -3);
			camera.setPosition(position);
			glm::vec3 front = { 0.0f, 0.0f, 1.0f };
			glm::vec3 up = { 0.0f, 1.0f, 0.0f };
			glm::vec3 side = glm::cross(front, up);
			staticCam = glm::lookAt(position, position + front, up);
		}

		void initImgui() {

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
	
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

			glEnable(GL_TEXTURE_2D);
			glFrontFace(GL_CCW);
			glCullFace(GL_BACK);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			float nearPlane = 1;
			float farPlane = 60000;
			float projAngle = 45;
			proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);

			VolumeScene *vs = new VolumeScene;
			vs->init(WIDTH, HEIGHT, &renderer, &camera);
			GSM.addState(vs);

			ModelScene *mods = new ModelScene;
			mods->init(WIDTH, HEIGHT, &renderer, &camera);
			GSM.addState(mods);
				
			GSM.currentScene = currentScene;

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
					lastFPS = FPSCount;
					lastUPS = UPSCount;
					
					FPSgraphPoints[0] = FPSCount;
					UPSgraphPoints[0] = UPSCount;

					for (int i = MAX_GRAPH_POINTS - 1; i > 0; i--) {
						FPSgraphPoints[i] = FPSgraphPoints[i - 1];
						UPSgraphPoints[i] = UPSgraphPoints[i - 1];
					}

					startTime = glfwGetTime();
					UPSCount = 0;
					FPSCount = 0;
				}

				update();
				render();
			}
		}

		void update() {
			int count = 0;
			float currentFrame = glfwGetTime();
			float deltaTime = currentFrame - previousUpdateTime;
			previousUpdateTime = currentFrame;

			if (deltaTime > targetUPSTime * (TARGET_UPS * 0.1f))
				deltaTime = targetUPSTime * (TARGET_UPS * 0.1f);

			accumulator += deltaTime;

			while (accumulator > targetUPSTime) {
				count++;
				accumulator -= targetUPSTime;
			}

			alphaInterpolator = accumulator / targetUPSTime;

			for (int i = 0; i < count; i++) {
				float previous = glfwGetTime();
				
				camera.update();
				GSM.update(targetUPSTime);
				glfwPollEvents();
			}

			UPSCount += count;
		}

		void renderImGUI() {
			ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(300, 330), ImGuiCond_Once);
			ImGui::Begin("Performance", p_open, window_flags);

			ImGui::Text("FPS:   %d", lastFPS);
			ImGui::PlotLines("", FPSgraphPoints, IM_ARRAYSIZE(FPSgraphPoints),0,0,0,200, ImVec2(300,60));

			ImGui::Text("UPS:   %d", lastUPS);
			ImGui::PlotLines("", UPSgraphPoints, IM_ARRAYSIZE(UPSgraphPoints), 0, 0, 0, 200, ImVec2(300, 60));
			
			ImGui::Text("Scenes");
			ImGui::ListBox("", &currentScene, scenes, IM_ARRAYSIZE(scenes), IM_ARRAYSIZE(scenes));
			GSM.changeStateTo(currentScene);

			ImGui::End();
		}

		void renderAxis() {
			std::string currentShader = "monocolor";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::mat4(1);
			model = glm::translate(model, glm::vec3(1.8,-1,0));
			model = glm::scale(model, { 0.01, 0.01, 0.01 });
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0,0,1));

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);
			ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", 0.3f, 0.3f, 0.3f,1);

			for (Mesh m : ResourceManager::getSelf()->getModel("xyzaxis").meshes) {
				m.render(currentShader);
			}
		}

		void render() {
			glViewport(0, 0, WIDTH, HEIGHT);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			previousRenderTime = glfwGetTime();
			
			GSM.render();
			renderAxis();
			renderImGUI();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			FPSCount++;
			glfwSwapBuffers(window);
		}
};

