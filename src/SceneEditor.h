#pragma once
#include "core/Scene.h"
#include "core/OfflineEngine.h"
#include "io/SceneReader.h"
#include "core/Camera.h"
#include "materials/Texture.h"
#include "core/Renderer.h"
#include "renderer/RendererGL.h"
#include "imgui.h"
#include "utils/ImGuiExt.h"
#include "utils/IconsFontAwesome5.h"
#include "utils/ColorPalette.h"
#include "addons/ImGuizmo/ImGuizmo.h" 
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "core/Camera.h"

namespace narvalengine {
	class SceneEditor : public Scene {
	public:
		GLint WIDTH, HEIGHT;
		float aspectRatio;
		std::string selectedFilePath = "";
		SceneReader sceneReader;
		Camera camera;

		OfflineEngine *offEngine;
		int renderedTiles = 0;

		float nearPlane = 1;
		float farPlane = 60000;
		float projAngle = 45;
		glm::mat4 proj;
		glm::mat4 staticCam;
		glm::mat4 cam;
		glm::mat4 orthoProj;
		glm::mat4 model = glm::mat4(1);
		RendererContext renderCtx;
		RendererGL *renderer;
		bool doneRendering = false;
		glm::vec2 renderSize;

		//FBO fbo;
		Texture *fboTex;
		TextureHandler fboTexHandler;

		ModelHandler quadModelHandler;
		ProgramHandler simpleTexProgramH;
		UniformHandler uniforms[10];
		int texPos = 0;

		//Compose Shader
		ProgramHandler composeTexProgramH;
		UniformHandler composeTexUniforms[7];
		int tex0 = 0, tex1 = 1, tex2 = 2, tex3 = 3;

		//GUI
		bool* p_open = false;
		ImGuiWindowFlags window_flags = 0;
		ImVec2 rightColumnMenuSize;
		ImVec2 menuBarSize = ImVec2(0, 0);
		ImVec2 columnOffset = ImVec2(10, 10);

		glm::vec2 sceneFrameSize = glm::vec2(0, 0);

		int currentRenderingMode = REALTIME_RENDERING_MODE;
		bool renderAPIinit = false;

		//REAL TIME
		const char* models[6] = { "cloud", "dragonHavard", "fireball", "explosion", "bunny_cloud", "explosion" };
		std::map<StringID, ModelHandler> rmToRenderAPI;
		FrameBufferHandler frameBuffers[4];
		TextureHandler renderFrameTex[3];
		TextureHandler renderFrameDepthTex[3];
		FrameBufferHandler fbRenderFrame[3];
		int numberOfActiveLights = 1;

		//REAL TIME - Phong
		ProgramHandler phongProgramHandler;
		UniformHandler phongUniforms[100];
		int phongDiffTex = 0;
		int phongSpecTex = 1;
		int phongNormTex = 2;
		int phongShadowMapTex = 3;
		int phongRenderingMode = 0;
		int normalMapping = 1;
		glm::mat4 lightProjection = glm::mat4(1);
		glm::mat4 lightView = glm::mat4(1);

		//REAL TIME - Volume
		ProgramHandler volProgramHandler;
		UniformHandler volUniforms[100];
		glm::vec3 cameraPosition = glm::vec3(0,0,0);
		glm::vec3 cameraPositionCache = cameraPosition;
		int volumeTexBind = 0;
		int bgTexBind = 1;
		int bgDepthTexBind = 2;
		int prevFrameTexBind = 3;
		float time = 0;
		int volRenderingMode = 0;

		glm::vec3 scattering = glm::vec3(0.2f, 0.2f, 0.2f);
		glm::vec3 absorption = glm::vec3(0.01f);
		float g = 0.9;
		int SPP = 1;
		float densityCoef = 1;
		float numberOfSteps = 64;
		float shadowSteps = 10;
		glm::vec3 lightPosition = glm::vec3(0, 4, 0);
		glm::vec3 lightColor = glm::vec3(1.0, 1.0, 1.0);
		float ambientStrength = 300;
		float Kc = 1.0, Kl = 0.7, Kq = 1.8;
		float frameCount = 0;
		float enableShadow = 1;

		//Current Selected Object
		InstancedModel *currentSelectedIM = nullptr;
		glm::vec3 selectObjPos = glm::vec3(0);
		glm::vec3 selectObjScale = glm::vec3(0);
		glm::vec3 selectObjRotation = glm::vec3(0);
		ImGuizmo::OPERATION currentGuizmoOp = ImGuizmo::TRANSLATE;

		//Selected Object Material is Volumetric
		int currentRenderMode = 0;
		int phaseFunctionOption = 0;
		const char* renderModes[3] = { "Ray marching grid", "Path tracing MC", "LBVH" };
		const char* phaseFunctionOptions[3] = { "Isotropic", "Rayleigh", "Henyey-Greenstein" };

		//Mono Color Shader
		ProgramHandler monoColorProgramH;
		UniformHandler monoColorUniforms[4];
		glm::vec4 outlineObjColor = glm::vec4(1, 1, 0, 1);

		//Performance
		double startTime;
		static const int MAX_GRAPH_POINTS = 100;
		float FPSgraphPoints[MAX_GRAPH_POINTS];
		float UPSgraphPoints[MAX_GRAPH_POINTS];
		int FPSCount = 0;
		int lastFPS = 0;

		//Scene Instanced Models list
		ImVec4 listHoverColor = ImVec4(66/255.0f, 77/255.0f, 194/255.0f, 1.0f);
		ImVec4 darkBlue = ImVec4(12/255.0f, 45/255.0f, 72/255.0f, 1.0f);
		ImVec4 yellowvivid = ImVec4(215/255.0f, 207/255.0f, 7/255.0f, 1.0f);
		char* imNames[100] = { "Cube", "Plane", "Hexa" };
		char* imIcons[100] = { "ICONCube", "ICONPlane", "ICONHexa" };
		int imNamesCurrentLength = 3;
		int currentIMListIndex = 0;
		int selectedImList = -1;

		SceneEditor();

		void init(GLint width, GLint height, RendererGL* r, Camera* c);
		void load();
		bool sceneChanged();
		void updateUI();
		void update(float deltaTime);
		void variableUpdate(float deltaTime);
		void startOffEngine();
		void renderImGUI();
		void generateSceneImList();
		void sceneListImGUI();
		void selectedModelImGUI();
		void selectedMaterialImGUI();
		void cameraImGUI();
		void lightImGui();
		void performanceImGUI();
		void renderOptionsImGUI();
		void renderOffline();
		void renderRealTime();
		void render();
		void reloadScene();
		void initVolumetricShader();
		void initPhongShader();
		void initComposeShader();
		void initMonoColorShader();
		void initRenderAPI();
		void hoverObject();
	};
}

