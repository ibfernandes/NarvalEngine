#include "SceneEditor.h"
#include "tinyfiledialogs.h"
#include "tinyexr.h"

namespace narvalengine {

	SceneEditor::SceneEditor() {

	};

	void SceneEditor::init() {
		int width = Engine3D::getSelf()->settings.resolution.x;
		int height = Engine3D::getSelf()->settings.resolution.y;
		this->WIDTH = width;
		this->HEIGHT = height;
		aspectRatio = (float)width / height;
		rightColumnMenuSize = ImVec2(WIDTH * 0.27f, HEIGHT);
		logWindowSize = ImVec2(WIDTH - rightColumnMenuSize.x, HEIGHT * 0.3f);
		logWindowPos = ImVec2(0, HEIGHT - logWindowSize.y);
		renderSize = glm::vec2(WIDTH - rightColumnMenuSize.x, HEIGHT);

		renderCtx = &Engine3D::getSelf()->renderer;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
		invProj = glm::inverse(proj);
		orthoProj = glm::ortho(0.f, -(float)width, 0.f, (float)height, -1.f, 1.f);

		glm::vec3 position(0, 0, 0);
		glm::vec3 front = { 0.0f, 0.0f, 1.0f };
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 side = glm::cross(front, up);
		staticCam = glm::lookAt(position, position + front, up);

		selectedFilePath = "scenes/cornellbox-pbrt.json";
		sceneReader.loadScene(selectedFilePath, false);
		scene = sceneReader.getScene();
		camera = sceneReader.getMainCamera();
		settings = sceneReader.getSettings();

		generateSceneImList();

		sceneFrameSize = settings.resolution;
		offEngine = new OfflineEngine(camera, settings, scene);

		cam = glm::lookAt(*camera.getPosition(), *camera.getPosition() + front, up);
		cameraPosition = *camera.getPosition();
		cameraPositionCache = cameraPosition;

		startTime = glfwGetTime();
		for (int i = MAX_GRAPH_POINTS - 1; i > 0; i--)
			FPSgraphPoints[i] = 0;

		ResourceManager::getSelf()->loadModel("quad", "models/", "quad.obj");
		ResourceManager::getSelf()->loadModel("quad", "models/", "quad.obj");
		ResourceManager::getSelf()->loadModel("cube", "models/", "cube.obj");
		ResourceManager::getSelf()->loadTexture("default", "imgs/checkboard.png");
		ResourceManager::getSelf()->loadTexture("defaultAlt", "imgs/checkboard2.png");
		ResourceManager::getSelf()->loadShader("volumeMonteCarlo", "shaders/volumeMonteCarlo.vert", "shaders/volumeMonteCarlo.frag", "");
		ResourceManager::getSelf()->loadShader("imageDifference", "shaders/imageDifference.vert", "shaders/imageDifference.frag", "");
		ResourceManager::getSelf()->loadShader("monocolor", "shaders/monoColor.vert", "shaders/monoColor.frag", "");
		ResourceManager::getSelf()->loadShader("randomVisualizer", "shaders/random.vert", "shaders/random.frag", "");
		ResourceManager::getSelf()->loadShader("billboard", "shaders/billboard.vert", "shaders/billboard.frag", "");
		ResourceManager::getSelf()->loadShader("phong", "shaders/phong.vert", "shaders/phong.frag", "");
		ResourceManager::getSelf()->loadShader("pbr", "shaders/pbr.vert", "shaders/pbr.frag", "");
		ResourceManager::getSelf()->loadShader("cloudscape", "shaders/cloudscape.vert", "shaders/cloudscape.frag", "");
		ResourceManager::getSelf()->loadShader("visibility", "shaders/visibility.vert", "shaders/visibility.frag", "");
		ResourceManager::getSelf()->loadShader("shvolume", "shaders/shvolume.vert", "shaders/shvolume.frag", "");
		ResourceManager::getSelf()->loadShader("volume", "shaders/volume.vert", "shaders/volume.frag", "");
		ResourceManager::getSelf()->loadShader("volumewcs", "shaders/volumeWCS.vert", "shaders/volumeWCS.frag", "");
		ResourceManager::getSelf()->loadShader("simpleTexture", "shaders/simpleTexture.vert", "shaders/simpleTexture.frag", "");
		ResourceManager::getSelf()->loadShader("gamma", "shaders/gammaCorrection.vert", "shaders/gammaCorrection.frag", "");
		ResourceManager::getSelf()->loadShader("pathTracingLastPass", "shaders/pathTracingLastPass.vert", "shaders/pathTracingLastPass.frag", "");
		ResourceManager::getSelf()->loadTexture("cloudheights", "imgs/heights.png");
		ResourceManager::getSelf()->loadTexture("weather", "imgs/weather.png");
		ResourceManager::getSelf()->loadTexture("lightbulb", "imgs/light-bulb.png");
		ResourceManager::getSelf()->loadShader("screentex", "shaders/screenTex.vert", "shaders/screenTex.frag", "");
		ResourceManager::getSelf()->loadShader("gradientBackground", "shaders/gradientBackground.vert", "shaders/gradientBackground.frag", "");
		ResourceManager::getSelf()->loadShader("volumelbvh", "shaders/volumeLBVH.vert", "shaders/volumeLBVH.frag", "");
		ResourceManager::getSelf()->loadShader("simpleLightDepth", "shaders/simpleLightDepth.vert", "shaders/simpleLightDepth.frag", "");
		ResourceManager::getSelf()->loadShader("composeTex", "shaders/composeTex.vert", "shaders/composeTex.frag", "");

		ResourceManager::getSelf()->loadShader("MS_rayMarching", "shaders/MS_rayMarching.vert", "shaders/MS_rayMarching.frag", "");
		ResourceManager::getSelf()->loadShader("MS_lobeSampling", "shaders/MS_lobeSampling.vert", "shaders/MS_lobeSampling.frag", "");
		ResourceManager::getSelf()->loadShader("MS_monteCarlo", "shaders/MS_monteCarlo.vert", "shaders/MS_monteCarlo.frag", "");
		ResourceManager::getSelf()->loadShader("postProcessing", "shaders/postProcessing.vert", "shaders/postProcessing.frag", "");

		load();
	}

