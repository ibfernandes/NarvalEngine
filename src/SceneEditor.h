#pragma once
#include "core/Scene.h"
#include "core/Engine3D.h"
#include "core/EngineState.h"
#include "core/OfflineEngine.h"
#include "io/SceneReader.h"
#include "core/Camera.h"
#include "materials/Texture.h"
#include "core/Renderer.h"
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
#include "VMS.h"
#include <sstream>

namespace narvalengine {

	enum RenderingMode {
		OFFLINE_RENDERING_MODE,
		REALTIME_RENDERING_MODE
	};

	/**
	 * A complete SceneEditor with User Interface.
	 */
	class SceneEditor : public EngineState {
	private:
		bool sceneChanged();
		void updateUI();
		void startOffEngine();
		void renderImGUI();
		int getSelectedIMListPos(InstancedModel* im);
		InstancedModel* getSelectedIMfromListPos(int pos);
		void generateSceneImList();
		void sceneListImGUI();
		void selectedModelImGUI();
		void selectedMaterialImGUI();
		void cameraImGUI();
		void lightImGui();
		void performanceImGUI();
		void toneMappingImGUI();
		void renderOptionsImGUI();
		void shootRay(int i);
		void shootMultipleRays(int qtt);
		void shootRayImGUI();
		void shadowMappingImGUI();
		void newVolRenderImGUI();
		void renderShadowMappingDebug();
		void renderOffline();
		void renderRealTimeShadows();
		void renderScene(FrameBufferHandler* fbh, TextureHandler* fbhTex, int currentFrame);
		void renderPostProcessing();
		void renderRealTimePBR();
		void reloadScene(bool absolutePath);
		void initVolumetricShader();
		void initPBR();
		void initPostProcessingShader();
		void initComposeShader();
		void initMonoColorShader();
		void initShadowShader();
		void initImageDifferenceShader();
		void initRenderAPI();
		void hoverObject();
		void sortAndGroup(Scene *scene);
		void generateModelHandlers();
		void deleteModelHandlers();
	public:
		GLint WIDTH, HEIGHT;
		float aspectRatio;
		std::string selectedFilePath = "";
		SceneReader sceneReader;
		SceneSettings settings;
		Scene *scene;
		Camera camera;

		OfflineEngine *offEngine;
		int renderedTiles = 0;

		float nearPlane = 0.1;
		float farPlane = 60000;
		float projAngle = 45;
		glm::mat4 proj;
		glm::mat4 invProj;
		glm::mat4 staticCam;
		glm::mat4 cam;
		glm::mat4 orthoProj;
		glm::mat4 model = glm::mat4(1);
		glm::mat4 identity = glm::mat4(1);
		RendererContext *renderCtx;
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
		//Maps the resources from the ResourceManager to the Graphic API.
		std::map<StringID, ModelHandler> rmToRenderAPI;
		FrameBufferHandler frameBuffers[4]; //TODO not being used(?)
		TextureHandler renderFrameTex[4];
		TextureHandler renderFrameDepthTex[4];
		TextureHandler infAreaLightTex;
		FrameBufferHandler fbRenderFrame[4];
		int numberOfActiveLights = 1;

		//REAL TIME - SHADOWS
		ProgramHandler shadowProgramHandler;
		UniformHandler shadowUniforms[10];
		//TextureHandler shadowfboTex;
		TextureHandler shadowDepthTex;
		FrameBufferHandler shadowfboh;
		glm::mat4 lightView[10] = { glm::mat4(1) , glm::mat4(1) }; //one for each light source
		glm::mat4 lightProjection = glm::mat4(1);

		//REAL TIME - PBR
		ProgramHandler pbrProgramHandler;
		static const int maxPBRUniforms = 100;
		UniformHandler pbrUniforms[maxPBRUniforms];
		UniformHandler pbrLightUniforms[maxPBRUniforms];
		glm::vec3 lightPos[10];
		bool isMaterialSet[6]{};
		int texIds[7] = { 0, 1, 2, 3, 4, 5, 7 };
		//Light is made of .position and .color, thus the offset is 2.
		int pbrLightsOffset = 2;
		TextureHandler lightTexColorH;
		glm::vec4 defaultLightColor = glm::vec4(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 1);

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
		int infAreaLightTexBind = 4;
		float time = 0;
		int currentFrame = 1;
		float densityMc = 1;
		int maxBounces = 1;
		int useFactor = 1;

		glm::vec3 scattering = glm::vec3(1.1f, 1.1f, 1.1f);
		glm::vec3 absorption = glm::vec3(0.01f);
		float g = 0.0;
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
		int volLightType = 0;


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
		const char* renderModes[3] = { "Ray marching", "Lobe Sampling", "Monte Carlo" };
		const char* phaseFunctionOptions[1] = { "Henyey-Greenstein" };

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
		Timer realTimeTimer;

		//Scene Instanced Models list
		ImVec4 listHoverColor = ImVec4(66/255.0f, 77/255.0f, 194/255.0f, 1.0f);
		ImVec4 darkBlue = ImVec4(12/255.0f, 45/255.0f, 72/255.0f, 1.0f);
		ImVec4 yellowvivid = ImVec4(215/255.0f, 207/255.0f, 7/255.0f, 1.0f);
		std::string imNames[100] = { "Cube", "Plane", "Hexa" };
		std::string imIcons[100] = { "ICONCube", "ICONPlane", "ICONHexa" };
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
		bool renderShadowMapping = false;

		//Log window
		std::string logText = "> Log begin \n";
		ImVec2 logWindowSize;
		ImVec2 logWindowPos;
		bool logWindowOpen = true;
		bool logState[2] = { true, true };

		//CompareTexture
		// 0 = compare to offline pah tracing, 1 = compare to another texture
		int compareMode = 1;
		StringID texIDToCompare1 = -1;
		StringID texIDToCompare2 = -1;
		TextureHandler compareTex1;
		TextureHandler compareTex2;

		//new Render test params
		float newStepSize = 0.1;
		int newNumberOfSteps = 100;
		int newnPointsToSample = 10;
		int newMeanPathMult = 10;
		float param_1 = 1;

		//Volumetric Method Shaders settings
		VMS *vms;

		//Tone Mapping
		ProgramHandler postProcessingPH;
		UniformHandler postProcessingUni[10];
		float gamma = 2.2;
		float exposure = 0.3;

		glm::vec4 defaultTexColor = glm::vec4(0 / 255.0f, -0 / 255.0f, 0 / 255.0f, 1);
		TextureHandler defaultTex;

		SceneEditor();
		void init();
		void load();
		void render();
		void update(float deltaTime);
		void variableUpdate(float deltaTime);
	};
}

