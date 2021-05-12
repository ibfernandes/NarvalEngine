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
#include <sstream>

namespace narvalengine {
	class SceneEditor : public Scene {
	public:
		GLint WIDTH, HEIGHT;
		float aspectRatio;
		std::string selectedFilePath = "";
		SceneReader sceneReader;
		Settings settings;
		Scene scene;
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
		bool didSceneChange = false;

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

		//REAL TIME - SHADOWS
		ProgramHandler shadowProgramHandler;
		UniformHandler shadowUniforms[10];
		//TextureHandler shadowfboTex;
		TextureHandler shadowDepthTex;
		FrameBufferHandler shadowfboh;
		glm::mat4 lightView[10] = { glm::mat4(1) , glm::mat4(1) }; //one for each light source
		glm::mat4 lightProjection = glm::mat4(1);

		//REAL TIME - Phong
		ProgramHandler phongProgramHandler;
		UniformHandler phongUniforms[100];
		UniformHandler lightUniforms[100];
		glm::vec3 lightPos[10];
		int phongLightsOffset = 7;
		int phongDiffTex = 0;
		int phongSpecTex = 1;
		int phongNormTex = 2;
		int phongShadowMapTex = 3;
		int phongRenderingMode = 0;
		int normalMapping = 1;
		glm::vec4 lightTexColor = glm::vec4(252/255.0f, 186/255.0f, 3/255.0f, 1);
		TextureHandler lightTexColorH;

		//REAL TIME - PBR
		ProgramHandler pbrProgramHandler;
		UniformHandler pbrUniforms[100];
		UniformHandler pbrLightUniforms[100];
		int texIds[7] = { 0, 1, 2, 3, 4, 5, 7 };
		int pbrLightsOffset = 2;

		//REAL TIME - Volume
		ProgramHandler volProgramHandler;
		UniformHandler volUniforms[100];
		glm::vec3 cameraPosition = glm::vec3(0,0,0);
		glm::vec3 cameraPositionCache = cameraPosition;
		float invMaxDensity = 1;
		int volumeTexBind = 0;
		int bgTexBind = 1;
		int bgDepthTexBind = 2;
		int prevFrameTexBind = 3;
		float time = 0;
		int volRenderingMode = 3;
		int currentFrame = 1;
		float densityMc = 1;

		glm::vec3 scattering = glm::vec3(1.1f, 1.1f, 1.1f);
		glm::vec3 absorption = glm::vec3(0.01f);
		float g = 0.0;
		int SPP = 1;
		float densityCoef = 1;
		float numberOfSteps = 64;
		float shadowSteps = 10;
		glm::vec3 lightPosition = glm::vec3(0, 6, 0); //TODO: currently hard coded
		glm::vec3 lightColor = glm::vec3(300.0, 300.0, 300.0);
		float ambientStrength = 10;
		float Kc = 1.0, Kl = 0.7, Kq = 1.8;
		int frameCount = 0;
		float enableShadow = 1;
		glm::vec3 volLightRecSize;
		glm::vec3 volLightRecMin;
		glm::vec3 volLightRecMax;
		glm::mat4 volLightWCS;


		//Current Selected Object
		bool shouldUpdateSelectedIM = false;
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

		//Image Difference Shader
		ProgramHandler imgDiffProgramH;
		UniformHandler imgDiffUniforms[5];
		int imgDiffId0 = 0;
		int imgDiffId1 = 1;

		//Performance
		double startTime;
		static const int MAX_GRAPH_POINTS = 100;
		float FPSgraphPoints[MAX_GRAPH_POINTS];
		float UPSgraphPoints[MAX_GRAPH_POINTS];
		int FPSCount = 0;
		int lastFPS = 0;
		Timer offEngineTimer;

		//Scene Instanced Models list
		ImVec4 listHoverColor = ImVec4(66/255.0f, 77/255.0f, 194/255.0f, 1.0f);
		ImVec4 darkBlue = ImVec4(12/255.0f, 45/255.0f, 72/255.0f, 1.0f);
		ImVec4 yellowvivid = ImVec4(215/255.0f, 207/255.0f, 7/255.0f, 1.0f);
		char* imNames[100] = { "Cube", "Plane", "Hexa" };
		char* imIcons[100] = { "ICONCube", "ICONPlane", "ICONHexa" };
		int imNamesCurrentLength = 3;
		int currentIMListIndex = 0;
		int selectedImList = -1;

		//Camera
		bool shouldUpdateCamera = false;

		//Debug
		//Debug Ray shooter
		bool pointTo = true;
		glm::vec3 rayDirection = glm::vec3(0.01, 1.5, 0);
		glm::vec3 rayOrigin = glm::vec3(0, 4, -15);
		//int *rayIndices = nullptr; 
		int* rayIndices[50];
		std::vector<PointPathDebugInfo> intersectionPoints[50];
		VertexLayout rayVertexLayout;
		int numberOfShootedRays = 0;
		VertexBufferHandler rayVertexBufferH[50];
		IndexBufferHandler rayIndexBufferH[50];
		glm::vec4 lineColor = glm::vec4(1, 1, 0, 1);
		TextureHandler rayTextureColorH;
		int numberOfBounces = 0;

		//Debug shadow mapping
		bool renderShadowMapping = true;

		//Log window
		std::string logText = "> Log begin \n";
		ImVec2 logWindowSize;
		ImVec2 logWindowPos;
		bool logWindowOpen = true;
		bool logState[2] = { true, true };

		//CompareTexture
		StringID texIDToCompare = -1;
		TextureHandler compareTex;

		SceneEditor();

		void init(GLint width, GLint height, RendererGL* r, Camera* c);
		void load();
		bool sceneChanged();
		void updateUI();
		void update(float deltaTime);
		void variableUpdate(float deltaTime);
		void startOffEngine();
		void renderImGUI();
		int getSelectedIMListPos(InstancedModel* im);
		InstancedModel *getSelectedIMfromListPos(int pos);
		void generateSceneImList();
		void sceneListImGUI();
		void selectedModelImGUI();
		void selectedMaterialImGUI();
		void cameraImGUI();
		void lightImGui();
		void performanceImGUI();
		void renderOptionsImGUI();
		void shootRay(int i);
		void shootMultipleRays(int qtt);
		void shootRayImGUI();
		void shadowMappingImGUI();
		void renderShadowMappingDebug();
		void renderOffline();
		void renderRealTimeShadows();
		void renderRealTimePBR();
		void renderRealTime();
		void renderCompare();
		void render();
		void reloadScene();
		void compareScene();
		void initVolumetricShader();
		void initPhongShader();
		void initPBR();
		void initComposeShader();
		void initMonoColorShader();
		void initShadowShader();
		void initImageDifferenceShader();
		void initRenderAPI();
		void hoverObject();
	};
}

