#pragma once
#include "GameStateManager.h"
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
		GLint WIDTH = 864, HEIGHT = 486;
		GLFWwindow *window;

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

		void update() {
			if (glfwGetTime() - previousUpdateTime > targetUPSTime) {
				previousUpdateTime = glfwGetTime();
				GSM.update();
				UPSCount++;
			}
		}

		void render() {
			previousRenderTime = glfwGetTime();
			GSM.render();
			FPSCount++;
		}
};

