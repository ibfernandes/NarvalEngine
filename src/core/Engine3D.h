#pragma once
#include "core/ResourceManager.h"
#include "core/SceneManager.h"
#include "core/Camera.h"
#include "io/InputManager.h"
#include "core/Renderer.h"
#include "defines.h"
#include "materials/Texture.h"
//#include "opengl/FBO.h"
//#include "VolumeScene.h"
//#include "ModelScene.h"
#include "SceneEditor.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "utils/IconsFontAwesome5.h"
#include "utils/ColorPalette.h"
#include <stdlib.h>
#include <time.h> 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace narvalengine {
	class Engine3D {
	public:
		SceneManager sm;
		Engine3D();
		~Engine3D();
		double startTime;
		double const targetUPSTime = 1.0 / TARGET_UPS;
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
		//GLint WIDTH = 1280, HEIGHT = 720;
		GLint WIDTH = 2560, HEIGHT = 1440;
		//GLint WIDTH = 3840, HEIGHT = 2160;
		//Uses 1920x1080 as base, i.e 1.0f
		const float resolutionMultiplier = (WIDTH / float(HEIGHT)) * WIDTH / 1920.0f;
		GLFWwindow *window;

		RendererGL renderer;
		Camera camera;
		glm::mat4 staticCam;

		bool showMainMenu = true;
		bool *p_open = &showMainMenu;
		ImGuiWindowFlags window_flags = 0;
		glm::mat4 proj;
		glm::mat4 model = glm::mat4(1);

		int currentScene = 0;
		const char *scenes[1] = { "Scene editor" };
		ImFont *robotoFont, *sourceSansFont;

		ImVec4 windowBg = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
		ImVec4 textColor = ImVec4(0.0f, 0.03f, 0.12f, 1.00f);

		void initInputManager();
		void initCamera();
		void initImgui();
		void init();
		int startGLFW();
		void mainLoop();
		void update();
		void renderAxis();
		void render();

		ImVec4 add(ImVec4 vec4, float v) {
			return ImVec4(vec4.x + v, vec4.y + v, vec4.y + v, vec4.z + v);
		}
	};
}