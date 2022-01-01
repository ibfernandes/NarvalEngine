#pragma once
#include "core/ResourceManager.h"
#include "core/StateManager.h"
#include "core/Camera.h"
#include "io/InputManager.h"
#include "core/Renderer.h"
#include "renderer/RendererGL.h"
#include "defines.h"
#include "materials/Texture.h"
#include "SceneEditor.h"
#include "imgui.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "utils/IconsFontAwesome5.h"
#include "utils/ColorPalette.h"
#include <stdlib.h>
#include <time.h> 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glog/logging.h>
#include "optick.h"


namespace narvalengine {
	/**
	 * Singleton responsible for initializing all the application context such as graphics, inputs, physics etc.
	 */
	class Engine3D {
	private:
		double startTime;
		//Time interval in seconds between each update() call.
		const double targetUPSTime = 1.0 / TARGET_UPS;
		double previousUpdateTime;
		double previousRenderTime;
		float accumulator = 0;
		float alphaInterpolator = 1;
		int UPSCount = 0;
		int FPSCount = 0;
		int lastFPS = 0;
		int lastUPS = 0;
		static Engine3D* self;
		Engine3D();
		void initInputManager();
		void initImgui();
		int initGLFW();
		/**
		 * Called before render() every {@code targetUPSTime} seconds.
		 */
		void update();
		/**
		 * Called every time before render().
		 */
		void variableUpdate();
		/**
		 * Called every time at the end of each loop iteration.
		 */
		void render();
	public:
		StateManager stateManager;
		Settings settings;
		~Engine3D();

		//Uses 1920x1080 as base resolution for the multiplier.
		float resolutionMultiplier;
		GLFWwindow *window;
		RendererContext renderer;

		ImGuiWindowFlags window_flags = 0;
		ImFont *robotoFont, *sourceSansFont;
		ImVec4 windowBg = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);

		static Engine3D* getSelf();
		void init(Settings settings, EngineState *state);
		void mainLoop();
	};
}