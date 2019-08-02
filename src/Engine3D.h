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

		glm::vec3 absorption = 0.0f * glm::vec3(0.001f);
		glm::vec3 scattering = glm::vec3(0.25, 0.5, 1.0);
		glm::vec3 lightPosition = glm::vec3(0.5,1.0,5.0);
		glm::vec3 lightColor =  glm::vec3(1.0, 1.0, 1.0);
		glm::vec3 materialColor =  glm::vec3(1.0, 1.0, 1.0);

		float densityCoef =  0.75;
		float powderCoef =  1.3;
		float HGCoef = 0.99;

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
			ResourceManager::getSelf()->getShader(currentShader).setFloat("powderCoef", powderCoef);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("HGCoef", HGCoef);
		}

		void renderImGUI() {
			// Show GUI
			// Start the ImGui frame (For Each new frame)
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::SetNextWindowPos(ImVec2(600, 4), ImGuiCond_Once); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
			ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);
			ImGui::Begin("Volumetric Menu", p_open, window_flags);
			ImGui::DragFloat3("Absorption", &absorption[0], 0.01, 0.0, 30.0);
			ImGui::DragFloat3("Scaterring", &scattering[0], 0.01, 0.0, 30.0);
			ImGui::DragFloat3("Light Pos", &lightPosition[0], 0.01, -30.0, 30.0);
			ImGui::DragFloat3("Light Lumminance", &lightColor[0], 0.01, 0.0, 30.0);
			ImGui::DragFloat3("Material Color", &materialColor[0], 0.01, 0.0, 1.0);

			ImGui::DragFloat("densityCoef", &densityCoef, 0.01, 0.0, 2.0);
			ImGui::DragFloat("powderCoef", &powderCoef, 0.01, 0.0, 2.0);
			ImGui::DragFloat("HGCoef", &HGCoef, 0.01, 0.0, 2.0);
			ImGui::End();
			
			// Imgui Render Calls (For Each end frame)
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		void render2DVolume() {
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
			model = glm::scale(model, { 0.2, 0.2, 0.2 });

			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", 1,0,0,1);

			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));


			currentShader = "volume";
			ResourceManager::getSelf()->getShader(currentShader).use();
			model = glm::mat4(1);
			model = glm::translate(model, { 0, 0, 4 });
			model = glm::scale(model, { 1, 1, 1 });

			ResourceManager::getSelf()->getShader(currentShader).setInteger("volume", 0);
			glActiveTexture(GL_TEXTURE0);
			cloudSystem.perlinWorley3.bind();

			float time = ((sin(glm::radians(glfwGetTime() * 100)) + 1) / 2) * 1;
			ResourceManager::getSelf()->getShader(currentShader).setFloat("time", time);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *camera.getCam());
			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
			glm::vec3 camPos = *(camera.getPosition());
			ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", camPos.x, camPos.y, camPos.z);

			renderer.render(ResourceManager::getSelf()->getModel("cubeTest"));

		}

		void render() {
			glViewport(0, 0, WIDTH, HEIGHT);
			glClearColor(0, 1, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

