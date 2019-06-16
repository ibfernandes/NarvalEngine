#pragma once
#include "ResourceManager.h"
#include "GameStateManager.h"
#include "FastNoise.h"
#include "Renderer.h"
#include "CloudSystem.h"
#include "Texture3D.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>


class Engine3D{
	public:
		GameStateManager GSM;
		Engine3D();
		~Engine3D();
		double startTime;
		int const targetUPS = 60;
		double const targetUPSTime = 1.0/targetUPS;
		double previousUpdateTime;
		double previousRenderTime;
		int UPSCount = 0;
		int FPSCount = 0;

		int const GL_CONTEXT_VERSION_MAJOR = 4;
		int const GL_CONTEXT_VERSION_MINOR = 0;
		int const IS_VSYNC_ON = 0;
		GLint WIDTH = 256, HEIGHT = 256;
		GLFWwindow *window;

		Renderer renderer;
		std::string currentShader = "cloudscape";
		CloudSystem cloudSystem;
		

		void init() {
			startGLFW();
			cloudSystem.generateCloudNoiseTextures();

			glEnable(GL_TEXTURE_2D);
			glFrontFace(GL_CCW);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glEnable(GL_DEPTH_TEST);
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
			if (glfwGetTime() - previousUpdateTime > targetUPSTime) {
				previousUpdateTime = glfwGetTime();
				GSM.update();
				UPSCount++;
			}
			glfwPollEvents();
		}

		void render() {
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 cam(1);
			glm::mat4 model(1);
			glm::mat4 proj(1);
			model = glm::translate(model, { 10,0, 20 });
			model = glm::scale(model, { 300,300, 30 });
			float nearPlane = 1;
			float farPlane = 7000;
			float projAngle = 45;
			proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);
			glm::vec3 cameraPos = {0, 0.0f, 600 };
			glm::vec3 cameraFront = { 0.0f, 0.0f, -1.0f };
			glm::vec3 cameraUp = { 0.0f, 1.0f, 0.0f };
			cam = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

			ResourceManager::getSelf()->getShader(currentShader).use();
			//should be done only once
			ResourceManager::getSelf()->getShader(currentShader).setInteger("perlinWorley", 0);
			glActiveTexture(GL_TEXTURE0);
			cloudSystem.perlinWorley3.bind();

			float time = ((sin(glm::radians(glfwGetTime()*10)) + 1) / 2);

			ResourceManager::getSelf()->getShader(currentShader).setFloat("time", time);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", cam);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
			ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
			ResourceManager::getSelf()->getShader(currentShader).setFloat("screenRes", WIDTH);
			
			renderer.render(ResourceManager::getSelf()->getModel("quadTest"));

			previousRenderTime = glfwGetTime();
			GSM.render();
			FPSCount++;
			glfwSwapBuffers(window);
		}
};

