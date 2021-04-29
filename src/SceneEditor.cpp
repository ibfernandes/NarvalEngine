#include "SceneEditor.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tinyfiledialogs.h"
#include "tinyexr.h"

namespace narvalengine {

	SceneEditor::SceneEditor() {

	};

	void SceneEditor::init(GLint width, GLint height, RendererGL* r, Camera* c) {
		this->WIDTH = width;
		this->HEIGHT = height;
		aspectRatio = (float)width / height;
		rightColumnMenuSize = ImVec2(WIDTH * 0.27f, HEIGHT);
		logWindowSize = ImVec2(WIDTH - rightColumnMenuSize.x, HEIGHT * 0.3f);
		logWindowPos = ImVec2(0, HEIGHT - logWindowSize.y);
		renderSize = glm::vec2(WIDTH - rightColumnMenuSize.x, HEIGHT);

		r->init();
		renderer = r;
		renderCtx.renderer = r;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
		orthoProj = glm::ortho(0.f, -(float)width, 0.f, (float)height, -1.f, 1.f);

		glm::vec3 position(0, 0, 0);
		glm::vec3 front = { 0.0f, 0.0f, 1.0f };
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 side = glm::cross(front, up);
		staticCam = glm::lookAt(position, position + front, up);

		//selectedFilePath = "scenes/cubeMatrixSolid.json";
		//selectedFilePath = "scenes/cubeMatrix.json";
		selectedFilePath = "scenes/testing.json";
		//selectedFilePath = "scenes/cornellbox-pbrt.json";
		//sceneReader.loadScene("scenes/defaultCube.json", false);
		sceneReader.loadScene(selectedFilePath, false);
		scene = *sceneReader.getScene();
		camera = *sceneReader.getMainCamera();
		settings = *sceneReader.getSettings();

		generateSceneImList();

		sceneFrameSize = settings.resolution;
		offEngine = new OfflineEngine(camera, settings, scene);
		currentRenderingMode = settings.renderMode;

		cam = glm::lookAt(*camera.getPosition(), *camera.getPosition() + front, up);
		cameraPosition = *camera.getPosition();
		cameraPositionCache = cameraPosition;

		startTime = glfwGetTime();
		for (int i = MAX_GRAPH_POINTS - 1; i > 0; i--)
			FPSgraphPoints[i] = 0;
	}

	void SceneEditor::initVolumetricShader() {
		Shader *shader = ResourceManager::getSelf()->getShader("volumelbvh");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		volProgramHandler = renderCtx.createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		volUniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		volUniforms[1] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		volUniforms[2] = renderCtx.createUniform("cam", mb3, UniformType::Mat4, 0);
		volUniforms[3] = renderCtx.createUniform("cameraPosition", {(uint8_t*)&cameraPosition[0], sizeof(cameraPosition)}, UniformType::Vec3, 0);
		volUniforms[4] = renderCtx.createUniform("volume", {(uint8_t*)&volumeTexBind, sizeof(volumeTexBind)}, UniformType::Sampler, 0);
		volUniforms[5] = renderCtx.createUniform("background", { (uint8_t*)&bgTexBind, sizeof(bgTexBind) }, UniformType::Sampler, 0);
		volUniforms[6] = renderCtx.createUniform("backgroundDepth", { (uint8_t*)&bgDepthTexBind, sizeof(bgDepthTexBind) }, UniformType::Sampler, 0);
		volUniforms[7] = renderCtx.createUniform("previousCloud", { (uint8_t*)&prevFrameTexBind, sizeof(prevFrameTexBind) }, UniformType::Sampler, 0);
		volUniforms[8] = renderCtx.createUniform("time", { (uint8_t*)&time, sizeof(time) }, UniformType::Float, 0);
		volUniforms[9] = renderCtx.createUniform("screenRes", { (uint8_t*)&sceneFrameSize[0], sizeof(sceneFrameSize) }, UniformType::Vec2, 0);
		volUniforms[10] = renderCtx.createUniform("renderingMode", { (uint8_t*)&volRenderingMode, sizeof(volRenderingMode) }, UniformType::Int, 0);
		volUniforms[11] = renderCtx.createUniform("scattering", { (uint8_t*)&scattering[0], sizeof(scattering) }, UniformType::Vec3, 0);
		volUniforms[12] = renderCtx.createUniform("absorption", { (uint8_t*)&absorption[0], sizeof(absorption) }, UniformType::Vec3, 0);
		volUniforms[13] = renderCtx.createUniform("densityCoef", { (uint8_t*)&densityCoef, sizeof(densityCoef) }, UniformType::Float, 0);
		volUniforms[14] = renderCtx.createUniform("numberOfSteps", { (uint8_t*)&numberOfSteps, sizeof(numberOfSteps) }, UniformType::Float, 0);
		volUniforms[15] = renderCtx.createUniform("shadowSteps", { (uint8_t*)&shadowSteps, sizeof(shadowSteps) }, UniformType::Float, 0);
		volUniforms[16] = renderCtx.createUniform("lightPosition", { (uint8_t*)&lightPosition[0], sizeof(lightPosition) }, UniformType::Vec3, 0);
		volUniforms[17] = renderCtx.createUniform("lightColor", { (uint8_t*)&lightColor[0], sizeof(lightColor) }, UniformType::Vec3, 0);
		volUniforms[18] = renderCtx.createUniform("ambientStrength", { (uint8_t*)&ambientStrength, sizeof(ambientStrength) }, UniformType::Float, 0);
		volUniforms[19] = renderCtx.createUniform("Kc", {(uint8_t*)&Kc, sizeof(Kc) }, UniformType::Float, 0);
		volUniforms[20] = renderCtx.createUniform("Kl", { (uint8_t*)&Kl, sizeof(Kl) }, UniformType::Float, 0);
		volUniforms[21] = renderCtx.createUniform("Kq", { (uint8_t*)&Kq, sizeof(Kq) }, UniformType::Float, 0);
		volUniforms[22] = renderCtx.createUniform("phaseFunctionOption", { (uint8_t*)&phaseFunctionOption, sizeof(phaseFunctionOption) }, UniformType::Float, 0);
		volUniforms[23] = renderCtx.createUniform("g", { (uint8_t*)&g, sizeof(g) }, UniformType::Float, 0);
		volUniforms[24] = renderCtx.createUniform("SPP", { (uint8_t*)&SPP, sizeof(SPP) }, UniformType::Int, 0);
		volUniforms[25] = renderCtx.createUniform("frameCount", { (uint8_t*)&frameCount, sizeof(frameCount) }, UniformType::Int, 0);
		volUniforms[26] = renderCtx.createUniform("enableShadow", { (uint8_t*)&enableShadow, sizeof(enableShadow) }, UniformType::Float, 0);
		volUniforms[27] = renderCtx.createUniform("invmodel", mb1, UniformType::Mat4, 0);
	}

	void SceneEditor::initPhongShader() {
		Shader *shader = ResourceManager::getSelf()->getShader("phong");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		phongProgramHandler = renderCtx.createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };
		MemoryBuffer mb4 = { (uint8_t*)(&lightProjection[0][0]), sizeof(model) };
		MemoryBuffer mb5 = { (uint8_t*)(&lightView[0][0][0]), sizeof(model) };

		phongUniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		phongUniforms[1] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		phongUniforms[2] = renderCtx.createUniform("cam", mb3, UniformType::Mat4, 0);
		phongUniforms[3] = renderCtx.createUniform("lightProj", mb4, UniformType::Mat4, 0);
		phongUniforms[4] = renderCtx.createUniform("lightCam", mb5, UniformType::Mat4, 0);
		phongUniforms[5] = renderCtx.createUniform("cameraPosition", { (uint8_t*)&cameraPosition[0], sizeof(cameraPosition) }, UniformType::Vec3, 0);
		phongUniforms[6] = renderCtx.createUniform("material.diffuse", { (uint8_t*)&phongDiffTex, sizeof(phongDiffTex)}, UniformType::Sampler, 0);
		phongUniforms[7] = renderCtx.createUniform("material.specular", { (uint8_t*)&phongSpecTex, sizeof(phongSpecTex) }, UniformType::Sampler, 0);
		phongUniforms[8] = renderCtx.createUniform("material.normal", { (uint8_t*)&phongNormTex, sizeof(phongNormTex) }, UniformType::Sampler, 0);
		phongUniforms[9] = renderCtx.createUniform("viewMode", { (uint8_t*)&phongRenderingMode, sizeof(phongRenderingMode) }, UniformType::Int, 0);
		phongUniforms[10] = renderCtx.createUniform("shadowMap", { (uint8_t*)&phongShadowMapTex, sizeof(phongShadowMapTex) }, UniformType::Sampler, 0);
		phongUniforms[11] = renderCtx.createUniform("normalMapping", { (uint8_t*)&normalMapping, sizeof(normalMapping) }, UniformType::Int, 0);
		
		numberOfActiveLights = scene.lights.size();
		int index = 0;
		for (int i = 0; i < scene.lights.size(); i++) {
			std::string shaderIndex = std::to_string(index / phongLightsOffset);
			std::string s = "lightPoints[" + shaderIndex + "].position";
			
			lightUniforms[index]   = renderCtx.createUniform(std::string("lightPoints["+shaderIndex +"].position").c_str(), { (uint8_t*)&lightPosition, sizeof(lightPosition) }, UniformType::Vec3, 0);
			lightUniforms[index+1] = renderCtx.createUniform(std::string("lightPoints["+shaderIndex+"].constant").c_str(), { (uint8_t*)&Kc, sizeof(Kc) }, UniformType::Float, 0);
			lightUniforms[index+2] = renderCtx.createUniform(std::string("lightPoints["+shaderIndex+"].linear").c_str(), { (uint8_t*)&Kl, sizeof(Kl) }, UniformType::Float, 0);
			lightUniforms[index+3] = renderCtx.createUniform(std::string("lightPoints["+shaderIndex+"].quadratic").c_str(), { (uint8_t*)&Kq, sizeof(Kq) }, UniformType::Float, 0);
			lightUniforms[index+4] = renderCtx.createUniform(std::string("lightPoints["+shaderIndex+"].ambient").c_str(), { (uint8_t*)&lightColor, sizeof(lightColor) }, UniformType::Vec3, 0);
			lightUniforms[index+5] = renderCtx.createUniform(std::string("lightPoints["+shaderIndex+"].diffuse").c_str(), { (uint8_t*)&lightColor, sizeof(lightColor) }, UniformType::Vec3, 0);
			lightUniforms[index+6] = renderCtx.createUniform(std::string("lightPoints["+shaderIndex+"].specular").c_str(), { (uint8_t*)&lightColor, sizeof(lightColor) }, UniformType::Vec3, 0);
			index += phongLightsOffset;
		}

