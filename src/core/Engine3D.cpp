#include "core/Engine3D.h"

namespace narvalengine {
	void APIENTRY
		MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {

		if (type == GL_DEBUG_TYPE_ERROR) {
			fprintf(stderr, "message = %s, GL CALLBACK: %s type = 0x%x, severity = 0x%x\n", 
				message,
				(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
				type, 
				severity);
		}
	}

	Engine3D::Engine3D() {
		this->startTime = glfwGetTime();
		this->previousUpdateTime = startTime;
	}

	void Engine3D::initInputManager() {
		glfwSetKeyCallback(window, InputManager::key_callback_handler);
		glfwSetCursorPosCallback(window, InputManager::mouse_pos_callback_handler);
		glfwSetMouseButtonCallback(window, InputManager::mouse_key_callback_handler);
		InputManager::getSelf();
	}

	void Engine3D::initCamera() {
		glm::vec3 position(0, 0, -3);
		camera.setPosition(position);
		glm::vec3 front = { 0.0f, 0.0f, 1.0f };
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 side = glm::cross(front, up);
		staticCam = glm::lookAt(position, position + front, up);
	}

	void Engine3D::initImgui() {

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
		style->FramePadding = ImVec2(5 * resolutionMultiplier, 5 * resolutionMultiplier);
		style->FrameRounding = 4.0f;
		style->ItemSpacing = ImVec2(12, 8);
		style->ItemInnerSpacing = ImVec2(8, 6);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 15.0f;
		style->ScrollbarRounding = 9.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;
		style->WindowMenuButtonPosition = ImGuiDir_Right;

		style->Colors[ImGuiCol_Text] = gUIPalette.text;

		style->Colors[ImGuiCol_MenuBarBg] = gUIPalette.iceGrey[0];
		style->Colors[ImGuiCol_WindowBg] = gUIPalette.iceGrey[0];
		style->Colors[ImGuiCol_ChildWindowBg] = gUIPalette.iceGrey[0];
		style->Colors[ImGuiCol_TitleBg] = gUIPalette.iceGrey[0];
		style->Colors[ImGuiCol_TitleBgActive] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_TitleBgCollapsed] = gUIPalette.iceGrey[2];

		style->Colors[ImGuiCol_Header] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_HeaderHovered] = gUIPalette.iceGrey[3];
		style->Colors[ImGuiCol_HeaderActive] = gUIPalette.iceGrey[4];

		style->Colors[ImGuiCol_FrameBg] = gUIPalette.iceGrey[0];
		style->Colors[ImGuiCol_FrameBgHovered] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_FrameBgActive] = gUIPalette.iceGrey[2];

		style->Colors[ImGuiCol_ScrollbarBg] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_ScrollbarGrab] = gUIPalette.iceGrey[2];
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = gUIPalette.iceGrey[3];
		style->Colors[ImGuiCol_ScrollbarGrabActive] = gUIPalette.iceGrey[4];

		style->Colors[ImGuiCol_Button] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_ButtonHovered] = gUIPalette.iceGrey[2];
		style->Colors[ImGuiCol_ButtonActive] = gUIPalette.iceGrey[3];

		style->Colors[ImGuiCol_Separator] = gUIPalette.iceGrey[2];
		style->Colors[ImGuiCol_SeparatorHovered] = gUIPalette.iceGrey[3];
		style->Colors[ImGuiCol_SeparatorActive] = gUIPalette.iceGrey[4];

		style->Colors[ImGuiCol_ResizeGrip] = add(windowBg, -0.15f);
		style->Colors[ImGuiCol_ResizeGripHovered] = add(windowBg, -0.25f);
		style->Colors[ImGuiCol_ResizeGripActive] = add(windowBg, -0.6f);

		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_PopupBg] = gUIPalette.iceGrey[0];
		style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.0f);
		style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style->Colors[ImGuiCol_CheckMark] = textColor;
		style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

		style->GrabMinSize = 18;

		style->WindowRounding = 0;
		style->ChildRounding = 0;
		style->FrameRounding = 0;
		style->PopupRounding = 0;
		style->ScrollbarRounding = 0;
		style->GrabRounding = 0;
		style->TabRounding = 0;

		std::string path = RESOURCES_DIR "fonts/roboto/Roboto-Regular.ttf";
		//robotoFont = io.Fonts->AddFontFromFileTTF(path.c_str(), 14);

		path = RESOURCES_DIR "fonts/sourcesans/SourceSansPro-SemiBold.ttf";
		sourceSansFont = io.Fonts->AddFontFromFileTTF(path.c_str(), 18 * resolutionMultiplier);
		io.FontDefault = sourceSansFont;

		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;

		path = RESOURCES_DIR "fonts/fontawesome/" FONT_ICON_FILE_NAME_FAS;
		io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f * resolutionMultiplier, &icons_config, icons_ranges);

		path = RESOURCES_DIR "fonts/fontawesome/" FONT_ICON_FILE_NAME_FAR;
		io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f * resolutionMultiplier, &icons_config, icons_ranges);
	}

	void Engine3D::init() {
		startGLFW();
		initInputManager();
		initCamera();

		glEnable(GL_TEXTURE_2D);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(MessageCallback, 0);


		float nearPlane = 1;
		float farPlane = 60000;
		float projAngle = 45;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)WIDTH / (GLfloat)HEIGHT, nearPlane, farPlane);

		/*VolumeScene *vs = new VolumeScene;
		vs->init(WIDTH, HEIGHT, &renderer, &camera);
		GSM.addState(vs);

		ModelScene *mods = new ModelScene;
		mods->init(WIDTH, HEIGHT, &renderer, &camera);
		GSM.addState(mods);*/

		SceneEditor* se = new SceneEditor;
		se->init(WIDTH, HEIGHT, &renderer, &camera);
		sm.addState(se);
		sm.currentScene = currentScene;

		initImgui();
	}

	int Engine3D::startGLFW() {
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_CONTEXT_VERSION_MAJOR);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_CONTEXT_VERSION_MINOR);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Narval Engine - alpha", nullptr, nullptr);
		if (window == nullptr) {
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

	void Engine3D::mainLoop() {
		sm.changeStateTo(currentScene);

		while (!glfwWindowShouldClose(window)) {

			if (glfwGetTime() - startTime >= 1.0) {
				lastFPS = FPSCount;
				lastUPS = UPSCount;

				startTime = glfwGetTime();
				UPSCount = 0;
				FPSCount = 0;
			}

			update();
			render();
		}
	}

	void Engine3D::update() {
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

			//camera.update(); //TODO: Why it was here
			sm.update(targetUPSTime);
			glfwPollEvents();
		}

		UPSCount += count;
	}

	void Engine3D::renderAxis() {
		/*std::string currentShader = "monocolor";
		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(1.8, -1, 0));
		model = glm::scale(model, { 0.01, 0.01, 0.01 });
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 0, 1));

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);
		ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", 0.3f, 0.3f, 0.3f, 1);

		for (Mesh m : ResourceManager::getSelf()->getModel("xyzaxis")->meshes) {
			m.render(currentShader);
		}*/
	}

	void Engine3D::render() {
		glViewport(0, 0, WIDTH, HEIGHT);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		previousRenderTime = glfwGetTime();

		sm.render();
		renderAxis();
		//renderImGUI();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		FPSCount++;
		glfwSwapBuffers(window);
	}

	Engine3D::~Engine3D() {
	}
}
