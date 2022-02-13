#include "core/Engine3D.h"

namespace narvalengine {
	Engine3D* Engine3D::self = nullptr;

	void glogCustomPrefix(std::ostream& s, const google::LogMessageInfo& l, void*) {
		#ifdef NE_DEBUG_MODE
			s << "[ "
			<< l.severity[0]
			<< ' '
			<< l.filename << ':' << l.line_number << "]";
		#else
			s << l.severity[0]
			<< std::setw(4) << 1900 + l.time.year()
			<< std::setw(2) << 1 + l.time.month()
			<< std::setw(2) << l.time.day()
			<< ' '
			<< std::setw(2) << l.time.hour() << ':'
			<< std::setw(2) << l.time.min() << ':'
			<< std::setw(2) << l.time.sec() << "."
			<< std::setw(6) << l.time.usec()
			<< ' '
			<< std::setfill(' ') << std::setw(5)
			<< l.thread_id << std::setfill('0')
			<< ' ]';
		#endif
	}

	Engine3D* Engine3D::getSelf() {
		if (self == nullptr) {
			FLAGS_log_dir = "./";
			#ifdef NE_DEBUG_MODE
				FLAGS_alsologtostderr = 1;
			#else
				FLAGS_minloglevel = 2;
			#endif
			google::InitGoogleLogging("", &glogCustomPrefix);
			self = new Engine3D();
		}

		return self;
	}

	void Engine3D::initInputManager() {
		glfwSetKeyCallback(window, InputManager::key_callback_handler);
		glfwSetCursorPosCallback(window, InputManager::mouse_pos_callback_handler);
		glfwSetMouseButtonCallback(window, InputManager::mouse_key_callback_handler);
		InputManager::getSelf();
	}

	void Engine3D::initImgui() {

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImNodes::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init();

		// Setup style
		ImGui::StyleColorsDark();
		ImGuiStyle * style = &ImGui::GetStyle();

		style->WindowPadding = imGuiStyleDefs.windowPadding;
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

		style->Colors[ImGuiCol_MenuBarBg] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_WindowBg] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_ChildBg] = gUIPalette.iceGrey[1];
		style->Colors[ImGuiCol_PopupBg] = gUIPalette.iceGrey[1];
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

		style->Colors[ImGuiCol_ResizeGrip] = ImGuiExt::add(windowBg, -0.15f);
		style->Colors[ImGuiCol_ResizeGripHovered] = ImGuiExt::add(windowBg, -0.25f);
		style->Colors[ImGuiCol_ResizeGripActive] = ImGuiExt::add(windowBg, -0.6f);

		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.0f);
		style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style->Colors[ImGuiCol_CheckMark] = gUIPalette.text;
		style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Separator] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

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
	
		ImNodesStyle* nodesStyle = &ImNodes::GetStyle();
		nodesStyle->Colors[ImNodesCol_NodeBackground] = colorToInt32(gUIPalette.iceGrey[2]);
		nodesStyle->Colors[ImNodesCol_NodeBackgroundHovered] = colorToInt32(gUIPalette.iceGrey[3]);
		nodesStyle->Colors[ImNodesCol_NodeBackgroundSelected] = colorToInt32(gUIPalette.iceGrey[4]);
		nodesStyle->Colors[ImNodesCol_NodeOutline] = colorToInt32(gUIPalette.iceGrey[5]);
		nodesStyle->Colors[ImNodesCol_TitleBar] = colorToInt32(gUIPalette.iceGrey[0]);
		nodesStyle->Colors[ImNodesCol_TitleBarHovered] = colorToInt32(gUIPalette.iceGrey[1]);
		nodesStyle->Colors[ImNodesCol_TitleBarSelected] = colorToInt32(gUIPalette.iceGrey[2]);
		nodesStyle->Colors[ImNodesCol_Link] = colorToInt32(gUIPalette.blue[2]);
		nodesStyle->Colors[ImNodesCol_LinkHovered] = colorToInt32(gUIPalette.blue[1]);
		nodesStyle->Colors[ImNodesCol_LinkSelected] = colorToInt32(gUIPalette.blue[2]);
		nodesStyle->Colors[ImNodesCol_Pin] = colorToInt32(gUIPalette.blue[0]);
		nodesStyle->Colors[ImNodesCol_PinHovered] = colorToInt32(gUIPalette.blue[2]);
		ImVec4 boxSelector = gUIPalette.blueGrey[0];
		boxSelector.w = 0.2;
		nodesStyle->Colors[ImNodesCol_BoxSelector] = colorToInt32(boxSelector);
		boxSelector.w = 0.4;
		nodesStyle->Colors[ImNodesCol_BoxSelectorOutline] = colorToInt32(boxSelector);
		nodesStyle->Colors[ImNodesCol_GridBackground] = colorToInt32(gUIPalette.iceGrey[5]);
		nodesStyle->Colors[ImNodesCol_GridLine] = colorToInt32(gUIPalette.iceGrey[4]);
	}

	void Engine3D::init(Settings settings, EngineState *state) {
		this->settings = settings;
		this->startTime = glfwGetTime();
		this->previousUpdateTime = startTime;
		resolutionMultiplier = (settings.resolution.x / float(settings.resolution.y)) * settings.resolution.x / 1920.0f;

		initGLFW();
		if (settings.api == RendererAPIName::OPENGL) {
			RendererGL *rendererGL = new RendererGL();
			rendererGL->init();
			renderer.setRenderer(rendererGL);
		}

		initInputManager();

		stateManager.addState(state);
		stateManager.changeStateTo(0);

		initImgui();
	}

	int Engine3D::initGLFW() {
		//Command line only. No user interface.
		if (settings.api == RendererAPIName::CPU)
			return 1;

		glfwInit();

		if (settings.api == RendererAPIName::OPENGL) {
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, settings.apiVersionMajor);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.apiVersionMinor);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
			glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		}

		window = glfwCreateWindow(settings.resolution.x, settings.resolution.y, "Narval Engine - alpha", nullptr, nullptr);
		if (window == nullptr) {
			LOG(FATAL) << "Failed to create GLFW window.";
			glfwTerminate();
			return -1;
		}

		glfwMakeContextCurrent(window);

		if (settings.api == RendererAPIName::OPENGL) {
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				LOG(FATAL) << "Failed to initialize GLAD.";
				return -1;
			}
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		if (settings.api == RendererAPIName::OPENGL) {
			glViewport(0, 0, width, height);
			//Vsync is disabled by default.
			glfwSwapInterval(false);
		}
	}

	void Engine3D::mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			OPTICK_FRAME("MainThread");

			if (glfwGetTime() - startTime >= 1.0) {
				lastFPS = FPSCount;
				lastUPS = UPSCount;

				startTime = glfwGetTime();
				UPSCount = 0;
				FPSCount = 0;
			}

			update();
			stateManager.variableUpdate(0);
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

			stateManager.update(targetUPSTime);
			glfwPollEvents();
		}

		UPSCount += count;
	}

	void Engine3D::render() {
		if (settings.api == RendererAPIName::OPENGL) {
			glViewport(0, 0, settings.resolution.x, settings.resolution.y);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			previousRenderTime = glfwGetTime();

			stateManager.render();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		
		FPSCount++;
		glfwSwapBuffers(window);
	}

	Engine3D::~Engine3D() {
		delete window;
		delete robotoFont;
		delete sourceSansFont;
	}

	Engine3D::Engine3D() {
	}
}