		phongUniforms[12] = renderCtx.createUniform("numberOfLights", { (uint8_t*)&numberOfActiveLights, sizeof(numberOfActiveLights) }, UniformType::Int, 0);
	}

	void SceneEditor::initPBR() {
		Shader* shader = ResourceManager::getSelf()->getShader("pbr");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		pbrProgramHandler = renderCtx.createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&cam[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb4 = { (uint8_t*)(&lightView[0][0][0]), sizeof(model) };
		MemoryBuffer mb5 = { (uint8_t*)(&lightProjection[0][0]), sizeof(model) };

		//VS uniforms
		pbrUniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		pbrUniforms[1] = renderCtx.createUniform("cam", mb2, UniformType::Mat4, 0);
		pbrUniforms[2] = renderCtx.createUniform("proj", mb3, UniformType::Mat4, 0);
		pbrUniforms[3] = renderCtx.createUniform("lightCam", mb4, UniformType::Mat4, 0);
		pbrUniforms[4] = renderCtx.createUniform("lightProj", mb5, UniformType::Mat4, 0);

		//FS uniforms
		numberOfActiveLights = scene.lights.size();
		pbrUniforms[5] = renderCtx.createUniform("numberOfLights", { (uint8_t*)&numberOfActiveLights, sizeof(numberOfActiveLights) }, UniformType::Int, 0);
		int index = 0;
		for (int i = 0; i < scene.lights.size(); i++) {
			std::string shaderIndex = std::to_string(index / pbrLightsOffset);
			std::string s = "lightPoints[" + shaderIndex + "].position";

			pbrLightUniforms[index] = renderCtx.createUniform(std::string("lightPoints[" + shaderIndex + "].position").c_str(), { (uint8_t*)&lightPosition, sizeof(lightPosition) }, UniformType::Vec3, 0);
			pbrLightUniforms[index + 1] = renderCtx.createUniform(std::string("lightPoints[" + shaderIndex + "].color").c_str(), { (uint8_t*)&lightColor, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			index += pbrLightsOffset;
		}

		
		pbrUniforms[6] = renderCtx.createUniform("material.diffuse", { (uint8_t*)&texIds[0], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[7] = renderCtx.createUniform("material.metallic", { (uint8_t*)&texIds[1], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[8] = renderCtx.createUniform("material.specular", { (uint8_t*)&texIds[2], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[9] = renderCtx.createUniform("material.normal", { (uint8_t*)&texIds[3], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[10] = renderCtx.createUniform("material.roughness", { (uint8_t*)&texIds[4], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[11] = renderCtx.createUniform("material.ao", { (uint8_t*)&texIds[5], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[12] = renderCtx.createUniform("cameraPosition", { (uint8_t*)&cameraPosition[0], sizeof(cameraPosition) }, UniformType::Vec3, 0);
		pbrUniforms[13] = renderCtx.createUniform("shadowMap", { (uint8_t*)&texIds[6], sizeof(int) }, UniformType::Sampler, 0);

	}
	
	void SceneEditor::initComposeShader() {

		Shader *shader = ResourceManager::getSelf()->getShader("composeTex");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		composeTexProgramH = renderCtx.createProgram(shvertex, shfragment);

		composeTexUniforms[0] = renderCtx.createUniform("tex1", { (uint8_t*)&tex0, sizeof(tex0) }, UniformType::Sampler, 0);
		composeTexUniforms[1] = renderCtx.createUniform("tex2", { (uint8_t*)&tex1, sizeof(tex1) }, UniformType::Sampler, 0);
		composeTexUniforms[2] = renderCtx.createUniform("depth1", { (uint8_t*)&tex2, sizeof(tex2) }, UniformType::Sampler, 0);
		composeTexUniforms[3] = renderCtx.createUniform("depth2", { (uint8_t*)&tex3, sizeof(tex3) }, UniformType::Sampler, 0);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&orthoProj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&staticCam[0][0]), sizeof(model) };

		composeTexUniforms[4] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		composeTexUniforms[5] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		composeTexUniforms[6] = renderCtx.createUniform("cam", mb3, UniformType::Mat4, 0);
	}

	void SceneEditor::initRenderAPI() {
		initVolumetricShader();
		initPhongShader();
		initShadowShader();
		initPBR();

		//Init rayShoot debugger
		rayVertexLayout.init();
		rayVertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		rayVertexLayout.end();
		rayTextureColorH = renderCtx.createTexture(1, 1, 0, TextureLayout::RGBA32F, { (uint8_t*)&lineColor[0], sizeof(glm::vec4) }, NE_TEX_SAMPLER_MIN_MAG_NEAREST | NE_TEX_SAMPLER_UVW_MIRROR);

		for (int i = 0; i < 50; i++) {
			rayIndices[i] = nullptr;
		}

		for (int i = 0; i < scene.instancedModels.size(); i++) {
			InstancedModel *im = scene.instancedModels.at(i);
			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
				if (rmToRenderAPI.find(im->modelID) != rmToRenderAPI.end())
					continue;

				Model *model = ResourceManager::getSelf()->getModel(im->modelID);
				ModelHandler mh = renderCtx.createModel(model);
				rmToRenderAPI.insert({ im->modelID, mh });
			}

			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
				if (rmToRenderAPI.find(im->modelID) != rmToRenderAPI.end())
					continue;

				Model *model = ResourceManager::getSelf()->getModel(im->modelID);
				ModelHandler mh = renderCtx.createModel(model);
				rmToRenderAPI.insert({ im->modelID, mh });
			}
		}

		for (int i = 0; i < scene.lights.size(); i++) {
			InstancedModel* im = scene.lights.at(i);
			if (rmToRenderAPI.find(im->modelID) != rmToRenderAPI.end())
				continue;

			Model* model = ResourceManager::getSelf()->getModel(im->modelID);
			ModelHandler mh = renderCtx.createModel(model);
			rmToRenderAPI.insert({ im->modelID, mh });
		}

		lightTexColorH = renderCtx.createTexture(1, 1, 0, TextureLayout::RGBA32F, { (uint8_t*)&lightTexColor[0], sizeof(glm::vec4) }, NE_TEX_SAMPLER_MIN_MAG_NEAREST | NE_TEX_SAMPLER_UVW_MIRROR);
	}

	void SceneEditor::initShadowShader() {
		Shader* shader = ResourceManager::getSelf()->getShader("simpleLightDepth");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		shadowProgramHandler = renderCtx.createProgram(shvertex, shfragment);
		glm::ivec2 res = settings.resolution;

		//lightProjection = glm::ortho(-(float)1.0f/ 2.0f, (float)1.0f / 2.0f, -(float)1.0f / 2.0f, (float)1.0f / 2.0f, 0.0f, 1.f);
		lightProjection = glm::ortho(4.5f, -4.5f, 4.5f, -4.5f, 0.0f, 10.f);
		//lightProjection = glm::ortho(0.f, -(float)res.x, 0.f, (float)res.y, -1.f, 1.f);
		//lightProjection = glm::ortho(-(float)res.x / 2.0f, (float)res.x / 2.0f, -(float)res.y / 2.0f, (float)res.y / 2.0f, 0.f, 10.f);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&lightProjection[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		shadowUniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		shadowUniforms[1] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		shadowUniforms[2] = renderCtx.createUniform("view", mb3, UniformType::Mat4, 0);

		uint32_t sizeBytes = res.x * res.y * 4;
		shadowDepthTex = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);

		Attachment a[1];
		a[0] = { shadowDepthTex };
		shadowfboh = renderCtx.createFrameBuffer(1, &a[0]);
	}

	void SceneEditor::initMonoColorShader() {
		Shader* shader = ResourceManager::getSelf()->getShader("monocolor");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		monoColorProgramH = renderCtx.createProgram(shvertex, shfragment);

		monoColorUniforms[3] = renderCtx.createUniform("rgbColor", { (uint8_t*)&outlineObjColor, sizeof(outlineObjColor) }, UniformType::Vec4, 0);


		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		monoColorUniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		monoColorUniforms[1] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		monoColorUniforms[2] = renderCtx.createUniform("cam", mb3, UniformType::Mat4, 0);
	}

	void SceneEditor::initImageDifferenceShader() {
		Shader* shader = ResourceManager::getSelf()->getShader("imageDifference");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		imgDiffProgramH = renderCtx.createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		imgDiffUniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		imgDiffUniforms[1] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		imgDiffUniforms[2] = renderCtx.createUniform("cam", mb3, UniformType::Mat4, 0);

		imgDiffUniforms[3] = renderCtx.createUniform("tex1", { (uint8_t*)(&imgDiffId0), sizeof(int) }, UniformType::Sampler, 0);
		imgDiffUniforms[4] = renderCtx.createUniform("tex2", { (uint8_t*)(&imgDiffId1), sizeof(int) }, UniformType::Sampler, 0);
	}

	void SceneEditor::load() {
		initComposeShader();
		initMonoColorShader();
		initImageDifferenceShader();

		Shader *shader = ResourceManager::getSelf()->getShader("simpleTexture");

		ShaderHandler shvertex = renderCtx.createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx.createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);

		simpleTexProgramH = renderCtx.createProgram(shvertex, shfragment);
		quadModelHandler = renderCtx.createModel(ResourceManager::getSelf()->getModel("quad"));

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&orthoProj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&staticCam[0][0]), sizeof(model) };

		uniforms[0] = renderCtx.createUniform("model", mb1, UniformType::Mat4, 0);
		uniforms[1] = renderCtx.createUniform("proj", mb2, UniformType::Mat4, 0);
		uniforms[2] = renderCtx.createUniform("cam", mb3, UniformType::Mat4, 0);
		uniforms[4] = renderCtx.createUniform("tex", { (uint8_t*)(&texPos), sizeof(int)}, UniformType::Sampler, 0);

		uint32_t texSize = settings.resolution.x * settings.resolution.y * 3 * sizeof(float);
		fboTex = new Texture(settings.resolution.x, settings.resolution.y, TextureLayout::RGB32F, NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_LINEAR, {new uint8_t[texSize], texSize});

		fboTexHandler = renderCtx.createTexture(fboTex);

		glm::ivec2 res = settings.resolution;
		uint32_t sizeBytes = res.x * res.y * 3 * 4;
		uint8_t *data = new uint8_t[sizeBytes];
		float *fdata = (float*)data;
		for (int i = 0; i < sizeBytes/4; i++) {
			fdata[i] = 0.7f;
		}
		renderFrameTex[0] = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { data, sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameTex[1] = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameTex[2] = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);

		sizeBytes = res.x * res.y * 4; // 3 bytes per depth pixel (4 to fill integer)
		uint8_t* dataD = new uint8_t[sizeBytes];
		renderFrameDepthTex[0] = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::D24S8, { dataD, sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameDepthTex[1] = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameDepthTex[2] = renderCtx.createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
	
		Attachment a[2];
		a[0] = { renderFrameTex[0] };
		a[1] = { renderFrameDepthTex[0] };
		fbRenderFrame[0] = renderCtx.createFrameBuffer(2, &a[0]);

		Attachment b[2];
		b[0] = { renderFrameTex[1] };
		b[1] = { renderFrameDepthTex[1] };
		fbRenderFrame[1] = renderCtx.createFrameBuffer(2, &b[0]);

		Attachment c[2];
		c[0] = { renderFrameTex[2] };
		c[1] = { renderFrameDepthTex[2] };
		fbRenderFrame[2] = renderCtx.createFrameBuffer(2, &c[0]);

		initRenderAPI();
	};

	void SceneEditor::updateUI() {

	};

	bool SceneEditor::sceneChanged() {
		if (cameraPositionCache != *camera.getPosition()) {
			float d = 0;
			offEngine->camera = camera;
			cameraPositionCache = *camera.getPosition();
			return true;
		}
	};

	void SceneEditor::hoverObject() {
		glm::vec2 mousePos = InputManager::getSelf()->getMousePosition();
		glm::vec2 renderFrameLeftCorner = glm::vec2((WIDTH - rightColumnMenuSize.x) / 2 - sceneFrameSize.x / 2, (HEIGHT - menuBarSize.y) / 2 + menuBarSize.y - sceneFrameSize.y / 2);

		bool selectObjectInput = InputManager::getSelf()->eventTriggered("SELECT_OBJECT");

		if (mousePos.x >= renderFrameLeftCorner.x && mousePos.x < renderFrameLeftCorner.x + sceneFrameSize.x) {
			if (mousePos.y >= renderFrameLeftCorner.y && mousePos.y < renderFrameLeftCorner.y + sceneFrameSize.y) {
				glm::vec2 uv = (mousePos - renderFrameLeftCorner) / sceneFrameSize;
				uv.y = 1 - uv.y;
				//uv.x = 1 - uv.x;
				uv = uv * 2.0f - 1.0f; //tranform to NDC [-1, 1]


				glm::vec4 coords = glm::vec4(uv.x, uv.y, -1, 1);
				glm::mat4 inv = glm::inverse(proj); //TODO could pre calc this
				coords = inv * coords;
				coords.w = 1;

				inv = glm::inverse(*camera.getCam());
				coords = inv * coords;
				//printVec3(*camera.getPosition());
				Ray r(*camera.getPosition(), glm::normalize(glm::vec3(coords) - *camera.getPosition()));
				RayIntersection ri;
				ri.tNear = 999999;
				ri.tFar = -999999;
				bool didHit = scene.intersectScene(r, ri, 0, 999);

				//if (didHit)
				//	std::cout << "hit" << std::endl;

				//TODO: could check input first, then check for function then if no input, no raycasting
				if (selectObjectInput && didHit) {
					currentSelectedIM = ri.instancedModel;
					glm::mat4 transf = currentSelectedIM->transformToWCS;

					selectObjPos = getTranslation(transf);
					selectObjScale = getScale(transf);
					selectObjRotation = getRotation(transf);

					selectedImList = getSelectedIMListPos(currentSelectedIM);
				}
				else if(!didHit && selectObjectInput){
					selectedImList = -1;
					currentSelectedIM = nullptr;
					selectObjPos = glm::vec3(0);
					selectObjScale = glm::vec3(0);
					selectObjRotation = glm::vec3(0);
				}
			}
		}
	};

	void SceneEditor::update(float deltaTime) {
		hoverObject();
		updateUI();
		camera.update();

		if (shouldUpdateCamera) {
			shouldUpdateCamera = false;
			proj = glm::perspective(glm::radians(camera.vfov), float(WIDTH)/HEIGHT, nearPlane, farPlane);
			offEngine->updateOfflineEngine(camera, settings, scene);
		}

		if (currentSelectedIM != nullptr && shouldUpdateSelectedIM) {
			currentSelectedIM->transformToWCS = getTransform(selectObjPos, selectObjRotation, selectObjScale);
			currentSelectedIM->invTransformToWCS = glm::inverse(currentSelectedIM->transformToWCS);
		}

		bool didSceneChange = sceneChanged();

		if (didSceneChange && currentRenderingMode == OFFLINE_RENDERING_MODE)
			startOffEngine();

		if (currentRenderingMode == OFFLINE_RENDERING_MODE || currentRenderingMode == COMPARE_RENDERING_MODE) {
			uint32_t sizeBytes = offEngine->settings.resolution.x * offEngine->settings.resolution.y * sizeof(int) * 3;

			//While all tiles are not rendered keep checking which threads are done and realloc them to reamining tiles to render.
			for (int i = 0; i < offEngine->numberOfThreads; i++) {
				if (renderedTiles == offEngine->numberOfTiles.x * offEngine->numberOfTiles.y)
					break;

				if (offEngine->isThreadDone[i]) {
					offEngine->threadPool[i].join();
					offEngine->isThreadDone[i] = false;
					offEngine->threadPool[i] = std::thread(&OfflineEngine::renderTile, offEngine, std::ref(offEngine->camera), renderedTiles, std::ref(offEngine->isThreadDone[i]));
					renderedTiles++;
					renderCtx.updateTexture(fboTexHandler, 0, 0, 0, settings.resolution.x, settings.resolution.y, 0,{ (uint8_t*)(offEngine->pixels),  sizeBytes });
				}
			}

			if (renderedTiles == offEngine->numberOfTiles.x * offEngine->numberOfTiles.y && !doneRendering) {
				bool allThreadsFinished = true;
				for (int i = 0; i < offEngine->numberOfThreads; i++) {
					if (!offEngine->isThreadDone[i]) {
						offEngine->threadPool[i].join();
						allThreadsFinished = false;
					}
				}

				//if not all threads finish, wait until they do finish
				if (!allThreadsFinished)
					return;

				renderCtx.updateTexture(fboTexHandler, 0, 0, 0, settings.resolution.x, settings.resolution.y, 0, { (uint8_t*)(offEngine->pixels),  sizeBytes });
				std::stringstream ss;
				ss.precision(0);
				ss << std::fixed << "Finished rendering scene in " << offEngineTimer.getElapsedSeconds() << " seconds." << std::endl;
				logText += ss.str();

				offEngineTimer.endTimer();
				doneRendering = true;
			}
		}

	};

	void SceneEditor::startOffEngine() {
		offEngineTimer.startTimer();
		doneRendering = false;
		renderedTiles = 0;

		//TODO delete previous loaded scene or update it on ResourceManager.
		if (currentRenderingMode == OFFLINE_RENDERING_MODE)
			offEngine = new OfflineEngine(camera, settings, scene);

		//Initializes first N threads
		for (int i = 0; i < offEngine->numberOfThreads; i++) {
			offEngine->threadPool[i] = std::thread(&OfflineEngine::renderTile, offEngine, std::ref(offEngine->camera), renderedTiles, std::ref(offEngine->isThreadDone[i]));
			renderedTiles++;
		}
	}

	void SceneEditor::reloadScene() {

		sceneReader.loadScene(selectedFilePath, false);
		scene = *sceneReader.getScene();
		camera = *sceneReader.getMainCamera();
		settings = *sceneReader.getSettings();

		sceneFrameSize = settings.resolution;
		currentRenderingMode = settings.renderMode;

		//startOffEngine(); //TODO why was I starting it here?

		//camera = *sceneReader.getMainCamera();
		initRenderAPI();
		generateSceneImList();
	}

	void SceneEditor::compareScene() {
		const char* title = "Open file";
		const char* startingFolder = "./";
		const char* optionalFileExtensionFilterString = "";//".jpg;.jpeg;.png;.tiff;.bmp;.gif;.txt";

		const char* pathptr = tinyfd_openFileDialog(title, startingFolder, 0, NULL, NULL, 0);
		if (pathptr != nullptr) {
			std::string imgPath = std::string(pathptr);

			if (imgPath.find(".exr") != std::string::npos) {
				const char* input = imgPath.c_str();
				float* data; // width * height * RGBA
				int width;
				int height;
				const char* err = NULL; // or nullptr in C++11

				int ret = LoadEXR(&data, &width, &height, input, &err);

				if (ret != TINYEXR_SUCCESS) {
					if (err) {
						fprintf(stderr, "ERR : %s\n", err);
						FreeEXRErrorMessage(err); // release memory of error message.
					}
				}
				else {
					//free(data); // release memory of image data
				}

				uint32_t sizeBytes = sizeof(float) * width * height * 4;

				int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_NEAREST;

				Texture* t = new Texture(width, height, RGBA32F, flags, { (uint8_t*)data, sizeBytes });
				free(data);

				texIDToCompare = ResourceManager::getSelf()->replaceTexture(imgPath, t);
				currentRenderingMode = COMPARE_RENDERING_MODE;

				compareTex = renderCtx.createTexture(ResourceManager::getSelf()->getTexture(texIDToCompare));

			}else {
				int width, height, channels;
				unsigned char* data = stbi_load(imgPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

				if (data == nullptr)
					throw "Error loading texture";

				uint32_t sizeBytes = sizeof(char) * width * height * 4;

				int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_NEAREST;

				Texture* t = new Texture(width, height, RGBA8, flags, { data, sizeBytes });
				stbi_image_free(data);

				texIDToCompare = ResourceManager::getSelf()->replaceTexture(imgPath, t);
				currentRenderingMode = COMPARE_RENDERING_MODE;

				compareTex = renderCtx.createTexture(ResourceManager::getSelf()->getTexture(texIDToCompare));
			}
			startOffEngine();
		}
	}

	void SceneEditor::variableUpdate(float deltaTime) {

	};

	int SceneEditor::getSelectedIMListPos(InstancedModel *selectedIm) {
		//TODO for now the first one is always the camera.
		int listPos = 1;

		//TODO could use a LBVH tree to sort through all scene's elements
		for (InstancedModel* im : scene.lights) {
			if (selectedIm == im)
				return listPos;
			listPos++;
		}

		for (InstancedModel* im : scene.instancedModels) {
			if (selectedIm == im)
				return listPos;
			listPos++;
		}

		//reached the end of the list with no matches, return -1, no item was selected
		return -1;
	}

	InstancedModel *SceneEditor::getSelectedIMfromListPos(int pos) {
		//TODO for now the first one is always the camera.
		int listPos = 1;

		//TODO could use a LBVH tree to sort through all scene's elements
		for (InstancedModel* im : scene.lights) {
			if (listPos == pos)
				return im;
			listPos++;
		}

		for (InstancedModel* im : scene.instancedModels) {
			if (listPos == pos)
				return im;
			listPos++;
		}

		//reached the end of the list with no matches, return -1, no item was selected
		return nullptr;
	}

	void SceneEditor::generateSceneImList() {

		imNamesCurrentLength = 0;
		//TODO: Support multiple cameras in the future
		imNames[imNamesCurrentLength] = "Camera";
		imIcons[imNamesCurrentLength] = ICON_FA_CAMERA;
		imNamesCurrentLength++;

		for (InstancedModel* im : scene.lights) {
			int s = gStringIDTable.size();
			imNames[imNamesCurrentLength] = (char*)gStringIDTable[im->modelID];

			if (!im->model->lights.empty())
				imIcons[imNamesCurrentLength] = ICON_FA_LIGHTBULB;

			imNamesCurrentLength++;
		}

		for (InstancedModel* im : scene.instancedModels) {
			int s = gStringIDTable.size();
			imNames[imNamesCurrentLength] = (char*)gStringIDTable[im->modelID];
			
			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE)
				imIcons[imNamesCurrentLength] = ICON_FA_VECTOR_SQUARE;
			else if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION)
				imIcons[imNamesCurrentLength] = ICON_FA_CLOUD;

			imNamesCurrentLength++;
		}
	}

	void SceneEditor::sceneListImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		if (ImGuiExt::CollapsingHeader("Scene", gUIPalette.iceGrey[4], ImGuiTreeNodeFlags_Bullet)) {
			ImGui::PushStyleColor(ImGuiCol_Header, gUIPalette.blueGrey[2]); //selected
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, gUIPalette.iceGrey[4]);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, gUIPalette.iceGrey[5]);
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(11, 11));
			ImGui::BeginChild("##sceneListChild", ImVec2(0, ImGui::GetFontSize() * 8), false, 0);
			ImGui::Columns(3, "##iconcolum", false);
			ImGui::SetColumnWidth(0, columnOffset.x);
			ImGui::SetColumnWidth(1, ImGui::GetFontSize() * 1.5f);
			ImGuiListClipper clipper(imNamesCurrentLength);
			ImGui::Dummy(ImVec2(0, 1)); ImGui::NextColumn();
			ImGui::Dummy(ImVec2(0, 1)); ImGui::NextColumn();
			ImGui::Dummy(ImVec2(0, 1)); ImGui::NextColumn();

			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					ImGui::BeginGroup();

					ImGui::Text("");
					ImGui::NextColumn();

					bool selectedItem = (i == selectedImList);

					if (selectedItem)
						ImGui::PushStyleColor(ImGuiCol_Text, gUIPalette.yellow[1]);
					else
						ImGui::PushStyleColor(ImGuiCol_Text, gUIPalette.textLight);

					ImGui::PushID(i);
					if (ImGui::Selectable(imIcons[i], selectedImList == i, ImGuiSelectableFlags_SpanAllColumns)) {
						currentSelectedIM = getSelectedIMfromListPos(i);
						selectedImList = i;
					}
					ImGui::PopID();

					if (!selectedItem)
						ImGui::PopStyleColor();

					ImGui::NextColumn();
					bool hovered = ImGui::IsItemHovered();
					ImGui::Text(imNames[i], hovered); ImGui::NextColumn();

					if (selectedItem)
						ImGui::PopStyleColor();

					ImGui::EndGroup();
				}
			}

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}
	}

	void SceneEditor::selectedModelImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Transform", gUIPalette.iceGrey[4])) {
			ImVec2 size = ImGui::GetContentRegionMax();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			ImGui::BeginChild("##transformChild", ImVec2(rightColumnMenuSize.x, ImGui::GetFontSize() * (3 + 2)), 0,  ImGuiWindowFlags_NoScrollWithMouse);
			//BeginChild Item padding

			float widthSpacing = ImGui::CalcTextSize("Position: ", 0, false, -1).x + columnOffset.x;
			ImGui::Columns(3, NULL, false);
			ImGui::SetColumnWidth(0, columnOffset.x);
			ImGui::SetColumnWidth(1, widthSpacing);
			ImGui::SetColumnWidth(2, rightColumnMenuSize.x - widthSpacing);

			ImGui::Dummy(ImVec2(1, columnOffset.y));
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Position: ");
			ImGui::NextColumn();
			if(ImGui::DragFloat3("##selectObjPos", &selectObjPos[0], 0.1, -999, 999, "%.2f"))
				shouldUpdateSelectedIM = true;
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Scale: ");
			ImGui::NextColumn();
			if(ImGui::DragFloat3("##selectObjScale", &selectObjScale[0], 0.1, -999, 999, "%.2f"))
				shouldUpdateSelectedIM = true;
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Rotation: ");
			ImGui::NextColumn();
			if (ImGui::DragFloat3("##selectObjRotation", &selectObjRotation[0], 0.1, -999, 999, "%.2f"))
				shouldUpdateSelectedIM = true;

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}

		//ImGui::Dummy(ImVec2(0, -200));
	};

	void SceneEditor::selectedMaterialImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);

		if (ImGuiExt::CollapsingHeader("Material", gUIPalette.iceGrey[4])) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			float height = 1;
			if (currentSelectedIM != nullptr && currentSelectedIM->model->lights.size() == 0) {
				if (currentSelectedIM->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
					height = ImGui::GetFontSize() * (7 + 4);
				}else if (currentSelectedIM->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
					height = ImGui::GetFontSize() * (1 + 2);
				}
			}

			ImGui::BeginChild("##selectMatChild", ImVec2(rightColumnMenuSize.x, height), 0, ImGuiWindowFlags_NoScrollWithMouse);
			if (currentSelectedIM != nullptr && currentSelectedIM->model->lights.size() == 0) {
				if (currentSelectedIM->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
					float widthSpacing = ImGui::CalcTextSize("Density Multiplier: ", 0, false, -1).x + columnOffset.x;
					ImGui::Columns(3, NULL, false);

					ImGui::SetColumnWidth(0, columnOffset.x);
					ImGui::SetColumnWidth(1, widthSpacing);
					ImGui::SetColumnWidth(2, rightColumnMenuSize.x - widthSpacing - columnOffset.x);

					ImGui::Dummy(ImVec2(1, columnOffset.y));
					ImGui::NextColumn();
					ImGui::NextColumn();
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Absorption (1/m): ");
					ImGui::NextColumn();
					ImGui::DragFloat3("##absorption", &absorption[0], 0.001f, 0.001f, 5.0f);
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Scattering (1/m): ");
					ImGui::NextColumn();
					ImGui::DragFloat3("##scattering", &scattering[0], 0.001f, 0.001f, 5000.0f);
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Density Multiplier: ");
					ImGui::NextColumn();
					ImGui::PushItemWidth(80.0f);
					ImGui::DragFloat("##densityCoef", &densityCoef, 0.01, 0.001f, 30.0);
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("N. of Steps: ");
					ImGui::NextColumn();
					ImGui::DragFloat("##numberOfSteps", &numberOfSteps, 1, 0.0, 256);
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Shadow Steps");
					ImGui::NextColumn();
					ImGui::DragFloat("##shadowSteps", &shadowSteps, 1, 0.0, 256);
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Render Mode: ");
					ImGui::NextColumn();
					ImGui::Combo("##currentRenderMode", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes));
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Phase Function: ");
					ImGui::NextColumn();
					ImGui::Combo("##phaseFunctionOption", &phaseFunctionOption, phaseFunctionOptions, IM_ARRAYSIZE(phaseFunctionOptions));
					ImGui::NextColumn();

					ImGui::Columns(1);
				}
				else if (currentSelectedIM->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
					ImGui::Columns(2, NULL, false);

					ImGui::SetColumnWidth(0, 150);
					ImGui::SetColumnWidth(1, rightColumnMenuSize.x - 150);

					ImGui::Text("Texture: ");
					ImGui::NextColumn();
					ImGui::Button("Select...");
					ImGui::NextColumn();


					ImGui::Columns(1);
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
	};

	void SceneEditor::cameraImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Camera", gUIPalette.iceGrey[4])) {
			ImVec2 size = ImGui::GetContentRegionMax();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			float height = ImGui::GetFontSize() * (3 + 2);
			float widthSpacing = ImGui::CalcTextSize("Focus Dist. (m): ", 0, false, -1).x + columnOffset.x;
			ImGui::BeginChild("##cameraChild", ImVec2(rightColumnMenuSize.x, height), 0, ImGuiWindowFlags_NoScrollWithMouse);
			//BeginChild Item padding

			ImGui::Columns(3, NULL, false);

			ImGui::SetColumnWidth(0, columnOffset.x);
			ImGui::SetColumnWidth(1, widthSpacing);
			ImGui::SetColumnWidth(2, rightColumnMenuSize.x - widthSpacing);

			ImGui::Dummy(ImVec2(1, columnOffset.y));
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("vFov: ");
			ImGui::NextColumn();
			if (ImGui::DragFloat("##vfovcam", &camera.vfov, 1.0, 0, 180, "%.1f"))
				shouldUpdateCamera = true;
			ImGui::NextColumn();
			
			ImGui::NextColumn();
			ImGui::Text("Aperture (mm): ");
			ImGui::NextColumn();
			if(ImGui::DragFloat("##aperturecam", &camera.aperture, 1.0, 0, 180, "%.1f"))
				shouldUpdateCamera = true;
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Focus Dist. (m): ");
			ImGui::NextColumn();
			if(ImGui::DragFloat("##focuscam", &camera.focusDistance, 1.0, 0, 100, "%.1f"))
				shouldUpdateCamera = true;
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}
	};

	void SceneEditor::lightImGui() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Light", gUIPalette.iceGrey[4])) {
			ImVec2 size = ImGui::GetContentRegionMax();

			float height = ImGui::GetFontSize() * (2 + 2);
			float widthSpacing = ImGui::CalcTextSize("Power (W): ", 0, false, -1).x + columnOffset.x;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			ImGui::BeginChild("##lightChild", ImVec2(rightColumnMenuSize.x, height), 0, ImGuiWindowFlags_NoScrollWithMouse);
			//BeginChild Item padding

			ImGui::Columns(3, NULL, false);

			ImGui::SetColumnWidth(0, columnOffset.x);
			ImGui::SetColumnWidth(1, widthSpacing);
			ImGui::SetColumnWidth(2, rightColumnMenuSize.x - widthSpacing);

			ImGui::Dummy(ImVec2(1, columnOffset.y));
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Power (W): ");
			ImGui::NextColumn();
			//TODO not quite right, there should be a treatment case for Le() and Li()
			ImGui::DragFloat3("##lightpower", &currentSelectedIM->model->lights.at(0)->material->light->li[0], 0.5, 0, 1000, "%.1f");
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}
	};

	void SceneEditor::renderOptionsImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Render Options", gUIPalette.iceGrey[4])) {
			ImVec2 size = ImGui::GetContentRegionMax();

			float height = ImGui::GetFontSize() * (3 + 2);
			float widthSpacing = ImGui::CalcTextSize("Resolution: ", 0, false, -1).x + columnOffset.x;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			ImGui::BeginChild("##renderoptions", ImVec2(rightColumnMenuSize.x, height), 0, ImGuiWindowFlags_NoScrollWithMouse);
			//BeginChild Item padding

			ImGui::Columns(3, NULL, false);

			ImGui::SetColumnWidth(0, columnOffset.x);
			ImGui::SetColumnWidth(1, widthSpacing);
			ImGui::SetColumnWidth(2, rightColumnMenuSize.x - widthSpacing);

			ImGui::Dummy(ImVec2(1, columnOffset.y));
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Resolution: ");
			ImGui::NextColumn();
			ImGui::DragInt2("##resolution", &settings.resolution[0], 1.0, 0, 3840, "%.1f");
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("SPP: ");
			ImGui::NextColumn();
			ImGui::DragInt("##spp", &settings.spp, 1.0, 0, 1000, "%.1f");
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Bounces: ");
			ImGui::NextColumn();
			ImGui::DragInt("##bounces", &settings.bounces, 1.0, 0, 10, "%.1f");
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}
	};

	void SceneEditor::performanceImGUI() {
		if (glfwGetTime() - startTime >= 1.0) {
			lastFPS = FPSCount;

			FPSgraphPoints[0] = FPSCount;

			for (int i = MAX_GRAPH_POINTS - 1; i > 0; i--)
				FPSgraphPoints[i] = FPSgraphPoints[i - 1];

			startTime = glfwGetTime();
			FPSCount = 0;
		}
		FPSCount++;

		
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Performance", gUIPalette.iceGrey[4])) {

			float height = ImGui::GetFontSize() * (5 + 2);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));	
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			ImGui::BeginChild("##perfChild", ImVec2(rightColumnMenuSize.x, height), 0, ImGuiWindowFlags_NoScrollWithMouse);

			ImGui::Columns(2, NULL, false);

			ImGui::SetColumnWidth(0, columnOffset.x);
			ImGui::SetColumnWidth(1, rightColumnMenuSize.x - columnOffset.x);

			ImGui::Dummy(ImVec2(1, columnOffset.y));
			ImGui::NextColumn();
			ImGui::NextColumn();

			if (currentRenderingMode == REALTIME_RENDERING_MODE) {
				ImGui::NextColumn();
				ImGui::Text("FPS:   %d", lastFPS);
				ImGui::PlotLines("", FPSgraphPoints, IM_ARRAYSIZE(FPSgraphPoints), 0, 0, 0, 200, ImVec2(rightColumnMenuSize.x - 80, ImGui::GetFontSize() * 4));
			}
			else if (currentRenderingMode == OFFLINE_RENDERING_MODE) {
				ImGui::NextColumn();
				if(!doneRendering)
					ImGui::Text("Elapsed Time:   %.0f s", offEngineTimer.getElapsedSeconds());
				else
					ImGui::Text("Elapsed Time:   %.0f s", offEngineTimer.getSeconds());
				ImGui::Text("Completed:   %.0f %%", float(renderedTiles)/(offEngine->numberOfTiles.x * offEngine->numberOfTiles.y) * 100.0f);
			}

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
	};

	void SceneEditor::shootRay(int i) {
		glm::vec3 rayDir;
		numberOfBounces = 0;

		if (pointTo)
			rayDir = glm::normalize(rayDirection - rayOrigin);
		else 
			rayDir = glm::normalize(rayDirection);

		intersectionPoints[i] = offEngine->pathIntegrator.getPath(Ray(rayOrigin, rayDir), &scene);
		if (rayIndices[i] != nullptr)
			delete rayIndices[i];

		int numberOfLines = (intersectionPoints[i].size() / 2.0f);
		if (intersectionPoints[i].size() > 2)
			numberOfLines++;

		numberOfBounces = numberOfLines;
		rayIndices[i] = new int[numberOfLines * 2]; // 2 indices fot 2 vertices per line

		int k = 0;
		for (int j = 0; j < numberOfLines; j++) {
			rayIndices[i][k] = j;
			rayIndices[i][k+1] = j + 1;
			k += 2;
		}

		//TODO should delete previous Handlers!!
		uint32_t verticesSizeInBytes = intersectionPoints[i].size() * sizeof(glm::vec3);
		std::vector<glm::vec3> pathPoints;
		for (int k = 0; k < intersectionPoints[i].size(); k++)
			pathPoints.push_back(intersectionPoints[i][k].incoming.o);

		rayVertexBufferH[i] = renderCtx.createVertexBuffer({ (uint8_t*)&pathPoints.front(), verticesSizeInBytes }, rayVertexLayout);
		rayIndexBufferH[i] = renderCtx.createIndexBuffer({ (uint8_t*)rayIndices[i], numberOfLines * 2 * sizeof(int) }, rayVertexLayout);

		std::stringstream ss;
		for (int i = 0; i < intersectionPoints[0].size(); i++) {
			PointPathDebugInfo point = intersectionPoints[0][i];
			ss << "Point " << i << std::endl;
			ss << toString(point.incoming.o, "inc. origin: ") << std::endl;
			ss << toString(point.incoming.d, "inc. dir: ") << std::endl;
			ss << toString(point.bsdf, "bsdf: ") << std::endl;
			ss << toString(point.Tr, "Tr: ") << std::endl;
			ss << toString(point.Li, "Li:  ") << std::endl;
			ss << "--------------------------------" << std::endl;
		}
		logText += ss.str();
	}

	void SceneEditor::shootMultipleRays(int qtt){
		for (int i = 0; i < qtt; i++) {
			shootRay(i);
		}
	}
	
	void SceneEditor::shootRayImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Ray Shooter", gUIPalette.iceGrey[4])) {
			ImGui::Checkbox("Point to", &pointTo);
			if (pointTo) {
				ImGui::Text("Point To: ");
				ImGui::InputFloat3("##PointTo", &rayDirection[0], 1, 0);
			}else {
				ImGui::Text("Direction: ");
				ImGui::InputFloat3("##Direction", &rayDirection[0], 1, 0);
			}
			ImGui::Text("Origin: ");
			ImGui::InputFloat3("##Origin", &rayOrigin[0], 1, 0);
			ImGui::Text("Number of bounces: %d", numberOfBounces);
			if (ImGui::Button("Shoot ray", ImVec2(0, 0))) {
				numberOfShootedRays = 1;
				shootRay(0);
			}
			if (ImGui::Button("Shoot Multiple ray", ImVec2(0, 0))) {
				numberOfShootedRays = 18;
				shootMultipleRays(18);
			}

			//First point is always the "camera" starting point
			/*for (int i = 0; i < intersectionPoints[0].size(); i++) {
				PointPathDebugInfo point = intersectionPoints[0][i];
				ImGui::Separator();
				ImGui::Text("Point %d", i);
				ImGuiExt::TextVec3(point.incoming.o, "inc. origin: ");
				ImGuiExt::TextVec3(point.incoming.d, "inc. dir: ");
				ImGuiExt::TextVec3(point.bsdf, "bsdf: ");
				ImGuiExt::TextVec3(point.Tr, "Tr: ");
				ImGuiExt::TextVec3(point.Li, "Li: ");
			}*/
		}
	}

	void SceneEditor::shadowMappingImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Shadow Mapping", gUIPalette.iceGrey[4])) {
			ImGui::Checkbox("Render Shadow Mapping", &renderShadowMapping);
		}
	}

	void SceneEditor::renderImGUI() {
		const char* title = "Open file";
		const char* startingFolder = "./";
		const char* optionalFileExtensionFilterString = "";//".jpg;.jpeg;.png;.tiff;.bmp;.gif;.txt";
		shouldUpdateSelectedIM = false;

		//Main menu
		ImGui::BeginMainMenuBar();
		menuBarSize = ImGui::GetWindowSize();
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl + O")) {
				const char* pathptr = tinyfd_openFileDialog(title, startingFolder, 0, NULL, NULL, 0);
				if (pathptr != nullptr) {
					selectedFilePath = std::string(pathptr);
					reloadScene();
				}
			}
			if (ImGui::MenuItem("Save", "Ctrl + S")) {

			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X", false, false)) {}
			if (ImGui::MenuItem("Copy", "CTRL+C", false, false)) {}
			if (ImGui::MenuItem("Paste", "CTRL+V", false, false)) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("About")) {
			if (ImGui::MenuItem("Narval Engine Alpha. \n Developed by Igor B. Fernandes", "", false, false)) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();

		//Right column fixed menu
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 5));
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 8);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, gUIPalette.iceGrey[2]);
		ImGui::SetNextWindowPos(ImVec2(WIDTH - rightColumnMenuSize.x, menuBarSize.y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(rightColumnMenuSize.x, rightColumnMenuSize.y - menuBarSize.y), 0);
		ImGui::Begin("Right Column Menu", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoMove  | ImGuiWindowFlags_NoScrollbar);
		sceneListImGUI();
		selectedModelImGUI();
		selectedMaterialImGUI();
		if (selectedImList == 0)
			cameraImGUI();
		//TODO: not the most elegant solution, requires the camera as element 0 strictly followed by all lights in the scene
		if (selectedImList > 0 && (selectedImList - 1) < scene.lights.size())
			lightImGui();
		renderOptionsImGUI();
		performanceImGUI();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::End();

		//Secondary Menu
		ImVec4 menuBarColor = ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg);
		menuBarColor = ImVec4(menuBarColor.x - 0.35f, menuBarColor.y - 0.35f, menuBarColor.z - 0.35f,  1);
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, menuBarColor);
		//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 8));
		ImGui::SetNextWindowPos(ImVec2(0, menuBarSize.y), 0);
		ImGui::SetNextWindowSize(ImVec2(WIDTH - rightColumnMenuSize.x, menuBarSize.y));
		ImGui::Begin("Secondary Menu", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);
		ImGui::BeginMenuBar();
		menuBarSize.y = menuBarSize.y + ImGui::GetWindowSize().y;
		renderSize.y = HEIGHT - (menuBarSize.y);
		if (ImGui::MenuItem("Real Time")) {
			currentRenderingMode = REALTIME_RENDERING_MODE;
		}
		if (ImGui::MenuItem("Pathtracer")) {
			currentRenderingMode = OFFLINE_RENDERING_MODE;
			startOffEngine();
		}
		if (ImGui::MenuItem("Reload Scene")) {
			reloadScene();
		}
		if (ImGui::MenuItem("Compare Renderer")) {
			compareScene();
		}
		ImGui::EndMenuBar();
		ImGui::End();
		ImGui::PopStyleVar();
		//ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		//ImGui::ShowDemoWindow();

		//Debug window
		ImGui::SetNextWindowPos(ImVec2(2, menuBarSize.y), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(rightColumnMenuSize.x / 1.5f, 800), ImGuiCond_Once);
		ImGui::Begin("Debug", p_open, ImGuiWindowFlags_None);
		shootRayImGUI();
		shadowMappingImGUI();
		ImGui::End();

		//Log window
		ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
		ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
		ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);
		ImGui::SetNextWindowSizeConstraints(logWindowSize, ImVec2(logWindowSize.x, HEIGHT * 0.4f));
		ImGui::SetNextWindowPos(logWindowPos, ImGuiCond_Once);
		if (!logWindowOpen) {
			ImGui::SetNextWindowPos(ImVec2(0, HEIGHT - 60), ImGuiCond_Always);
		}else {
			if (!logState[0] && logState[1]) {
				logState[0] = true;
				ImGui::SetNextWindowPos(logWindowPos, ImGuiCond_Always);
			}
		}

		///std::cout << logWindowPos.y << std::endl;

		ImGui::SetNextWindowSize(logWindowSize, ImGuiCond_Once);
		if (ImGui::Begin("Log", p_open, ImGuiWindowFlags_NoMove)) {
			logState[1] = true;
			logWindowOpen = true;
			ImGui::TextUnformatted(logText.c_str(), logText.c_str() + logText.size());
		}else {
			logState[1] = false;
			logWindowOpen = false;
		}

		if (!logWindowOpen) {
			if (logState[0] && !logState[1]) {
				logState[0] = false;
				logWindowPos = ImGui::GetWindowPos();
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		//test ImGuizmo
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImVec2 pos = ImVec2((WIDTH - rightColumnMenuSize.x) / 2 - sceneFrameSize.x / 2, (HEIGHT - menuBarSize.y) / 2 + menuBarSize.y - sceneFrameSize.y / 2);
		//ImVec2 pos = ImVec2(20, 20);
		ImGui::SetNextWindowSize(ImVec2(sceneFrameSize.x, sceneFrameSize.y));
		ImGui::SetNextWindowPos(pos);
		ImGui::Begin("Gizmo", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground /*| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing*/);


		ImGuizmo::BeginFrame();
		ImGuizmo::SetDrawlist();
		float windowWidth = (float)ImGui::GetWindowWidth();
		float windowHeight = (float)ImGui::GetWindowHeight();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);


		//	ImGuizmo::DrawGrid(&((*camera.getCam())[0][0]), &proj[0][0], &glm::mat4(1)[0][0], 100.f);

			//ImGuizmo::DrawCubes(&((*camera.getCam())[0][0]), &proj[0][0], &glm::mat4(1)[0][0], 1);

		if (currentSelectedIM != nullptr) {
			glm::mat4 transf = currentSelectedIM->transformToWCS;

			if (InputManager::getSelf()->eventTriggered("ROTATE_OBJECT"))
				currentGuizmoOp = ImGuizmo::ROTATE;
			else if (InputManager::getSelf()->eventTriggered("TRANSLATE_OBJECT"))
				currentGuizmoOp = ImGuizmo::TRANSLATE;
			else if (InputManager::getSelf()->eventTriggered("SCALE_OBJECT"))
				currentGuizmoOp = ImGuizmo::SCALE;

			ImGuizmo::Manipulate(&((*camera.getCam())[0][0]), &proj[0][0], currentGuizmoOp, ImGuizmo::LOCAL, &transf[0][0], NULL, NULL, NULL, NULL);

			if (ImGuizmo::IsUsing()) {
				shouldUpdateSelectedIM = true;
				selectObjPos = getTranslation(transf);
				selectObjScale = getScale(transf);
				selectObjRotation = getRotation(transf);
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();

		/*ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
		ImVec2 ImGuizmoViewSize = ImVec2(100, 100);
		pos = ImVec2((WIDTH - rightColumnMenuSize.x) - ImGuizmoViewSize.x, menuBarSize.y);
		ImGui::SetNextWindowSize(ImGuizmoViewSize);
		ImGui::SetNextWindowPos(pos);
		ImGui::Begin("GizmoView", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		
		ImGuizmo::BeginFrame();
		windowWidth = (float)ImGui::GetWindowWidth();
		windowHeight = (float)ImGui::GetWindowHeight();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
		ImGuizmo::CamPack camPack = {0,0,0,0,0,0,0,0,0};
		//glm::mat4 camMat = (*camera.getCam());
		glm::mat4 camMat = glm::lookAt(camera.position, camera.position - camera.front, camera.up);
		//ImGuizmo::ViewManipulate(&((*camera.getCam())[0][0]), 2, pos, ImGuizmoViewSize, 0x10101010, &camPack);
		ImGuizmo::ViewManipulate(&camMat[0][0], 1, pos, ImGuizmoViewSize, 0x10101010, &camPack);
		camera.front = glm::normalize(glm::vec3(camPack.at[0], camPack.at[1], camPack.at[2]));
		camera.up = glm::vec3(camPack.up[0], camPack.up[1], camPack.up[2]);
		camera.position = glm::vec3(camPack.eye[0], camPack.eye[1], camPack.eye[2]);

		ImGui::End();
		ImGui::PopStyleColor();*/
	};

	void SceneEditor::renderOffline() {
		model = glm::mat4(1);
		model = glm::translate(model, { (WIDTH - rightColumnMenuSize.x) / 2, (HEIGHT - menuBarSize.y) / 2, 0 });
		//model = glm::scale(model, { -renderSize.x / 2, -renderSize.y/ 2, 1 });
		model = glm::scale(model, { sceneFrameSize.x / 2, sceneFrameSize.y / 2, 1 });

		renderCtx.setUniform(uniforms[0]);
		renderCtx.setUniform(uniforms[1]);
		renderCtx.setUniform(uniforms[2]);
		renderCtx.setUniform(uniforms[4]);

		renderCtx.updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(fboTexHandler, uniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.render(simpleTexProgramH);
	}

	void SceneEditor::renderShadowMappingDebug() {
		model = glm::mat4(1);
		model = glm::translate(model, { (settings.resolution.x * 0.2f)/2, (settings.resolution.y * 0.2f) + 15, 0 });
		model = glm::scale(model, { (settings.resolution.x * 0.2f) / 2, -(settings.resolution.y * 0.2f) / 2, 1 });

		renderCtx.setUniform(uniforms[0]);
		renderCtx.setUniform(uniforms[1]);
		renderCtx.setUniform(uniforms[2]);
		renderCtx.setUniform(uniforms[4]);

		renderCtx.updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		//renderCtx.setTexture(renderFrameDepthTex[0], uniforms[4]);
		renderCtx.setTexture(shadowDepthTex, uniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.render(simpleTexProgramH);
	}

	void SceneEditor::renderRealTimeShadows() {
		renderCtx.setFrameBufferClear(shadowfboh, 1.1, 0, 0, 1);

		//for each light render each object's scene from its light point of view.
		int index = 0;
		int indexpbr = 0;
		for (int i = 0; i < scene.lights.size(); i++) {
			InstancedModel* imLight = scene.lights.at(i);
			lightPos[i] = getTranslation(imLight->transformToWCS);
			glm::vec3 lightLookAt = lightPos[i];
			lightLookAt.y = lightLookAt.y - lightLookAt.y;
			glm::vec3 front = glm::vec3(0.0f, 0.0f, 0.0f);
			lightView[i] = glm::lookAt(lightPos[i], lightLookAt + front, glm::vec3(0.0f, 0.0f, 1.0f));

			renderCtx.updateUniform(shadowUniforms[1], { (uint8_t*)(&lightProjection[0][0]), sizeof(glm::mat4) });
			renderCtx.updateUniform(shadowUniforms[2], { (uint8_t*)(&lightView[i][0][0]), sizeof(glm::mat4) });
			renderCtx.updateUniform(lightUniforms[index], { (uint8_t*)&lightPos[i], sizeof(glm::vec3) });

			renderCtx.updateUniform(pbrLightUniforms[indexpbr], { (uint8_t*)&lightPos[i], sizeof(glm::vec3) });
			renderCtx.updateUniform(pbrLightUniforms[indexpbr + 1], { (uint8_t*)&imLight->model->materials.at(0)->light->li, sizeof(glm::vec3) });

			//renderCtx.updateUniform(shadowUniforms[1], { (uint8_t*)&proj[0][0], sizeof(glm::mat4) });
			//renderCtx.updateUniform(shadowUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });

			for (InstancedModel* im : scene.instancedModels) {

				ModelHandler mh = rmToRenderAPI.at(im->modelID);
				//For each mesh that this model is made of
				for (int i = 0; i < im->model->meshes.size(); i++) {

					//TODO not correct, either move a pointer inside mesh or... 
					if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
						renderCtx.updateUniform(shadowUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });

						renderCtx.setUniform(shadowUniforms[0]);
						renderCtx.setUniform(shadowUniforms[1]);
						renderCtx.setUniform(shadowUniforms[2]);
						renderCtx.setMesh(mh.meshes.at(i));

						renderCtx.setFrameBuffer(shadowfboh);

						renderCtx.render(shadowProgramHandler);
					}
				}

				if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
					ModelHandler mh = rmToRenderAPI.at(im->modelID);
					renderCtx.updateUniform(shadowUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
					
					renderCtx.setUniform(shadowUniforms[0]);
					renderCtx.setUniform(shadowUniforms[1]);
					renderCtx.setUniform(shadowUniforms[2]);
					renderCtx.setModel(rmToRenderAPI.at(im->modelID));

					renderCtx.setFrameBuffer(shadowfboh);

					renderCtx.render(shadowProgramHandler);
				}
			}

			index += phongLightsOffset;
			indexpbr += pbrLightsOffset;
		}
	}

	void SceneEditor::renderRealTimePBR() {
		renderCtx.setFrameBufferClear(fbRenderFrame[0], 0, 0, 0, 1);
		renderCtx.setFrameBufferClear(fbRenderFrame[1], 0, 0, 0, 1);
		renderCtx.setFrameBufferClear(fbRenderFrame[2], 0, 0, 0, 1);

		for (InstancedModel* im : scene.lights) {
			if (im == currentSelectedIM) {
				renderCtx.setStencil(
					NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE |
					NE_STENCIL_TEST_ALWAYS |
					NE_STENCIL_FUNC_MASK(0xff));
			}


			for (int i = 0; i < 14; i++) {
				renderCtx.setUniform(pbrUniforms[i]);
			}

			for (int i = 0; i < scene.lights.size() * pbrLightsOffset; i++) {
				renderCtx.setUniform(pbrLightUniforms[i]);
			}

			//texHandler e uniformHandler
			ModelHandler mh = rmToRenderAPI.at(im->modelID);
			renderCtx.setTexture(lightTexColorH, pbrUniforms[6]); //material.diffuse
			renderCtx.setTexture(lightTexColorH, pbrUniforms[7]); //material.metallic
			renderCtx.setTexture(lightTexColorH, pbrUniforms[8]); //material.specular
			renderCtx.setTexture(lightTexColorH, pbrUniforms[9]); //material.normal
			renderCtx.setTexture(lightTexColorH, pbrUniforms[10]); //material.roughness
			renderCtx.setTexture(lightTexColorH, pbrUniforms[11]); //material.ao

			renderCtx.updateUniform(pbrUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(glm::mat4) });
			renderCtx.updateUniform(pbrUniforms[1], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
			renderCtx.updateUniform(pbrUniforms[12], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
			renderCtx.setModel(rmToRenderAPI.at(im->modelID));

			renderCtx.setFrameBuffer(fbRenderFrame[0]);

			renderCtx.render(pbrProgramHandler);

		}

		for (InstancedModel* im : scene.instancedModels) {

			if (im == currentSelectedIM) {
				renderCtx.setStencil(
					NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE |
					NE_STENCIL_TEST_ALWAYS |
					NE_STENCIL_FUNC_MASK(0xff));
			}

			//For each mesh that this model is made of
			for (int i = 0; i < im->model->meshes.size(); i++) {

				//TODO not correct, either move a pointer inside mesh or... 
				if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
					ModelHandler mh = rmToRenderAPI.at(im->modelID);
					if (mh.meshes.at(i).material.id == INVALID_HANDLE)
						continue;

					for (int i = 0; i < 14; i++) {
						renderCtx.setUniform(pbrUniforms[i]);
					}

					for (int i = 0; i < scene.lights.size() * pbrLightsOffset; i++) {
						renderCtx.setUniform(pbrLightUniforms[i]);
					}

					//texHandler e uniformHandler
					if(isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::ALBEDO)].id))
						renderCtx.setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::ALBEDO)], pbrUniforms[6]); //material.diffuse
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::METALLIC)].id))
						renderCtx.setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::METALLIC)], pbrUniforms[7]); //material.metallic
					//renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[8]); //material.specular
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::NORMAL_MAP)].id))
						renderCtx.setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::NORMAL_MAP)], pbrUniforms[9]); //material.normal
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::ROUGHNESS)].id))
						renderCtx.setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::ROUGHNESS)], pbrUniforms[10]); //material.roughness
					//renderCtx.setTexture(mh.meshes.at(i).material.textures[TextureName::AO], pbrUniforms[11]); //material.ao
					
					/*renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[6]); //material.diffuse
					renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[7]); //material.metallic
					//renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[8]); //material.specular
					renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[9]); //material.normal
					renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[10]); //material.roughness
					renderCtx.setTexture(mh.meshes.at(i).textures.at(0), pbrUniforms[11]); //material.ao*/
					
					renderCtx.setTexture(shadowDepthTex, pbrUniforms[13]); //shadowTex

					renderCtx.updateUniform(pbrUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
					renderCtx.updateUniform(pbrUniforms[1], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
					renderCtx.updateUniform(pbrUniforms[12], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });

					renderCtx.setMesh(mh.meshes.at(i));

					renderCtx.setFrameBuffer(fbRenderFrame[0]);

					renderCtx.render(pbrProgramHandler);
				}
			}

			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
				for (int i = 0; i < 28; i++)
					renderCtx.setUniform(volUniforms[i]);

				//texHandler e uniformHandler
				ModelHandler mh = rmToRenderAPI.at(im->modelID);

				if (isHandleValid(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)].id))
					renderCtx.setTexture(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)], volUniforms[4]); //volume
				//if (mh.meshes.at(0).textures.size() > 0)
				//	renderCtx.setTexture(mh.meshes.at(0).textures.at(0), volUniforms[4]); //volume

				renderCtx.setTexture(renderFrameTex[0], volUniforms[5]);
				renderCtx.setTexture(renderFrameDepthTex[0], volUniforms[6]);
				renderCtx.setTexture(shadowDepthTex, phongUniforms[10]); //shadowTex

				renderCtx.updateUniform(volUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
				renderCtx.updateUniform(volUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
				renderCtx.updateUniform(volUniforms[3], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
				renderCtx.updateUniform(volUniforms[27], { (uint8_t*)&im->invTransformToWCS[0][0], sizeof(glm::mat4) });
				renderCtx.setModel(rmToRenderAPI.at(im->modelID));

				renderCtx.setFrameBuffer(fbRenderFrame[0]);

				renderCtx.render(volProgramHandler);
			}
		}

		if (currentSelectedIM != nullptr && currentSelectedIM->model->primitives.size() > 0) {
			glDisable(GL_DEPTH_TEST); //TODO NOT EXPOSE GL HERE. TEMPORARY
			renderCtx.setStencil(
				NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE |
				NE_STENCIL_TEST_NOTEQUAL |
				NE_STENCIL_FUNC_MASK(0x00));

			//Render outline
			for (int i = 0; i < 4; i++)
				renderCtx.setUniform(monoColorUniforms[i]);

			glm::mat4 scaled = glm::scale(currentSelectedIM->transformToWCS, glm::vec3(1.1));

			renderCtx.updateUniform(monoColorUniforms[0], { (uint8_t*)(&scaled), sizeof(glm::mat4) });
			renderCtx.updateUniform(monoColorUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });

			renderCtx.setModel(rmToRenderAPI.at(currentSelectedIM->modelID));
			renderCtx.setFrameBuffer(fbRenderFrame[0]);
			renderCtx.render(monoColorProgramH);
			glEnable(GL_DEPTH_TEST);
		}

		//Render shooting Ray
		if (!intersectionPoints[0].empty()) {
			glDisable(GL_DEPTH_TEST); //TODO NOT EXPOSE GL HERE. TEMPORARY
			for (int k = 0; k < numberOfShootedRays; k++) {
				for (int i = 0; i < 13; i++) {
					renderCtx.setUniform(phongUniforms[i]);
				}
				for (int i = 0; i < scene.lights.size() * phongLightsOffset; i++) {
					renderCtx.setUniform(lightUniforms[i]);
				}
				renderCtx.setTexture(rayTextureColorH, phongUniforms[6]); //material.diffuse

				glm::mat4 identity = glm::mat4(1);
				renderCtx.updateUniform(phongUniforms[0], { (uint8_t*)(&identity), sizeof(glm::mat4) });
				renderCtx.updateUniform(phongUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
				renderCtx.updateUniform(phongUniforms[5], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
				renderCtx.setVertexBuffer(rayVertexBufferH[k]);
				renderCtx.setIndexBuffer(rayIndexBufferH[k]);
				renderCtx.setState(NE_STATE_PRIMITIVE_TYPE_LINES);
				//set texture
				renderCtx.setFrameBuffer(fbRenderFrame[0]);

				renderCtx.render(phongProgramHandler);
			}
			glEnable(GL_DEPTH_TEST);
		}

		//Display final result
		model = glm::mat4(1);
		model = glm::translate(model, { (WIDTH - rightColumnMenuSize.x) / 2, (HEIGHT - menuBarSize.y) / 2, 0 });
		model = glm::scale(model, { sceneFrameSize.x / 2, -sceneFrameSize.y / 2, 1 });

		renderCtx.setUniform(uniforms[0]);
		renderCtx.setUniform(uniforms[1]);
		renderCtx.setUniform(uniforms[2]);
		renderCtx.setUniform(uniforms[4]);

		renderCtx.updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(renderFrameTex[0], uniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.render(simpleTexProgramH);

		if (renderShadowMapping)
			renderShadowMappingDebug();
	}

	void SceneEditor::renderRealTime() {
		renderCtx.setFrameBufferClear(fbRenderFrame[0], 0, 0, 0, 1);
		renderCtx.setFrameBufferClear(fbRenderFrame[1], 0, 0, 0, 1);
		renderCtx.setFrameBufferClear(fbRenderFrame[2], 0, 0, 0, 1);
		for (InstancedModel* im : scene.lights) {
			if (im == currentSelectedIM) {
				renderCtx.setStencil(
					NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE |
					NE_STENCIL_TEST_ALWAYS |
					NE_STENCIL_FUNC_MASK(0xff));
			}


			for (int i = 0; i < 13; i++) {
				renderCtx.setUniform(phongUniforms[i]);
			}

			for (int i = 0; i < scene.lights.size() * phongLightsOffset; i++) {
				renderCtx.setUniform(lightUniforms[i]);
			}

			//texHandler e uniformHandler
			ModelHandler mh = rmToRenderAPI.at(im->modelID);
			renderCtx.setTexture(lightTexColorH, phongUniforms[6]); //material.diffuse

			//glm::mat4 test = glm::scale(im->transformToWCS, glm::vec3(0.5f));
			//renderCtx.updateUniform(phongUniforms[0], { (uint8_t*)(&test), sizeof(im->transformToWCS) });
			renderCtx.updateUniform(phongUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
			renderCtx.updateUniform(phongUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
			renderCtx.updateUniform(phongUniforms[5], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
			renderCtx.setModel(rmToRenderAPI.at(im->modelID));

			renderCtx.setFrameBuffer(fbRenderFrame[0]);

			renderCtx.render(phongProgramHandler);
			
		}
		/*
		for (InstancedModel *im : scene.instancedModels) {

			if (im == currentSelectedIM){
				renderCtx.setStencil(
					NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE | 
					NE_STENCIL_TEST_ALWAYS | 
					NE_STENCIL_FUNC_MASK(0xff));
			}

			//For each mesh that this model is made of
			for (int i = 0; i < im->model->meshes.size(); i++) {

				//TODO not correct, either move a pointer inside mesh or... 
				if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
					ModelHandler mh = rmToRenderAPI.at(im->modelID);
					if (mh.meshes.at(i).textures.size() == 0)
						continue;

					for (int i = 0; i < 13; i++) {
						renderCtx.setUniform(phongUniforms[i]);
					}

					for (int i = 0; i < scene.lights.size() * phongLightsOffset; i++) {
						renderCtx.setUniform(lightUniforms[i]);
					}

					renderCtx.setTexture(mh.meshes.at(i).textures.at(0), phongUniforms[6]); //material.diffuse
					renderCtx.setTexture(shadowDepthTex, phongUniforms[10]); //shadowTex

					//glm::mat4 test = glm::scale(im->transformToWCS, glm::vec3(0.5f));
					//renderCtx.updateUniform(phongUniforms[0], { (uint8_t*)(&test), sizeof(im->transformToWCS) });
					renderCtx.updateUniform(phongUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
					renderCtx.updateUniform(phongUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
					renderCtx.updateUniform(phongUniforms[5], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
					renderCtx.setMesh(mh.meshes.at(i));

					renderCtx.setFrameBuffer(fbRenderFrame[0]);

					renderCtx.render(phongProgramHandler);
				}
			}

			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
				for (int i = 0; i < 28; i++)
					renderCtx.setUniform(volUniforms[i]);

				//texHandler e uniformHandler
				ModelHandler mh = rmToRenderAPI.at(im->modelID);
				if(mh.meshes.at(0).textures.size() > 0)
					renderCtx.setTexture(mh.meshes.at(0).textures.at(0), volUniforms[4]); //volume
				renderCtx.setTexture(renderFrameTex[0], volUniforms[5]);
				renderCtx.setTexture(renderFrameDepthTex[0], volUniforms[6]);
				renderCtx.setTexture(shadowDepthTex, phongUniforms[10]); //shadowTex

				renderCtx.updateUniform(volUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
				renderCtx.updateUniform(volUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
				renderCtx.updateUniform(volUniforms[3], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
				renderCtx.updateUniform(volUniforms[27], { (uint8_t*)&im->invTransformToWCS[0][0], sizeof(glm::mat4) });
				renderCtx.setModel(rmToRenderAPI.at(im->modelID));

				renderCtx.setFrameBuffer(fbRenderFrame[0]);

				renderCtx.render(volProgramHandler);
			}
		}*/

		if (currentSelectedIM != nullptr && currentSelectedIM->model->primitives.size() > 0) {
			glDisable(GL_DEPTH_TEST); //TODO NOT EXPOSE GL HERE. TEMPORARY
			renderCtx.setStencil(
				NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE |
				NE_STENCIL_TEST_NOTEQUAL |
				NE_STENCIL_FUNC_MASK(0x00));

			//Render outline
			for (int i = 0; i < 4; i++) 
				renderCtx.setUniform(monoColorUniforms[i]);

			glm::mat4 scaled = glm::scale(currentSelectedIM->transformToWCS, glm::vec3(1.1 ));

			renderCtx.updateUniform(monoColorUniforms[0], { (uint8_t*)(&scaled), sizeof(glm::mat4) });
			renderCtx.updateUniform(monoColorUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });

			renderCtx.setModel(rmToRenderAPI.at(currentSelectedIM->modelID));
			renderCtx.setFrameBuffer(fbRenderFrame[0]);
			renderCtx.render(monoColorProgramH);
			glEnable(GL_DEPTH_TEST);
		}

		//Render shooting Ray
		if (!intersectionPoints[0].empty()) {
			glDisable(GL_DEPTH_TEST); //TODO NOT EXPOSE GL HERE. TEMPORARY
			for (int k = 0; k < numberOfShootedRays; k++) {
				for (int i = 0; i < 13; i++) {
					renderCtx.setUniform(phongUniforms[i]);
				}
				for (int i = 0; i < scene.lights.size() * phongLightsOffset; i++) {
					renderCtx.setUniform(lightUniforms[i]);
				}
				renderCtx.setTexture(rayTextureColorH, phongUniforms[6]); //material.diffuse

				glm::mat4 identity = glm::mat4(1);
				renderCtx.updateUniform(phongUniforms[0], { (uint8_t*)(&identity), sizeof(glm::mat4) });
				renderCtx.updateUniform(phongUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
				renderCtx.updateUniform(phongUniforms[5], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
				renderCtx.setVertexBuffer(rayVertexBufferH[k]);
				renderCtx.setIndexBuffer(rayIndexBufferH[k]);
				renderCtx.setState(NE_STATE_PRIMITIVE_TYPE_LINES);
				//set texture
				renderCtx.setFrameBuffer(fbRenderFrame[0]);

				renderCtx.render(phongProgramHandler);
			}
			glEnable(GL_DEPTH_TEST);
		}

		/*model = glm::mat4(1);
		model = glm::translate(model, { (WIDTH - rightColumnMenuSize.x) / 2, (HEIGHT - menuBarSize.y) / 2, 0 });
		model = glm::scale(model, { -sceneFrameSize.x / 2, -sceneFrameSize.y / 2, 1 });

		renderCtx.setUniform(composeTexUniforms[0]);
		renderCtx.setUniform(composeTexUniforms[1]);
		renderCtx.setUniform(composeTexUniforms[2]);
		renderCtx.setUniform(composeTexUniforms[3]);
		renderCtx.setUniform(composeTexUniforms[4]);
		renderCtx.setUniform(composeTexUniforms[5]);
		renderCtx.setUniform(composeTexUniforms[6]);

		renderCtx.updateUniform(composeTexUniforms[4], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(composeTexUniforms[5], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(composeTexUniforms[6], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(renderFrameTex[0], composeTexUniforms[0]);
		renderCtx.setTexture(renderFrameTex[1], composeTexUniforms[1]);
		renderCtx.setTexture(renderFrameDepthTex[0], composeTexUniforms[2]);
		renderCtx.setTexture(renderFrameDepthTex[1], composeTexUniforms[3]);

		renderCtx.setModel(quadModelHandler);

		renderCtx.render(composeTexProgramH);*/

		//Display final result
		model = glm::mat4(1);
		model = glm::translate(model, { (WIDTH - rightColumnMenuSize.x) / 2, (HEIGHT - menuBarSize.y) / 2, 0 });
		model = glm::scale(model, { sceneFrameSize.x / 2, -sceneFrameSize.y / 2, 1 });

		renderCtx.setUniform(uniforms[0]);
		renderCtx.setUniform(uniforms[1]);
		renderCtx.setUniform(uniforms[2]);
		renderCtx.setUniform(uniforms[4]);

		renderCtx.updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(renderFrameTex[0], uniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.render(simpleTexProgramH);

		if (renderShadowMapping)
			renderShadowMappingDebug();
	}

	void SceneEditor::renderCompare() {

		glm::vec2 viewSize = glm::vec2(WIDTH - rightColumnMenuSize.x, 0);
		viewSize.x = viewSize.x / 2;
		viewSize.y = sceneFrameSize.y;

		glm::vec2 totalView = glm::vec2((WIDTH - rightColumnMenuSize.x), HEIGHT - menuBarSize.y);

		//view 1 - External Selected image
		model = glm::mat4(1);
		//(x,y) = (0,0) is the bottom left corner
		model = glm::translate(model, { viewSize.x / 2, viewSize.y / 2 + totalView.y / 2, 0 });
		model = glm::scale(model, { viewSize.x / 2, viewSize.y / 2, 1 });

		renderCtx.setUniform(uniforms[0]);
		renderCtx.setUniform(uniforms[1]);
		renderCtx.setUniform(uniforms[2]);
		renderCtx.setUniform(uniforms[4]);

		renderCtx.updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(compareTex, uniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.setClear(NE_CLEAR_ALL, 0, 0, 0, 1);

		renderCtx.render(simpleTexProgramH);

		//view 2 - Narval Path traced image
		model = glm::mat4(1);
		model = glm::translate(model, { viewSize.x / 2 + viewSize.x, viewSize.y / 2 + totalView.y / 2, 0 });
		model = glm::scale(model, { viewSize.x / 2, viewSize.y / 2, 1 });

		renderCtx.setUniform(uniforms[0]);
		renderCtx.setUniform(uniforms[1]);
		renderCtx.setUniform(uniforms[2]);
		renderCtx.setUniform(uniforms[4]);

		renderCtx.updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(fboTexHandler, uniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.render(simpleTexProgramH);

		//view 3 - Difference between both
		model = glm::mat4(1);
		model = glm::translate(model, {  totalView.x / 2, viewSize.y / 2 + 60, 0 });
		model = glm::scale(model, { viewSize.x / 2, viewSize.y / 2, 1 });

		renderCtx.setUniform(imgDiffUniforms[0]);
		renderCtx.setUniform(imgDiffUniforms[1]);
		renderCtx.setUniform(imgDiffUniforms[2]);
		renderCtx.setUniform(imgDiffUniforms[3]);
		renderCtx.setUniform(imgDiffUniforms[4]);

		renderCtx.updateUniform(imgDiffUniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx.updateUniform(imgDiffUniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx.updateUniform(imgDiffUniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx.setTexture(compareTex, imgDiffUniforms[3]);
		renderCtx.setTexture(fboTexHandler, imgDiffUniforms[4]);
		renderCtx.setModel(quadModelHandler);

		renderCtx.render(imgDiffProgramH);
	}

	void SceneEditor::render() {
		if (currentRenderingMode == OFFLINE_RENDERING_MODE) {
			renderOffline();
		}
		else if (currentRenderingMode == REALTIME_RENDERING_MODE) {
			renderRealTimeShadows();
			//renderRealTime();
			renderRealTimePBR();
		}
		else if (currentRenderingMode == COMPARE_RENDERING_MODE) {
			renderCompare();
		}

		renderImGUI();
	};
}