	void SceneEditor::initVolumetricShader() {
		Shader *shader = ResourceManager::getSelf()->getShader("volumelbvh");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		volProgramHandler = renderCtx->createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		volUniforms[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		volUniforms[1] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		volUniforms[2] = renderCtx->createUniform("cam", mb3, UniformType::Mat4, 0);
		volUniforms[3] = renderCtx->createUniform("cameraPosition", {(uint8_t*)&cameraPosition[0], sizeof(cameraPosition)}, UniformType::Vec3, 0);
		volUniforms[4] = renderCtx->createUniform("volume", {(uint8_t*)&volumeTexBind, sizeof(volumeTexBind)}, UniformType::Sampler, 0);
		volUniforms[5] = renderCtx->createUniform("background", { (uint8_t*)&bgTexBind, sizeof(bgTexBind) }, UniformType::Sampler, 0);
		volUniforms[6] = renderCtx->createUniform("backgroundDepth", { (uint8_t*)&bgDepthTexBind, sizeof(bgDepthTexBind) }, UniformType::Sampler, 0);
		volUniforms[7] = renderCtx->createUniform("previousCloud", { (uint8_t*)&prevFrameTexBind, sizeof(prevFrameTexBind) }, UniformType::Sampler, 0);
		volUniforms[8] = renderCtx->createUniform("time", { (uint8_t*)&time, sizeof(time) }, UniformType::Float, 0);
		volUniforms[9] = renderCtx->createUniform("screenRes", { (uint8_t*)&sceneFrameSize[0], sizeof(sceneFrameSize) }, UniformType::Vec2, 0);
		volUniforms[10] = renderCtx->createUniform("renderingMode", { (uint8_t*)&vms->currentMethod, sizeof(int) }, UniformType::Int, 0);
		volUniforms[11] = renderCtx->createUniform("scattering", { (uint8_t*)&scattering[0], sizeof(scattering) }, UniformType::Vec3, 0);
		volUniforms[12] = renderCtx->createUniform("absorption", { (uint8_t*)&absorption[0], sizeof(absorption) }, UniformType::Vec3, 0);
		volUniforms[13] = renderCtx->createUniform("densityCoef", { (uint8_t*)&densityCoef, sizeof(densityCoef) }, UniformType::Float, 0);
		volUniforms[14] = renderCtx->createUniform("numberOfSteps", { (uint8_t*)&numberOfSteps, sizeof(numberOfSteps) }, UniformType::Float, 0);
		volUniforms[15] = renderCtx->createUniform("shadowSteps", { (uint8_t*)&shadowSteps, sizeof(shadowSteps) }, UniformType::Float, 0);
		//TODO changed to struct Light!
		volUniforms[16] = renderCtx->createUniform(std::string("lightPoints[0].position").c_str(), { (uint8_t*)&lightPosition, sizeof(lightPosition) }, UniformType::Vec3, 0);
		volUniforms[17] = renderCtx->createUniform(std::string("lightPoints[0].Li").c_str(), { (uint8_t*)&lightColor, sizeof(glm::vec3) }, UniformType::Vec3, 0);
		//volUniforms[16] = renderCtx->createUniform("lightPosition", { (uint8_t*)&lightPosition[0], sizeof(lightPosition) }, UniformType::Vec3, 0);
		//volUniforms[17] = renderCtx->createUniform("lightColor", { (uint8_t*)&lightColor[0], sizeof(lightColor) }, UniformType::Vec3, 0);
		volUniforms[18] = renderCtx->createUniform("ambientStrength", { (uint8_t*)&ambientStrength, sizeof(ambientStrength) }, UniformType::Float, 0);
		volUniforms[19] = renderCtx->createUniform("Kc", {(uint8_t*)&Kc, sizeof(Kc) }, UniformType::Float, 0);
		volUniforms[20] = renderCtx->createUniform("Kl", { (uint8_t*)&Kl, sizeof(Kl) }, UniformType::Float, 0);
		volUniforms[21] = renderCtx->createUniform("Kq", { (uint8_t*)&Kq, sizeof(Kq) }, UniformType::Float, 0);
		volUniforms[22] = renderCtx->createUniform("phaseFunctionOption", { (uint8_t*)&phaseFunctionOption, sizeof(phaseFunctionOption) }, UniformType::Float, 0);
		volUniforms[23] = renderCtx->createUniform("g", { (uint8_t*)&g, sizeof(g) }, UniformType::Float, 0);
		volUniforms[24] = renderCtx->createUniform("SPP", { (uint8_t*)&settings.spp, sizeof(settings.spp) }, UniformType::Int, 0);
		volUniforms[25] = renderCtx->createUniform("frameCount", { (uint8_t*)&frameCount, sizeof(frameCount) }, UniformType::Int, 0);
		volUniforms[26] = renderCtx->createUniform("enableShadow", { (uint8_t*)&enableShadow, sizeof(enableShadow) }, UniformType::Float, 0);
		volUniforms[27] = renderCtx->createUniform("invmodel", mb1, UniformType::Mat4, 0);
		volUniforms[28] = renderCtx->createUniform("invMaxDensity", { (uint8_t*)&invMaxDensity, sizeof(invMaxDensity) }, UniformType::Float, 0);
		volUniforms[29] = renderCtx->createUniform("densityMc", { (uint8_t*)&densityMc, sizeof(densityMc) }, UniformType::Float, 0);
		volUniforms[30] = renderCtx->createUniform(std::string("lightPoints[0].minVertex").c_str(), { (uint8_t*)&volLightRecMin, sizeof(glm::vec3) }, UniformType::Vec3, 0);
		volUniforms[31] = renderCtx->createUniform(std::string("lightPoints[0].maxVertex").c_str(), { (uint8_t*)&volLightRecMax, sizeof(glm::vec3) }, UniformType::Vec3, 0);
		volUniforms[32] = renderCtx->createUniform(std::string("lightPoints[0].size").c_str(), { (uint8_t*)&volLightRecSize, sizeof(glm::vec3) }, UniformType::Vec3, 0);
		volUniforms[33] = renderCtx->createUniform(std::string("lightPoints[0].transformWCS").c_str(), { (uint8_t*)&volLightWCS, sizeof(glm::mat4) }, UniformType::Mat4, 0);
		volUniforms[34] = renderCtx->createUniform("maxBounces", { (uint8_t*)&maxBounces, sizeof(int) }, UniformType::Int, 0);
		volUniforms[35] = renderCtx->createUniform("useFactor", { (uint8_t*)&useFactor, sizeof(int) }, UniformType::Int, 0);
		volUniforms[36] = renderCtx->createUniform(std::string("lightPoints[0].type").c_str(), { (uint8_t*)&volLightType, sizeof(int) }, UniformType::Int, 0);
		volUniforms[37] = renderCtx->createUniform("infAreaLight", { (uint8_t*)&infAreaLightTexBind, sizeof(infAreaLightTexBind) }, UniformType::Sampler, 0);
		volUniforms[38] = renderCtx->createUniform("stepSize", { (uint8_t*)&newStepSize, sizeof(newStepSize) }, UniformType::Float, 0);
		volUniforms[39] = renderCtx->createUniform("newNumberOfSteps", { (uint8_t*)&newNumberOfSteps, sizeof(newNumberOfSteps) }, UniformType::Int, 0);
		volUniforms[40] = renderCtx->createUniform("nPointsToSample", { (uint8_t*)&newnPointsToSample, sizeof(newnPointsToSample) }, UniformType::Int, 0);
		volUniforms[41] = renderCtx->createUniform("newMeanPathMult", { (uint8_t*)&newMeanPathMult, sizeof(newMeanPathMult) }, UniformType::Int, 0);
		volUniforms[42] = renderCtx->createUniform("param_1", { (uint8_t*)&param_1, sizeof(param_1) }, UniformType::Float, 0);

		//init shader LBVH
	}

	void SceneEditor::initPBR() {
		Shader* shader = ResourceManager::getSelf()->getShader("pbr");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		pbrProgramHandler = renderCtx->createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&cam[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb4 = { (uint8_t*)(&lightView[0][0][0]), sizeof(model) };
		MemoryBuffer mb5 = { (uint8_t*)(&lightProjection[0][0]), sizeof(model) };

		//VS uniforms
		pbrUniforms[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		pbrUniforms[1] = renderCtx->createUniform("cam", mb2, UniformType::Mat4, 0);
		pbrUniforms[2] = renderCtx->createUniform("proj", mb3, UniformType::Mat4, 0);
		pbrUniforms[3] = renderCtx->createUniform("lightCam", mb4, UniformType::Mat4, 0);
		pbrUniforms[4] = renderCtx->createUniform("lightProj", mb5, UniformType::Mat4, 0);

		//FS uniforms
		numberOfActiveLights = scene->lights.size();
		pbrUniforms[5] = renderCtx->createUniform("numberOfLights", { (uint8_t*)&numberOfActiveLights, sizeof(numberOfActiveLights) }, UniformType::Int, 0);
		int index = 0;
		for (int i = 0; i < scene->lights.size(); i++) {
			std::string shaderIndex = std::to_string(index / pbrLightsOffset);
			std::string s = "lightPoints[" + shaderIndex + "].position";

			pbrLightUniforms[index] = renderCtx->createUniform(std::string("lightPoints[" + shaderIndex + "].position").c_str(), { (uint8_t*)&lightPosition, sizeof(lightPosition) }, UniformType::Vec3, 0);
			pbrLightUniforms[index + 1] = renderCtx->createUniform(std::string("lightPoints[" + shaderIndex + "].color").c_str(), { (uint8_t*)&lightColor, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			index += pbrLightsOffset;
		}

		
		pbrUniforms[6] = renderCtx->createUniform("material.diffuse", { &texIds[0], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[7] = renderCtx->createUniform("material.metallic", { &texIds[1], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[8] = renderCtx->createUniform("material.specular", { &texIds[2], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[9] = renderCtx->createUniform("material.normal", { &texIds[3], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[10] = renderCtx->createUniform("material.roughness", { &texIds[4], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[11] = renderCtx->createUniform("material.ao", { &texIds[5], sizeof(int) }, UniformType::Sampler, 0);
		pbrUniforms[12] = renderCtx->createUniform("cameraPosition", { &cameraPosition[0], sizeof(cameraPosition) }, UniformType::Vec3, 0);
		pbrUniforms[13] = renderCtx->createUniform("shadowMap", { &texIds[6], sizeof(int) }, UniformType::Sampler, 0);

		pbrUniforms[14] = renderCtx->createUniform("materialIsSet.diffuse", { &isMaterialSet[0], sizeof(bool) }, UniformType::Bool, 0);
		pbrUniforms[15] = renderCtx->createUniform("materialIsSet.metallic", { &isMaterialSet[1], sizeof(bool) }, UniformType::Bool, 0);
		pbrUniforms[16] = renderCtx->createUniform("materialIsSet.specular", { &isMaterialSet[2], sizeof(bool) }, UniformType::Bool, 0);
		pbrUniforms[17] = renderCtx->createUniform("materialIsSet.normal", { &isMaterialSet[3], sizeof(bool) }, UniformType::Bool, 0);
		pbrUniforms[18] = renderCtx->createUniform("materialIsSet.roughness", { &isMaterialSet[4], sizeof(bool) }, UniformType::Bool, 0);
		pbrUniforms[19] = renderCtx->createUniform("materialIsSet.ao", { &isMaterialSet[5], sizeof(bool) }, UniformType::Bool, 0);

	}
	
	void SceneEditor::initComposeShader() {

		Shader *shader = ResourceManager::getSelf()->getShader("composeTex");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		composeTexProgramH = renderCtx->createProgram(shvertex, shfragment);

		composeTexUniforms[0] = renderCtx->createUniform("tex1", { (uint8_t*)&tex0, sizeof(tex0) }, UniformType::Sampler, 0);
		composeTexUniforms[1] = renderCtx->createUniform("tex2", { (uint8_t*)&tex1, sizeof(tex1) }, UniformType::Sampler, 0);
		composeTexUniforms[2] = renderCtx->createUniform("depth1", { (uint8_t*)&tex2, sizeof(tex2) }, UniformType::Sampler, 0);
		composeTexUniforms[3] = renderCtx->createUniform("depth2", { (uint8_t*)&tex3, sizeof(tex3) }, UniformType::Sampler, 0);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&orthoProj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&staticCam[0][0]), sizeof(model) };

		composeTexUniforms[4] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		composeTexUniforms[5] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		composeTexUniforms[6] = renderCtx->createUniform("cam", mb3, UniformType::Mat4, 0);
	}

	void SceneEditor::initPostProcessingShader() {

		Shader* shader = ResourceManager::getSelf()->getShader("postProcessing");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		postProcessingPH = renderCtx->createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&orthoProj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&staticCam[0][0]), sizeof(model) };

		postProcessingUni[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		postProcessingUni[1] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		postProcessingUni[2] = renderCtx->createUniform("cam", mb3, UniformType::Mat4, 0);

		postProcessingUni[3] = renderCtx->createUniform("tex", { (uint8_t*)&tex0, sizeof(tex0) }, UniformType::Sampler, 0);
		postProcessingUni[4] = renderCtx->createUniform("mode", { (uint8_t*)&tex0, sizeof(int) }, UniformType::Int, 0);
		postProcessingUni[5] = renderCtx->createUniform("gamma", { (uint8_t*)&gamma, sizeof(float) }, UniformType::Float, 0);
		postProcessingUni[6] = renderCtx->createUniform("exposure", { (uint8_t*)&exposure, sizeof(float) }, UniformType::Float, 0);
	}

	void SceneEditor::initRenderAPI() {
		vms = new VMS(renderCtx, scene);
		vms->mcSPP = settings.spp;
		vms->mcMaxBounces = settings.bounces;

		initVolumetricShader();
		initShadowShader();
		initPBR();
		initPostProcessingShader();

		//Init Default checkboardTex
		defaultTex = renderCtx->createTexture(1, 1, 0, TextureLayout::RGBA32F, { (uint8_t*)&defaultTexColor[0], sizeof(glm::vec4) }, NE_TEX_SAMPLER_MIN_MAG_NEAREST | NE_TEX_SAMPLER_UVW_MIRROR);

		//Init rayShoot debugger
		rayVertexLayout.init();
		rayVertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		rayVertexLayout.end();
		rayTextureColorH = renderCtx->createTexture(1, 1, 0, TextureLayout::RGBA32F, { (uint8_t*)&lineColor[0], sizeof(glm::vec4) }, NE_TEX_SAMPLER_MIN_MAG_NEAREST | NE_TEX_SAMPLER_UVW_MIRROR);

		for (int i = 0; i < 50; i++) {
			rayIndices[i] = nullptr;
		}

		for (int i = 0; i < scene->instancedModels.size(); i++) {
			InstancedModel *im = scene->instancedModels.at(i);
			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
				if (rmToRenderAPI.find(im->modelID) != rmToRenderAPI.end())
					continue;

				Model *model = ResourceManager::getSelf()->getModel(im->modelID);
				ModelHandler mh = renderCtx->createModel(model);
				rmToRenderAPI.insert({ im->modelID, mh });
			}

			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
				if (rmToRenderAPI.find(im->modelID) != rmToRenderAPI.end())
					continue;

				Model *model = ResourceManager::getSelf()->getModel(im->modelID);
				ModelHandler mh = renderCtx->createModel(model);
				rmToRenderAPI.insert({ im->modelID, mh });
			}
		}

		for (int i = 0; i < scene->lights.size(); i++) {
			InstancedModel* im = scene->lights.at(i);

			InfiniteAreaLight* infAreaLight;
			if (infAreaLight = dynamic_cast<InfiniteAreaLight*>(im->model->materials.at(0)->light)) {
				Material* m = im->model->materials.at(0);
				Texture *tex = m->getTexture(TextureName::TEX_1);
				infAreaLightTex = renderCtx->createTexture(tex);
			}

			if (rmToRenderAPI.find(im->modelID) != rmToRenderAPI.end())
				continue;

			Model* model = ResourceManager::getSelf()->getModel(im->modelID);
			ModelHandler mh = renderCtx->createModel(model);
			rmToRenderAPI.insert({ im->modelID, mh });
		}

		lightTexColorH = renderCtx->createTexture(1, 1, 0, TextureLayout::RGBA32F, { (uint8_t*)&defaultLightColor[0], sizeof(glm::vec4) }, NE_TEX_SAMPLER_MIN_MAG_NEAREST | NE_TEX_SAMPLER_UVW_MIRROR);
		realTimeTimer.startTimer();
	}

	void SceneEditor::initShadowShader() {
		Shader* shader = ResourceManager::getSelf()->getShader("simpleLightDepth");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		shadowProgramHandler = renderCtx->createProgram(shvertex, shfragment);
		glm::ivec2 res = settings.resolution;

		//lightProjection = glm::ortho(-(float)1.0f/ 2.0f, (float)1.0f / 2.0f, -(float)1.0f / 2.0f, (float)1.0f / 2.0f, 0.0f, 1.f);
		lightProjection = glm::ortho(4.5f, -4.5f, 4.5f, -4.5f, 0.0f, 10.f);
		//lightProjection = glm::ortho(0.f, -(float)res.x, 0.f, (float)res.y, -1.f, 1.f);
		//lightProjection = glm::ortho(-(float)res.x / 2.0f, (float)res.x / 2.0f, -(float)res.y / 2.0f, (float)res.y / 2.0f, 0.f, 10.f);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&lightProjection[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		shadowUniforms[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		shadowUniforms[1] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		shadowUniforms[2] = renderCtx->createUniform("view", mb3, UniformType::Mat4, 0);

		uint32_t sizeBytes = res.x * res.y * 4;
		shadowDepthTex = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);

		Attachment a[1];
		a[0] = { shadowDepthTex };
		shadowfboh = renderCtx->createFrameBuffer(1, &a[0]);
	}

	void SceneEditor::initMonoColorShader() {
		Shader* shader = ResourceManager::getSelf()->getShader("monocolor");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		monoColorProgramH = renderCtx->createProgram(shvertex, shfragment);

		monoColorUniforms[3] = renderCtx->createUniform("rgbColor", { (uint8_t*)&outlineObjColor, sizeof(outlineObjColor) }, UniformType::Vec4, 0);


		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		monoColorUniforms[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		monoColorUniforms[1] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		monoColorUniforms[2] = renderCtx->createUniform("cam", mb3, UniformType::Mat4, 0);
	}

	void SceneEditor::initImageDifferenceShader() {
		Shader* shader = ResourceManager::getSelf()->getShader("imageDifference");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
		imgDiffProgramH = renderCtx->createProgram(shvertex, shfragment);

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&proj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&cam[0][0]), sizeof(model) };

		imgDiffUniforms[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		imgDiffUniforms[1] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		imgDiffUniforms[2] = renderCtx->createUniform("cam", mb3, UniformType::Mat4, 0);

		imgDiffUniforms[3] = renderCtx->createUniform("tex1", { (uint8_t*)(&imgDiffId0), sizeof(int) }, UniformType::Sampler, 0);
		imgDiffUniforms[4] = renderCtx->createUniform("tex2", { (uint8_t*)(&imgDiffId1), sizeof(int) }, UniformType::Sampler, 0);
	}

	void SceneEditor::load() {
		initComposeShader();
		initMonoColorShader();
		initImageDifferenceShader();

		Shader *shader = ResourceManager::getSelf()->getShader("simpleTexture");

		ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
		ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);

		simpleTexProgramH = renderCtx->createProgram(shvertex, shfragment);
		quadModelHandler = renderCtx->createModel(ResourceManager::getSelf()->getModel("quad"));

		MemoryBuffer mb1 = { (uint8_t*)(&model[0][0]), sizeof(model) };
		MemoryBuffer mb2 = { (uint8_t*)(&orthoProj[0][0]), sizeof(model) };
		MemoryBuffer mb3 = { (uint8_t*)(&staticCam[0][0]), sizeof(model) };

		uniforms[0] = renderCtx->createUniform("model", mb1, UniformType::Mat4, 0);
		uniforms[1] = renderCtx->createUniform("proj", mb2, UniformType::Mat4, 0);
		uniforms[2] = renderCtx->createUniform("cam", mb3, UniformType::Mat4, 0);
		uniforms[4] = renderCtx->createUniform("tex", { (uint8_t*)(&texPos), sizeof(int)}, UniformType::Sampler, 0);

		uint32_t texSize = settings.resolution.x * settings.resolution.y * 3 * sizeof(float);
		fboTex = new Texture(settings.resolution.x, settings.resolution.y, TextureLayout::RGB32F, NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_LINEAR, {new uint8_t[texSize], texSize});

		fboTexHandler = renderCtx->createTexture(fboTex);

		glm::ivec2 res = settings.resolution;
		uint32_t sizeBytes = res.x * res.y * 3 * 4;
		uint8_t *data = new uint8_t[sizeBytes];
		float *fdata = (float*)data;
		for (int i = 0; i < sizeBytes/4; i++) {
			fdata[i] = 0.0f;
		}
		renderFrameTex[0] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { data, sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameTex[1] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameTex[2] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameTex[3] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::RGB32F, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);

		sizeBytes = res.x * res.y * 4; // 3 bytes per depth pixel (4 to fill integer)
		uint8_t* dataD = new uint8_t[sizeBytes];
		renderFrameDepthTex[0] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::D24S8, { dataD, sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameDepthTex[1] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameDepthTex[2] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
		renderFrameDepthTex[3] = renderCtx->createTexture(res.x, res.y, 0, TextureLayout::D24S8, { new uint8_t[sizeBytes], sizeBytes }, NE_TEX_SAMPLER_MIN_MAG_LINEAR | NE_TEX_SAMPLER_UVW_CLAMP);
	
		Attachment a[2];
		a[0] = { renderFrameTex[0] };
		a[1] = { renderFrameDepthTex[0] };
		fbRenderFrame[0] = renderCtx->createFrameBuffer(2, &a[0]);

		Attachment b[2];
		b[0] = { renderFrameTex[1] };
		b[1] = { renderFrameDepthTex[1] };
		fbRenderFrame[1] = renderCtx->createFrameBuffer(2, &b[0]);

		Attachment c[2];
		c[0] = { renderFrameTex[2] };
		c[1] = { renderFrameDepthTex[2] };
		fbRenderFrame[2] = renderCtx->createFrameBuffer(2, &c[0]);

		Attachment d[2];
		d[0] = { renderFrameTex[3] };
		d[1] = { renderFrameDepthTex[3] };
		fbRenderFrame[3] = renderCtx->createFrameBuffer(2, &d[0]);

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
		if (shouldUpdateSelectedIM)
			return true;

		return false;
	};

	void SceneEditor::hoverObject() {
		glm::vec2 mousePos = InputManager::getSelf()->getMousePosition();
		glm::vec2 renderFrameLeftCorner = glm::vec2((WIDTH - rightColumnMenuSize.x) / 2 - sceneFrameSize.x / 2, (HEIGHT - menuBarSize.y) / 2 + menuBarSize.y - sceneFrameSize.y / 2);

		bool selectObjectInput = InputManager::getSelf()->eventTriggered("SELECT_OBJECT");
		selectObjectInput = selectObjectInput && !ImGuizmo::IsUsing();

		if (mousePos.x >= renderFrameLeftCorner.x && mousePos.x < renderFrameLeftCorner.x + sceneFrameSize.x) {
			if (mousePos.y >= renderFrameLeftCorner.y && mousePos.y < renderFrameLeftCorner.y + sceneFrameSize.y) {
				glm::vec2 uv = (mousePos - renderFrameLeftCorner) / sceneFrameSize;
				uv.y = 1 - uv.y;
				//uv.x = 1 - uv.x;
				uv = uv * 2.0f - 1.0f; //tranform to NDC [-1, 1]


				glm::vec4 coords = glm::vec4(uv.x, uv.y, -1, 1);
				coords = invProj * coords;
				coords.w = 1;

				glm::mat4 invCam = glm::inverse(*camera.getCam());
				coords = invCam * coords;
				Ray r(*camera.getPosition(), glm::normalize(glm::vec3(coords) - *camera.getPosition()));
				RayIntersection ri;
				ri.tNear = 999999;
				ri.tFar = -999999;

				if (selectObjectInput) {
					bool didHit = scene->intersectScene(r, ri, 0, 999);
					if (didHit) {
						currentSelectedIM = ri.instancedModel;
						glm::mat4 transf = currentSelectedIM->transformToWCS;

						selectObjPos = getTranslation(transf);
						selectObjScale = getScale(transf);
						selectObjRotation = getRotation(transf);

						selectedImList = getSelectedIMListPos(currentSelectedIM);
					}else{
						selectedImList = -1;
						currentSelectedIM = nullptr;
						selectObjPos = glm::vec3(0);
						selectObjScale = glm::vec3(0);
						selectObjRotation = glm::vec3(0);
					}
				}
			}
		}
	};

	void SceneEditor::update(float deltaTime) {
		updateUI();
		camera.update();
		time = glfwGetTime();

		if (shouldUpdateCamera) {
			shouldUpdateCamera = false;
			proj = glm::perspective(glm::radians(camera.vfov), float(WIDTH)/HEIGHT, nearPlane, farPlane);
			invProj = glm::inverse(proj);
			offEngine->updateOfflineEngine(camera, settings, scene);
		}


		didSceneChange = sceneChanged();

		//if (didSceneChange && currentRenderingMode == REALTIME_RENDERING_MODE)
		//	realTimeTimer.startTimer();

		if (didSceneChange && currentRenderingMode == OFFLINE_RENDERING_MODE)
			startOffEngine();

		if (currentRenderingMode == OFFLINE_RENDERING_MODE) {
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
					renderCtx->updateTexture(fboTexHandler, 0, 0, 0, settings.resolution.x, settings.resolution.y, 0,{ (uint8_t*)(offEngine->pixels),  sizeBytes });
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

				renderCtx->updateTexture(fboTexHandler, 0, 0, 0, settings.resolution.x, settings.resolution.y, 0, { (uint8_t*)(offEngine->pixels),  sizeBytes });
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
		scene = sceneReader.getScene();
		camera = sceneReader.getMainCamera();
		settings = sceneReader.getSettings();

		sceneFrameSize = settings.resolution;

		//startOffEngine(); //TODO why was I starting it here?

		//camera = *sceneReader.getMainCamera();
		initRenderAPI();
		generateSceneImList();
	}

	void SceneEditor::variableUpdate(float deltaTime) {

	};

	int SceneEditor::getSelectedIMListPos(InstancedModel *selectedIm) {
		//TODO for now the first one is always the camera.
		int listPos = 1;

		//TODO could use a LBVH tree to sort through all scene's elements
		for (InstancedModel* im : scene->lights) {
			if (selectedIm == im)
				return listPos;
			listPos++;
		}

		for (InstancedModel* im : scene->instancedModels) {
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
		for (InstancedModel* im : scene->lights) {
			if (listPos == pos)
				return im;
			listPos++;
		}

		for (InstancedModel* im : scene->instancedModels) {
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

		for (InstancedModel* im : scene->lights) {
			int s = gStringIDTable.size();
			if (gStringIDTable.count(im->modelID))
				imNames[imNamesCurrentLength] = (char*)gStringIDTable[im->modelID];
			else
				imNames[imNamesCurrentLength] = std::string(std::to_string(im->modelID)).c_str();

			if (!im->model->lights.empty())
				imIcons[imNamesCurrentLength] = ICON_FA_LIGHTBULB;

			imNamesCurrentLength++;
		}

		for (InstancedModel* im : scene->instancedModels) {
			int s = gStringIDTable.size();
			if (gStringIDTable.count(im->modelID))
				imNames[imNamesCurrentLength] = (char*)gStringIDTable[im->modelID];
			else
				imNames[imNamesCurrentLength] = std::string(std::to_string(im->modelID)).c_str();

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
					if (ImGui::Selectable(imIcons[i].c_str(), selectedImList == i, ImGuiSelectableFlags_SpanAllColumns)) {
						currentSelectedIM = getSelectedIMfromListPos(i);
						selectedImList = i;
						glm::mat4 transf = currentSelectedIM->transformToWCS;

						selectObjPos = getTranslation(transf);
						selectObjScale = getScale(transf);
						selectObjRotation = getRotation(transf);
					}
					ImGui::PopID();

					if (!selectedItem)
						ImGui::PopStyleColor();

					ImGui::NextColumn();
					bool hovered = ImGui::IsItemHovered();
					ImGui::Text(imNames[i].c_str(), hovered); ImGui::NextColumn();

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
			if (ImGui::DragFloat3("##selectObjPos", &selectObjPos[0], 0.1, -999, 999, "%.2f")) {
				if (currentSelectedIM != nullptr) {
					ImGuizmo::RecomposeMatrixFromComponents(&selectObjPos[0], &selectObjRotation[0], &selectObjScale[0], &currentSelectedIM->transformToWCS[0][0]);
					currentSelectedIM->invTransformToWCS = glm::inverse(currentSelectedIM->transformToWCS);
				}
			}
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Scale: ");
			ImGui::NextColumn();
			if (ImGui::DragFloat3("##selectObjScale", &selectObjScale[0], 0.1, -999, 999, "%.2f")) {
				if (currentSelectedIM != nullptr) {
					ImGuizmo::RecomposeMatrixFromComponents(&selectObjPos[0], &selectObjRotation[0], &selectObjScale[0], &currentSelectedIM->transformToWCS[0][0]);
					currentSelectedIM->invTransformToWCS = glm::inverse(currentSelectedIM->transformToWCS);
				}
	}
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Rotation: ");
			ImGui::NextColumn();
			if (ImGui::DragFloat3("##selectObjRotation", &selectObjRotation[0], 0.1, -999, 999, "%.2f")) {
				if (currentSelectedIM != nullptr) {
					ImGuizmo::RecomposeMatrixFromComponents(&selectObjPos[0], &selectObjRotation[0], &selectObjScale[0], &currentSelectedIM->transformToWCS[0][0]);
					currentSelectedIM->invTransformToWCS = glm::inverse(currentSelectedIM->transformToWCS);
				}
			}

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
					if (ImGui::DragFloat3("##absorption", ((float*)vms->absorption), 0.001f, 0.001f, 5.0f))
						didSceneChange = true;
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Scattering (1/m): ");
					ImGui::NextColumn();
					if(ImGui::DragFloat3("##scattering", ((float*)vms->scattering), 0.001f, 0.001f, 5000.0f))
						didSceneChange = true;
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("Density Multiplier: ");
					ImGui::NextColumn();
					ImGui::PushItemWidth(80.0f);
					if(ImGui::DragFloat("##densityCoef", vms->density, 0.01, 0.001f, 150.0))
						didSceneChange = true;
					ImGui::PopItemWidth();
					ImGui::NextColumn();

					if (vms->currentMethod == RAY_MARCHING) {
						ImGui::NextColumn();
						ImGui::Text("N. of Steps: ");
						ImGui::NextColumn();
						ImGui::DragInt("##numberOfSteps", &(vms->rmNumberOfSteps), 2, 1.0, 100);
						ImGui::NextColumn();

						ImGui::NextColumn();
						ImGui::Text("Shadow Steps");
						ImGui::NextColumn();
						ImGui::DragInt("##shadowSteps", &vms->rmNumberOfShadowSteps, 1, 0.0, 256);
						ImGui::NextColumn();
					}

					ImGui::NextColumn();
					ImGui::Text("Phase Function: ");
					ImGui::NextColumn();
					ImGui::Combo("##phaseFunctionOption", &phaseFunctionOption, phaseFunctionOptions, IM_ARRAYSIZE(phaseFunctionOptions));
					ImGui::NextColumn();

					ImGui::NextColumn();
					ImGui::Text("g: ");
					ImGui::NextColumn();
					ImGui::DragFloat("##shadowSteps", vms->g, 0.01, -0.99, 0.99);
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
			ImGui::DragFloat3("##lightpower", &currentSelectedIM->model->lights.at(0)->material->light->li[0], 0.5, 0, 10000, "%.1f");
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

			float height = ImGui::GetFontSize() * (5 + 2);
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
			ImGui::Text("Volumetric Algorithm: ");
			ImGui::NextColumn();
			if(ImGui::Combo("##currentRenderMode", &(vms->currentMethod), renderModes, IM_ARRAYSIZE(renderModes)))
				didSceneChange = true;
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Resolution: ");
			ImGui::NextColumn();
			ImGui::DragInt2("##resolution", &settings.resolution[0], 1.0, 0, 3840, "%.1f");
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("SPP: ");
			ImGui::NextColumn();
			if (ImGui::DragInt("##spp", &settings.spp, 1.0, 0, 1000, "%.1f")) {
				vms->mcSPP = settings.spp;
				didSceneChange = true;
			}
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Bounces: ");
			ImGui::NextColumn();
			if (ImGui::DragInt("##bounces", &settings.bounces, 1.0, 0, 300, "%.0f")) {
				vms->mcMaxBounces = settings.bounces;
				didSceneChange = true;
			}
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

		if (frameCount == settings.spp && vms->currentMethod == VolumetricMethod::MONTE_CARLO)
			realTimeTimer.endTimer();
		
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
				if (vms->currentMethod == VolumetricMethod::MONTE_CARLO) {
					bool finished = (frameCount > settings.spp) ? true : false;

					if (!finished)
						ImGui::Text("Elapsed Time:   %.2f s", realTimeTimer.getElapsedSeconds());
					else
						ImGui::Text("Elapsed Time:   %.0f ms", realTimeTimer.getMilliseconds());
				}

			}else if (currentRenderingMode == OFFLINE_RENDERING_MODE) {
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

	void SceneEditor::toneMappingImGUI() {

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Tone Mapping", gUIPalette.iceGrey[4])) {
			ImVec2 size = ImGui::GetContentRegionMax();

			float height = ImGui::GetFontSize() * (5 + 2);
			float widthSpacing = ImGui::CalcTextSize("Exposure: ", 0, false, -1).x + columnOffset.x;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
			ImGui::BeginChild("##tonemapping", ImVec2(rightColumnMenuSize.x, height), 0, ImGuiWindowFlags_NoScrollWithMouse);
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
			ImGui::Text("Gamma: ");
			ImGui::NextColumn();
			ImGui::DragFloat("##gamma", &gamma, 0.1, 0, 10, "%.1f");
			ImGui::NextColumn();

			ImGui::NextColumn();
			ImGui::Text("Exposure: ");
			ImGui::NextColumn();
			ImGui::DragFloat("##exposure", &exposure, 0.1, 0, 10, "%.1f");
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}
	};

	void SceneEditor::shootRay(int i) {
		/*glm::vec3 rayDir;
		numberOfBounces = 0;

		if (pointTo)
			rayDir = glm::normalize(rayDirection - rayOrigin);
		else 
			rayDir = glm::normalize(rayDirection);

		intersectionPoints[i] = offEngine->pathIntegrator->getPath(Ray(rayOrigin, rayDir), scene);
		if (rayIndices[i] != nullptr)
			delete rayIndices[i];

		uint32_t numberOfLines = (intersectionPoints[i].size() / 2.0f);
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

		rayVertexBufferH[i] = renderCtx->createVertexBuffer({ (uint8_t*)&pathPoints.front(), verticesSizeInBytes }, rayVertexLayout);
		rayIndexBufferH[i] = renderCtx->createIndexBuffer({ (uint8_t*)rayIndices[i], numberOfLines * 2 * (uint32_t)sizeof(int) }, rayVertexLayout);

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
		logText += ss.str();*/
	}

	void SceneEditor::shootMultipleRays(int qtt){
		for (int i = 0; i < qtt; i++) {
			shootRay(i);
		}
	}
	
	void SceneEditor::shootRayImGUI() {
		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
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
		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("Shadow Mapping", gUIPalette.iceGrey[4])) {
			ImGui::Checkbox("Render Shadow Mapping", &renderShadowMapping);
		}
	}

	void SceneEditor::newVolRenderImGUI() {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGuiExt::CollapsingHeader("New Render", gUIPalette.iceGrey[4])) {
			didSceneChange = true;
			//ImGui::Text("Step Size: ");
			//ImGui::InputFloat("##newStepSize", &vms->lsStepSize, 0.01, 0);
			//ImGui::Text("Number of steps: ");
			//ImGui::InputInt("##newNumberOfSteps", &vms->lsNumberOfSteps, 1, 0);
			ImGui::Text("Points To Sample: ");
			ImGui::InputInt("##newnPointsToSample", &vms->lsnPointsToSample, 1, 0);
			ImGui::Text("Mean Path Mult: ");
			ImGui::InputInt("##newMeanPathMult", &vms->lsMeanPathMult, 1, 0);
			//ImGui::Text("Param_1: ");
			//ImGui::InputFloat("##param_1", &param_1, 1, 0);

			if(ImGui::Button("Random lobe points"))
				vms->lshasPointsBeenGenerated = false;

			ImGui::Text("Avg. mean path:");
			ImGui::Text("%f", vms->avgMeanPath);
		}
	}

	void SceneEditor::renderImGUI() {
		const char* openFile = "Open file";
		const char* closeFile = "Close file";
		const char* startingFolder = "./";
		const char* optionalFileExtensionFilterString = "";//".jpg;.jpeg;.png;.tiff;.bmp;.gif;.txt";

		//Main menu
		ImGui::BeginMainMenuBar();
		menuBarSize = ImGui::GetWindowSize();
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl + O")) {
				const char* pathptr = tinyfd_openFileDialog(openFile, startingFolder, 0, NULL, NULL, 0);
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

		if (ImGui::BeginMenu("Renderer")) {
			if (ImGui::BeginMenu("Export frame as...")) {
				if (ImGui::MenuItem(".EXR")) {
					const char *patterns[1] = {"*.exr"};
					std::string file = startingFolder + std::string("frame.exr");
					const char* path = tinyfd_saveFileDialog(closeFile, file.c_str(), 1, patterns, ".exr");
					if (path != nullptr) {
						int sizeBytes = settings.resolution.x * settings.resolution.y * 3 * sizeof(float);
						MemoryBuffer mem = { new uint8_t[sizeBytes], sizeBytes };

						renderCtx->readTexture(renderFrameTex[0], mem.data);
						saveImage(mem, settings.resolution.x, settings.resolution.y, TextureLayout::RGB32F, ImageFileFormat::EXR, path);
					}
				}
				if (ImGui::MenuItem(".PNG")) {
					const char* patterns[1] = { "*.png" };
					std::string file = startingFolder + std::string("frame.png");
					const char* path = tinyfd_saveFileDialog(closeFile, file.c_str(), 1, patterns, ".png");
					if (path != nullptr) {
						int sizeBytes = settings.resolution.x * settings.resolution.y * 3 * sizeof(float);
						MemoryBuffer mem = { new uint8_t[sizeBytes], sizeBytes };

						renderCtx->readTexture(renderFrameTex[0], mem.data);
						saveImage(mem, settings.resolution.x, settings.resolution.y, TextureLayout::RGB32F, ImageFileFormat::PNG, path);
					}
				}
				ImGui::EndMenu();
			}
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
		if (selectedImList > 0 && (selectedImList - 1) < scene->lights.size())
			lightImGui();
		renderOptionsImGUI();
		toneMappingImGUI();
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
			frameCount = 0;
			didSceneChange = true;
			currentRenderingMode = REALTIME_RENDERING_MODE;
		}
		if (ImGui::MenuItem("Pathtracer")) {
			currentRenderingMode = OFFLINE_RENDERING_MODE;
			startOffEngine();
		}
		if (ImGui::MenuItem("Reload Scene")) {
			//reloadScene(); //TODO fix
		}
		if (ImGui::MenuItem("Compare RealTime")) {
			frameCount = 0;
			didSceneChange = true;
			currentRenderingMode = 3;
		}
		ImGui::EndMenuBar();
		ImGui::End();
		ImGui::PopStyleVar();
		//ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		//ImGui::ShowDemoWindow();

		//Debug window
		ImGui::SetNextWindowPos(ImVec2(2, menuBarSize.y), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(rightColumnMenuSize.x / 1.5f, 1100), ImGuiCond_Once);
		ImGui::Begin("Debug", p_open, ImGuiWindowFlags_None);
		shootRayImGUI();
		shadowMappingImGUI();
		newVolRenderImGUI();
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


		//ImGuizmo::DrawGrid(&((*camera.getCam())[0][0]), &proj[0][0], &glm::mat4(1)[0][0], 100.f);

		if (currentSelectedIM != nullptr) {
			glm::mat4 transf = currentSelectedIM->transformToWCS;

			if (InputManager::getSelf()->eventTriggered("ROTATE_OBJECT"))
				currentGuizmoOp = ImGuizmo::ROTATE;
			else if (InputManager::getSelf()->eventTriggered("TRANSLATE_OBJECT"))
				currentGuizmoOp = ImGuizmo::TRANSLATE;
			else if (InputManager::getSelf()->eventTriggered("SCALE_OBJECT"))
				currentGuizmoOp = ImGuizmo::SCALE;


			ImGuizmo::Manipulate(&((*camera.getCam())[0][0]), &proj[0][0], currentGuizmoOp, ImGuizmo::LOCAL, &transf[0][0], NULL, NULL, NULL, NULL);
			
			//ImGuizmo uses RotX * RotY * RotZ rotation order.
			if (ImGuizmo::IsUsing()) {
				selectObjPos = getTranslation(transf);
				selectObjScale = getScale(transf);
				selectObjRotation = getRotation(transf);

				//ImGuizmo uses RotX * RotY * RotZ order.
				currentSelectedIM->transformToWCS = transf;
				currentSelectedIM->invTransformToWCS = glm::inverse(currentSelectedIM->transformToWCS);
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

		renderCtx->setUniform(uniforms[0]);
		renderCtx->setUniform(uniforms[1]);
		renderCtx->setUniform(uniforms[2]);
		renderCtx->setUniform(uniforms[4]);

		renderCtx->updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx->updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx->updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx->setTexture(fboTexHandler, uniforms[4]);
		renderCtx->setModel(quadModelHandler);

		renderCtx->render(simpleTexProgramH);
	}

	void SceneEditor::renderShadowMappingDebug() {
		model = glm::mat4(1);
		model = glm::translate(model, { (settings.resolution.x * 0.2f)/2, (settings.resolution.y * 0.2f) + 15, 0 });
		model = glm::scale(model, { (settings.resolution.x * 0.2f) / 2, -(settings.resolution.y * 0.2f) / 2, 1 });

		renderCtx->setUniform(uniforms[0]);
		renderCtx->setUniform(uniforms[1]);
		renderCtx->setUniform(uniforms[2]);
		renderCtx->setUniform(uniforms[4]);

		renderCtx->updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx->updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx->updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		//renderCtx->setTexture(renderFrameDepthTex[0], uniforms[4]);
		renderCtx->setTexture(shadowDepthTex, uniforms[4]);
		renderCtx->setModel(quadModelHandler);

		renderCtx->render(simpleTexProgramH);
	}

	void SceneEditor::renderRealTimeShadows() {
		renderCtx->setFrameBufferClear(shadowfboh, 1.1, 0, 0, 1);

		//for each light render each object's scene from its light point of view.
		int indexpbr = 0;
		for (int i = 0; i < scene->lights.size(); i++) {
			InstancedModel* imLight = scene->lights.at(i);
			lightPos[i] = getTranslation(imLight->transformToWCS);
			glm::vec3 lightLookAt = lightPos[i];
			lightLookAt.y = lightLookAt.y - lightLookAt.y;
			glm::vec3 front = glm::vec3(0.0f, 0.0f, 0.0f);
			lightView[i] = glm::lookAt(lightPos[i], lightLookAt + front, glm::vec3(0.0f, 0.0f, 1.0f));

			renderCtx->updateUniform(shadowUniforms[1], { &lightProjection[0][0], sizeof(glm::mat4) });
			renderCtx->updateUniform(shadowUniforms[2], { &lightView[i][0][0], sizeof(glm::mat4) });

			renderCtx->updateUniform(pbrLightUniforms[indexpbr], { (uint8_t*)&lightPos[i], sizeof(glm::vec3) });
			renderCtx->updateUniform(pbrLightUniforms[indexpbr + 1], { (uint8_t*)&imLight->model->materials.at(0)->light->li, sizeof(glm::vec3) });

			//TODO temporary gambiarra
			Model* m = scene->lights.at(i)->model;
			Rectangle* r;
			if (r = dynamic_cast<Rectangle*>(scene->lights.at(i)->model->materials.at(0)->light->primitive)) {
				volLightType = 0;
			}
			else {
				Sphere* s = dynamic_cast<Sphere*>(scene->lights.at(i)->model->materials.at(0)->light->primitive);

				volLightType = 1;
				renderCtx->updateUniform(volUniforms[16], { (uint8_t*)&lightPos[0], sizeof(glm::vec3) });
				volLightRecSize = glm::vec3(s->radius);
				continue;
			}

			renderCtx->updateUniform(volUniforms[16], { (uint8_t*)&lightPos[0], sizeof(glm::vec3) });
			renderCtx->updateUniform(volUniforms[17], { (uint8_t*)&scene->lights.at(0)->model->materials.at(0)->light->li, sizeof(glm::vec3) });
			volLightRecMin = r->getVertex(0);
			volLightRecMax = r->getVertex(1);
			volLightRecSize = r->getSize();
			volLightWCS = imLight->transformToWCS;
			renderCtx->updateUniform(volUniforms[33], { (uint8_t*)&volLightWCS, sizeof(glm::mat4) });

			for (InstancedModel* im : scene->instancedModels) {
				ModelHandler mh = rmToRenderAPI.at(im->modelID);
				//For each mesh that this model is made of
				for (int i = 0; i < im->model->meshes.size(); i++) {

					//TODO not correct, either move a pointer inside mesh or... 
					if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_DIFFUSE) {
						renderCtx->updateUniform(shadowUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });

						renderCtx->setUniform(shadowUniforms[0]);
						renderCtx->setUniform(shadowUniforms[1]);
						renderCtx->setUniform(shadowUniforms[2]);
						renderCtx->setMesh(mh.meshes.at(i));

						renderCtx->setFrameBuffer(shadowfboh);

						renderCtx->render(shadowProgramHandler);
					}
				}

				if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {
					ModelHandler mh = rmToRenderAPI.at(im->modelID);
					renderCtx->updateUniform(shadowUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
					
					renderCtx->setUniform(shadowUniforms[0]);
					renderCtx->setUniform(shadowUniforms[1]);
					renderCtx->setUniform(shadowUniforms[2]);
					renderCtx->setModel(rmToRenderAPI.at(im->modelID));

					renderCtx->setFrameBuffer(shadowfboh);

					renderCtx->render(shadowProgramHandler);
				}
			}

			indexpbr += pbrLightsOffset;
		}
	}

	void SceneEditor::renderScene(FrameBufferHandler *fbh, TextureHandler *fbhTex, int currentFrame) {
		for (InstancedModel* im : scene->lights) {

			if (im == currentSelectedIM) {
				renderCtx->setStencil(
					NE_STENCIL_OP_FAIL_S_KEEP | NE_STENCIL_OP_FAIL_Z_KEEP | NE_STENCIL_OP_PASS_Z_REPLACE |
					NE_STENCIL_TEST_ALWAYS |
					NE_STENCIL_FUNC_MASK(0xff));
			}


			for (int i = 0; i < maxPBRUniforms; i++) {
				if (isHandleValid(pbrUniforms[i].id))
					renderCtx->setUniform(pbrUniforms[i]);
				else
					break;
			}

			for (int i = 0; i < scene->lights.size() * pbrLightsOffset; i++) {
				if (isHandleValid(pbrLightUniforms[i].id))
					renderCtx->setUniform(pbrLightUniforms[i]);
				else
					break;
			}

			//texHandler e uniformHandler
			ModelHandler mh = rmToRenderAPI.at(im->modelID);
			renderCtx->setTexture(lightTexColorH, pbrUniforms[6]); //material.diffuse
			renderCtx->setTexture(lightTexColorH, pbrUniforms[7]); //material.metallic
			renderCtx->setTexture(lightTexColorH, pbrUniforms[8]); //material.specular
			renderCtx->setTexture(lightTexColorH, pbrUniforms[9]); //material.normal
			renderCtx->setTexture(lightTexColorH, pbrUniforms[10]); //material.roughness
			renderCtx->setTexture(lightTexColorH, pbrUniforms[11]); //material.ao

			renderCtx->updateUniform(pbrUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(glm::mat4) });
			renderCtx->updateUniform(pbrUniforms[1], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
			renderCtx->updateUniform(pbrUniforms[12], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
			renderCtx->setModel(rmToRenderAPI.at(im->modelID));

			renderCtx->setFrameBuffer(fbh[currentFrame]);

			renderCtx->render(pbrProgramHandler);

		}

		for (InstancedModel* im : scene->instancedModels) {

			if (im == currentSelectedIM) {
				renderCtx->setStencil(
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

					for (int i = 0; i < maxPBRUniforms; i++) {
						if (isHandleValid(pbrUniforms[i].id))
							renderCtx->setUniform(pbrUniforms[i]);
						else
							break;
					}

					for (int i = 0; i < scene->lights.size() * pbrLightsOffset; i++) {
						if (isHandleValid(pbrLightUniforms[i].id))
							renderCtx->setUniform(pbrLightUniforms[i]);
						else
							break;
					}

					//material.diffuse
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::ALBEDO)].id)) {
						isMaterialSet[0] = true;
						renderCtx->setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::ALBEDO)], pbrUniforms[6]);
					}else
						isMaterialSet[0] = false;

					//material.metallic
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::METALLIC)].id)) {
						isMaterialSet[1] = true;
						renderCtx->setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::METALLIC)], pbrUniforms[7]);
					}else
						isMaterialSet[1] = false;

					//material.normal
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::NORMAL_MAP)].id)) {
						isMaterialSet[3] = true;
						renderCtx->setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::NORMAL_MAP)], pbrUniforms[9]);
					}else
						isMaterialSet[3] = false;

					//material.roughness
					if (isHandleValid(mh.meshes.at(i).material.textures[ctz(TextureName::ROUGHNESS)].id)) {
						isMaterialSet[4] = true;
						renderCtx->setTexture(mh.meshes.at(i).material.textures[ctz(TextureName::ROUGHNESS)], pbrUniforms[10]);
					}else
						isMaterialSet[4] = false;

					renderCtx->setTexture(shadowDepthTex, pbrUniforms[13]); //shadowTex

					renderCtx->updateUniform(pbrUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
					renderCtx->updateUniform(pbrUniforms[1], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
					renderCtx->updateUniform(pbrUniforms[12], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });

					renderCtx->setMesh(mh.meshes.at(i));

					renderCtx->setFrameBuffer(fbh[currentFrame]);

					renderCtx->render(pbrProgramHandler);
				}
			}

			if (im->model->materials.at(0)->bsdf->bsdfType & BxDF_TRANSMISSION) {

				/*for (int i = 0; i < 43; i++)
					renderCtx->setUniform(volUniforms[i]);

				//texHandler e uniformHandler
				ModelHandler mh = rmToRenderAPI.at(im->modelID);

				if (isHandleValid(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)].id))
					renderCtx->setTexture(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)], volUniforms[4]); //volume
				//if (mh.meshes.at(0).textures.size() > 0)
				//	renderCtx->setTexture(mh.meshes.at(0).textures.at(0), volUniforms[4]); //volume

				//renderCtx->updateUniform(volUniforms[8], { (uint8_t*)&time, sizeof(time) }); //update time	
				//renderCtx->updateUniform(volUniforms[25], { (uint8_t*)&frameCount, sizeof(frameCount) }); //update frameCount
				//TODO invMaxDensity not setted as per volume
				Medium* m = im->model->materials.at(0)->medium;
				GridMedia* gm = (GridMedia*)m;
				densityMc = gm->densityMultiplier;
				invMaxDensity = gm->invMaxDensity;
				VolumeBSDF *volbsdf = (VolumeBSDF*)im->model->materials.at(0)->bsdf->bxdf[0];
				HG* hg = (HG*)volbsdf->phaseFunction;
				g = hg->g;

				scattering = gm->scattering / gm->densityMultiplier;
				absorption = gm->absorption / gm->densityMultiplier;

				renderCtx->updateUniform(volUniforms[28], { (uint8_t*)&invMaxDensity, sizeof(float) });
				renderCtx->updateUniform(volUniforms[29], { (uint8_t*)&densityMc, sizeof(float) });

				renderCtx->setTexture(fbhTex[(currentFrame + 1) % 2], volUniforms[5]); //background
				renderCtx->setTexture(renderFrameDepthTex[(currentFrame + 1) % 2], volUniforms[6]); //backgroundDepth
				renderCtx->setTexture(fbhTex[(currentFrame + 1) % 2], volUniforms[7]); //previous frame
				//renderCtx->setTexture(shadowDepthTex, phongUniforms[10]); //shadowTex
				if(infAreaLightTex.id != INVALID_HANDLE)
					renderCtx->setTexture(infAreaLightTex, volUniforms[37]); //infArea Texture

				renderCtx->updateUniform(volUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) });
				renderCtx->updateUniform(volUniforms[2], { (uint8_t*)camera.getCam(), sizeof(glm::mat4) });
				renderCtx->updateUniform(volUniforms[3], { (uint8_t*)(camera.getPosition()), sizeof(glm::vec3) });
				renderCtx->updateUniform(volUniforms[27], { (uint8_t*)&im->invTransformToWCS[0][0], sizeof(glm::mat4) });
				renderCtx->setModel(rmToRenderAPI.at(im->modelID));

				renderCtx->setFrameBuffer(fbh[currentFrame]);
				glDisable(GL_DEPTH_TEST); //TODO temporary, should not expose GL here plus it should be enabled only on volRederingMode = 1 (PATH TRACING MODE)	
				renderCtx->render(volProgramHandler);
				glEnable(GL_DEPTH_TEST);*/

				vms->proj = proj;
				vms->cam = *camera.getCam();
				vms->prepare(&camera, rmToRenderAPI.at(im->modelID), im, fbh[currentFrame], frameCount, fbhTex[(currentFrame + 1) % 2], defaultTex, renderFrameDepthTex[currentFrame]);

			}
		}
	}

	void SceneEditor::renderRealTimePBR() {
		hoverObject();
		currentFrame = ++currentFrame % 2;
		renderCtx->setFrameBufferClear(fbRenderFrame[3], 0, 0, 0, 1);

		if (didSceneChange && vms->currentMethod == VolumetricMethod::MONTE_CARLO) {
			realTimeTimer.startTimer();
			frameCount = 0;
			renderCtx->setFrameBufferClear(fbRenderFrame[0], 0, 0, 0, 1);
			renderCtx->setFrameBufferClear(fbRenderFrame[1], 0, 0, 0, 1);
			renderCtx->setFrameBufferClear(fbRenderFrame[2], 0, 0, 0, 1);
		}else if (vms->currentMethod == VolumetricMethod::RAY_MARCHING || vms->currentMethod == VolumetricMethod::LOBE_SAMPLING) {
			renderCtx->setFrameBufferClear(fbRenderFrame[0], 0, 0, 0, 1);
			renderCtx->setFrameBufferClear(fbRenderFrame[1], 0, 0, 0, 1);
			renderCtx->setFrameBufferClear(fbRenderFrame[2], 0, 0, 0, 1);
		}

		if (vms->currentMethod == VolumetricMethod::RAY_MARCHING || vms->currentMethod == VolumetricMethod::LOBE_SAMPLING)
			currentFrame = 0;

		maxBounces = settings.bounces;
		renderScene(&fbRenderFrame[0], &renderFrameTex[0], currentFrame);

		//Display final result
		model = glm::mat4(1);
		model = glm::translate(model, { (WIDTH) / 2, (HEIGHT) / 2, 0 });
		model = glm::scale(model, { WIDTH / 2, HEIGHT / 2, 1 });

		renderCtx->setUniform(uniforms[0]);
		renderCtx->setUniform(uniforms[1]);
		renderCtx->setUniform(uniforms[2]);
		renderCtx->setUniform(uniforms[4]);

		renderCtx->updateUniform(uniforms[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx->updateUniform(uniforms[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx->updateUniform(uniforms[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx->setTexture(renderFrameTex[currentFrame], uniforms[4]);
		renderCtx->setModel(quadModelHandler);

		renderCtx->setFrameBuffer(fbRenderFrame[3]);

		renderCtx->render(simpleTexProgramH);
		renderPostProcessing();

		if (renderShadowMapping)
			renderShadowMappingDebug();
		frameCount++;
	}

	void SceneEditor::renderPostProcessing() {
		model = glm::mat4(1);
		model = glm::translate(model, { (WIDTH - rightColumnMenuSize.x) / 2, (HEIGHT - menuBarSize.y) / 2, 0 });
		model = glm::scale(model, { sceneFrameSize.x / 2, sceneFrameSize.y / 2, 1 });

		for (int i = 0; i < 10; i++) {
			if (postProcessingUni[i].id == INVALID_HANDLE)
				break;
			renderCtx->setUniform(postProcessingUni[i]);
		}

		renderCtx->updateUniform(postProcessingUni[0], { (uint8_t*)(&model[0]), sizeof(model) });
		renderCtx->updateUniform(postProcessingUni[1], { (uint8_t*)(&orthoProj[0]), sizeof(model) });
		renderCtx->updateUniform(postProcessingUni[2], { (uint8_t*)(&staticCam[0]), sizeof(model) });

		renderCtx->setTexture(renderFrameTex[3], postProcessingUni[3]);
		renderCtx->setModel(quadModelHandler);

		renderCtx->render(postProcessingPH);
	}

	void SceneEditor::render() {
		if (currentRenderingMode == OFFLINE_RENDERING_MODE)
			renderOffline();
		else if (currentRenderingMode == REALTIME_RENDERING_MODE) {
			renderRealTimeShadows();
			renderRealTimePBR();
		}

		renderImGUI();
	};
}