#pragma once
#include "core/Renderer.h"
#include "core/Settings.h"
#include "primitives/InstancedModel.h"
#include "core/VolumeBSDF.h"
#include "core/Camera.h"

namespace narvalengine {
	enum VolumetricMethod {
		RAY_MARCHING,
		LOBE_SAMPLING,
		MONTE_CARLO
	};

	//VMS = Volumetric Methods Shaders
	class VMS {
	public:
		//Others
		RendererContext *renderCtx;
		Scene* scene;
		float dummyVar[3] = {-1, -1, -1};
		int currentMethod = RAY_MARCHING;
		static const int maxUniforms = 50;
		float avgMeanPath = 0;

		//Shader Commons
		glm::mat4 proj = glm::mat4(1);
		glm::mat4 cam = glm::mat4(1);
		glm::mat4 model = glm::mat4(1);
		glm::vec3 cameraPosition;
		glm::vec3 *scattering;
		glm::vec3 *absorption;
		float *density;
		float *g;
		float time = 0;
		int numberOfLights = 0;
		int volTexBind = 0;
		int bgTexBind = 1;
		int bgDepthTexBind = 2;
		UniformHandler lightUniforms[maxUniforms];
		glm::vec3 lightPos[maxUniforms / 6];
		glm::vec3 lightSize[maxUniforms / 6];
		glm::vec3 lightScale[maxUniforms / 6];
		const int lightStructElements = 7;

		//MS_rayMarching shader parameters
		//rm = Ray Marching
		ProgramHandler rmProgram;
		UniformHandler rmUniforms[maxUniforms];
		int rmNumberOfSteps = 60;
		int rmNumberOfShadowSteps = 10;

		//MS_lobeSampling shader parameters
		//ls = Lobe Sampling
		ProgramHandler lsProgram;
		UniformHandler lsUniforms[maxUniforms];
		UniformHandler lsPointUniforms[maxUniforms];
		float lsStepSize = 0.1;
		int lsNumberOfSteps = 50;
		int lsnPointsToSample = 30;
		int lsMeanPathMult = 20;
		glm::vec3 *lsPoints;
		bool lshasPointsBeenGenerated = false;

		//MS_monterCarlo shader parameters
		//mc = Monte Carlo
		ProgramHandler mcProgram;
		UniformHandler mcUniforms[maxUniforms];
		int mcPreviousFrameTexBind = 3;
		int mcMaxBounces;
		int mcSPP;
		int mcFrameCount = 0;

		VMS(RendererContext *renderCtx, Scene *scene) {
			this->renderCtx = renderCtx;
			this->scene = scene;

			initCommons();
			initMonteCarloShader();
			initRayMarchingShader();
			initLobeSamplingShader();
		};

		void handPickLobeSampling() {
			lshasPointsBeenGenerated = true;
			//sphere points
			lsPoints[0] = glm::normalize(glm::vec3(1,0,0));
			lsPoints[1] = glm::normalize(glm::vec3(0,1,0));
			lsPoints[2] = glm::normalize(glm::vec3(-1,0,0));
			lsPoints[3] = glm::normalize(glm::vec3(0,-1,0));

			lsPoints[4] = glm::normalize(glm::vec3(0,1,1));
			lsPoints[5] = glm::normalize(glm::vec3(0,-1,1));

			lsPoints[6] = glm::normalize(glm::vec3(0,1,-1));
			lsPoints[7] = glm::normalize(glm::vec3(0,-1,-1));

			lsPoints[8] = glm::normalize(glm::vec3(0,0,1));
			lsPoints[9] = glm::normalize(glm::vec3(0,0,-1));
		}

		void updateLights() {
			numberOfLights = scene->lights.size();

			for (int i = 0; i < numberOfLights; i++) {
				int index = 6 * i;
				InstancedModel* imLight = scene->lights.at(i);
				lightPos[i] = getTranslation(imLight->transformToWCS);

				Rectangle* r;
				if (r = dynamic_cast<Rectangle*>(scene->lights.at(i)->model->materials.at(0)->light->primitive)) {
					//volLightType = 0;
				}
				else {
					/*Sphere* s = dynamic_cast<Sphere*>(scene.lights.at(i)->model->materials.at(0)->light->primitive);

					volLightType = 1;
					renderCtx.updateUniform(volUniforms[16], { (uint8_t*)&lightPos[0], sizeof(glm::vec3) });
					volLightRecSize = glm::vec3(s->radius);
					continue;*/
					continue;
				}

				lightSize[i] = r->getSize();
				lightScale[i] = getScale(scene->lights.at(i)->transformToWCS);

				renderCtx->updateUniform(lightUniforms[index], { (uint8_t*)&lightPos[0], sizeof(glm::vec3) });
				renderCtx->updateUniform(lightUniforms[index + 1], { (uint8_t*)&scene->lights.at(i)->model->materials.at(0)->light->li, sizeof(glm::vec3) });
				renderCtx->updateUniform(lightUniforms[index + 2], { (uint8_t*)r->vertexData[0], sizeof(glm::vec3) });
				renderCtx->updateUniform(lightUniforms[index + 3], { (uint8_t*)r->vertexData[1], sizeof(glm::vec3) });
				renderCtx->updateUniform(lightUniforms[index + 4], { (uint8_t*)&lightSize[i], sizeof(glm::vec3) });
				renderCtx->updateUniform(lightUniforms[index + 5], { (uint8_t*)&scene->lights.at(i)->transformToWCS, sizeof(glm::mat4) });
				renderCtx->updateUniform(lightUniforms[index + 6], { &lightScale[i], sizeof(glm::vec3)});
			}
		}

		void initCommons() {
			numberOfLights = scene->lights.size();

			for (int i = 0; i < numberOfLights; i++) {
				InstancedModel* imLight = scene->lights.at(i);
				lightPos[i] = getTranslation(imLight->transformToWCS);

				int index = lightStructElements * i;

				//TODO missing minVertex, maxVertex and size
				std::string name = "lights[" + std::to_string(i) + "]";
				lightUniforms[index] = renderCtx->createUniform(std::string(name + ".position").c_str(), { (uint8_t*)&lightPos[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
				lightUniforms[index + 1] = renderCtx->createUniform(std::string(name + ".Li").c_str(), { (uint8_t*)&scene->lights.at(i)->model->materials.at(0)->light->li, sizeof(glm::vec3) }, UniformType::Vec3, 0);
				lightUniforms[index + 2] = renderCtx->createUniform(std::string(name + ".minVertex").c_str(), { (uint8_t*)&dummyVar[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
				lightUniforms[index + 3] = renderCtx->createUniform(std::string(name + ".maxVertex").c_str(), { (uint8_t*)&dummyVar[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
				lightUniforms[index + 4] = renderCtx->createUniform(std::string(name + ".size").c_str(), { (uint8_t*)&dummyVar[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
				lightUniforms[index + 5] = renderCtx->createUniform(std::string(name + ".transformWCS").c_str(), { (uint8_t*)&scene->lights.at(i)->transformToWCS, sizeof(glm::mat4) }, UniformType::Mat4, 0);
				lightUniforms[index + 6] = renderCtx->createUniform(std::string(name + ".scale").c_str(), { (uint8_t*)&dummyVar[0], sizeof(glm::vec3)}, UniformType::Vec3, 0);
			}
		}

		void initMonteCarloShader() {
			Shader* shader = ResourceManager::getSelf()->getShader("MS_monteCarlo");

			ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
			ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
			mcProgram = renderCtx->createProgram(shvertex, shfragment);

			mcUniforms[0] = renderCtx->createUniform("model", { (uint8_t*)&model[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			mcUniforms[1] = renderCtx->createUniform("invmodel", { (uint8_t*)&model[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			mcUniforms[2] = renderCtx->createUniform("cam", { (uint8_t*)&cam[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			mcUniforms[3] = renderCtx->createUniform("proj", { (uint8_t*)&proj[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			mcUniforms[4] = renderCtx->createUniform("cameraPosition", { (uint8_t*)&dummyVar[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
			mcUniforms[5] = renderCtx->createUniform("volume", { (uint8_t*)&volTexBind, sizeof(int) }, UniformType::Sampler, 0);
			mcUniforms[6] = renderCtx->createUniform("background", { (uint8_t*)&bgTexBind, sizeof(int) }, UniformType::Sampler, 0);
			mcUniforms[7] = renderCtx->createUniform("backgroundDepth", { (uint8_t*)&bgDepthTexBind, sizeof(int) }, UniformType::Sampler, 0);
			mcUniforms[8] = renderCtx->createUniform("scattering", { (uint8_t*)&dummyVar, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			mcUniforms[9] = renderCtx->createUniform("absorption", { (uint8_t*)&dummyVar, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			mcUniforms[10] = renderCtx->createUniform("densityCoef", { (uint8_t*)&dummyVar, sizeof(float) }, UniformType::Float, 0);
			mcUniforms[11] = renderCtx->createUniform("g", { (uint8_t*)dummyVar, sizeof(float) }, UniformType::Float, 0);
			mcUniforms[12] = renderCtx->createUniform("previousFrame", { (uint8_t*)&mcPreviousFrameTexBind, sizeof(float) }, UniformType::Sampler, 0);
			mcUniforms[13] = renderCtx->createUniform("invMaxDensity", { (uint8_t*)&dummyVar, sizeof(int) }, UniformType::Float, 0);
			mcUniforms[14] = renderCtx->createUniform("time", { (uint8_t*)&time, sizeof(float) }, UniformType::Float, 0);
			mcUniforms[15] = renderCtx->createUniform("maxBounces", { (uint8_t*)&mcMaxBounces, sizeof(int) }, UniformType::Int, 0);
			mcUniforms[16] = renderCtx->createUniform("SPP", { (uint8_t*)&mcSPP, sizeof(int) }, UniformType::Int, 0);
			mcUniforms[17] = renderCtx->createUniform("frameCount", { (uint8_t*)&mcFrameCount, sizeof(int) }, UniformType::Int, 0);
		}

		void initLobeSamplingShader() {
			Shader* shader = ResourceManager::getSelf()->getShader("MS_lobeSampling");

			ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
			ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
			lsProgram = renderCtx->createProgram(shvertex, shfragment);

			lsUniforms[0] = renderCtx->createUniform("model", { (uint8_t*)&model[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			lsUniforms[1] = renderCtx->createUniform("invmodel", { (uint8_t*)&model[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			lsUniforms[2] = renderCtx->createUniform("cam", { (uint8_t*)&cam[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			lsUniforms[3] = renderCtx->createUniform("proj", { (uint8_t*)&proj[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			lsUniforms[4] = renderCtx->createUniform("cameraPosition", { (uint8_t*)&dummyVar[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
			lsUniforms[5] = renderCtx->createUniform("volume", { (uint8_t*)&volTexBind, sizeof(int) }, UniformType::Sampler, 0);
			lsUniforms[6] = renderCtx->createUniform("background", { (uint8_t*)&bgTexBind, sizeof(int) }, UniformType::Sampler, 0);
			lsUniforms[7] = renderCtx->createUniform("backgroundDepth", { (uint8_t*)&bgDepthTexBind, sizeof(int) }, UniformType::Sampler, 0);
			lsUniforms[8] = renderCtx->createUniform("scattering", { (uint8_t*)&dummyVar, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			lsUniforms[9] = renderCtx->createUniform("absorption", { (uint8_t*)&dummyVar, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			lsUniforms[10] = renderCtx->createUniform("densityCoef", { (uint8_t*)&dummyVar, sizeof(float) }, UniformType::Float, 0);
			lsUniforms[11] = renderCtx->createUniform("g", { (uint8_t*)dummyVar, sizeof(float) }, UniformType::Float, 0);
			
			lsUniforms[12] = renderCtx->createUniform("time", { (uint8_t*)&time, sizeof(float) }, UniformType::Float, 0);
			lsUniforms[13] = renderCtx->createUniform("invMaxDensity", { (uint8_t*)&dummyVar, sizeof(float) }, UniformType::Float, 0);
			lsUniforms[14] = renderCtx->createUniform("stepSize", { (uint8_t*)&lsStepSize, sizeof(float) }, UniformType::Float, 0);
			lsUniforms[15] = renderCtx->createUniform("numberOfSteps", { (uint8_t*)&lsNumberOfSteps, sizeof(int) }, UniformType::Int, 0);
			lsUniforms[16] = renderCtx->createUniform("nPointsToSample", { (uint8_t*)&lsnPointsToSample, sizeof(int) }, UniformType::Int, 0);
			lsUniforms[17] = renderCtx->createUniform("meanPathMult", { (uint8_t*)&lsMeanPathMult, sizeof(int) }, UniformType::Int, 0);
			
			lsPoints = new glm::vec3[lsnPointsToSample];
			for (int i = 0; i < lsnPointsToSample; i++) {
				std::string name = "lobePoints[" + std::to_string(i) + "]";
				lsPointUniforms[i] = renderCtx->createUniform(name.c_str(), { (uint8_t*)&lsPoints[i], sizeof(glm::vec3) }, UniformType::Vec3, 0);
			}
		}

		void initRayMarchingShader() {
			Shader* shader = ResourceManager::getSelf()->getShader("MS_rayMarching");

			ShaderHandler shvertex = renderCtx->createShader(shader->vertexShader, NE_SHADER_TYPE_VERTEX);
			ShaderHandler shfragment = renderCtx->createShader(shader->fragmentShader, NE_SHADER_TYPE_FRAGMENT);
			rmProgram = renderCtx->createProgram(shvertex, shfragment);

			rmUniforms[0] = renderCtx->createUniform("model", { (uint8_t*)&model[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			rmUniforms[1] = renderCtx->createUniform("invmodel", { (uint8_t*)&model[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			rmUniforms[2] = renderCtx->createUniform("cam", { (uint8_t*)&cam[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			rmUniforms[3] = renderCtx->createUniform("proj", { (uint8_t*)&proj[0][0], sizeof(glm::mat4) }, UniformType::Mat4, 0);
			rmUniforms[4] = renderCtx->createUniform("cameraPosition", { (uint8_t*)&dummyVar[0], sizeof(glm::vec3) }, UniformType::Vec3, 0);
			rmUniforms[5] = renderCtx->createUniform("volume", { (uint8_t*)&volTexBind, sizeof(int) }, UniformType::Sampler, 0);
			rmUniforms[6] = renderCtx->createUniform("background", { (uint8_t*)&bgTexBind, sizeof(int) }, UniformType::Sampler, 0);
			rmUniforms[7] = renderCtx->createUniform("backgroundDepth", { (uint8_t*)&bgDepthTexBind, sizeof(int) }, UniformType::Sampler, 0);
			rmUniforms[8] = renderCtx->createUniform("scattering", { (uint8_t*)&dummyVar, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			rmUniforms[9] = renderCtx->createUniform("absorption", { (uint8_t*)&dummyVar, sizeof(glm::vec3) }, UniformType::Vec3, 0);
			rmUniforms[10] = renderCtx->createUniform("densityCoef", { (uint8_t*)&dummyVar, sizeof(float) }, UniformType::Float, 0);
			rmUniforms[11] = renderCtx->createUniform("numberOfSteps", { (uint8_t*)&rmNumberOfSteps, sizeof(int) }, UniformType::Int, 0);
			rmUniforms[12] = renderCtx->createUniform("numberOfShadowSteps", { (uint8_t*)&rmNumberOfShadowSteps, sizeof(int) }, UniformType::Int, 0);
			rmUniforms[13] = renderCtx->createUniform("g", { (uint8_t*)dummyVar, sizeof(float) }, UniformType::Float, 0);
		}

		void prepareMonteCarlo(Camera* camera, ModelHandler mh, InstancedModel* im, FrameBufferHandler fbh, TextureHandler previousFrame, TextureHandler background, TextureHandler backgroundDepth) {
			updateLights(); //TODO should be invoked only once a changed has been detected
			time = glfwGetTime();

			for (int i = 0; i < maxUniforms; i++) {
				if (mcUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(mcUniforms[i]);
			}

			for (int i = 0; i < maxUniforms; i++) {
				if (lightUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(lightUniforms[i]);
			}

			if (isHandleValid(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)].id))
				renderCtx->setTexture(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)], mcUniforms[5]); //volume Tex

			Medium* m = im->model->materials.at(0)->medium;
			GridMedia* gm = (GridMedia*)m;
			VolumeBSDF* volbsdf = (VolumeBSDF*)im->model->materials.at(0)->bsdf->bxdf[0];
			HG* hg = (HG*)volbsdf->phaseFunction;
			g = &hg->g;

			scattering = &gm->scattering; //TODO  divide by gm->densityMultiplier
			absorption = &gm->absorption;
			density = &gm->densityMultiplier;

			renderCtx->updateUniform(mcUniforms[8], { (uint8_t*)scattering, sizeof(glm::vec3) });
			renderCtx->updateUniform(mcUniforms[9], { (uint8_t*)absorption, sizeof(glm::vec3) });
			renderCtx->updateUniform(mcUniforms[10], { (uint8_t*)density, sizeof(float) });
			renderCtx->updateUniform(mcUniforms[13], { (uint8_t*)&gm->invMaxDensity, sizeof(float) });
			renderCtx->updateUniform(mcUniforms[11], { (uint8_t*)g, sizeof(float) });

			renderCtx->setTexture(background, mcUniforms[6]); //background Tex
			renderCtx->setTexture(backgroundDepth, mcUniforms[7]); //backgroundDepth Tex
			renderCtx->setTexture(previousFrame, mcUniforms[12]); //previousFrame Tex

			renderCtx->updateUniform(mcUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) }); //model
			renderCtx->updateUniform(mcUniforms[1], { (uint8_t*)&im->invTransformToWCS[0][0], sizeof(glm::mat4) }); //invmodel
			renderCtx->updateUniform(mcUniforms[4], { (uint8_t*)(camera->getPosition()), sizeof(glm::vec3) });
			renderCtx->setModel(mh);

			renderCtx->setFrameBuffer(fbh);
			glDisable(GL_DEPTH_TEST); //TODO temporary, should not expose GL here plus it should be enabled only on volRederingMode = 1 (PATH TRACING MODE)	
			renderCtx->render(mcProgram);
			glEnable(GL_DEPTH_TEST);
		}

		void prepareLobeSampling(Camera* camera, ModelHandler mh, InstancedModel* im, FrameBufferHandler fbh, TextureHandler background, TextureHandler backgroundDepth) {
			updateLights(); //TODO should be invoked only once a changed has been detected
			time = glfwGetTime();

			for (int i = 0; i < maxUniforms; i++) {
				if (lsUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(lsUniforms[i]);
			}

			for (int i = 0; i < maxUniforms; i++) {
				if (lsPointUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(lsPointUniforms[i]);
			}

			for (int i = 0; i < maxUniforms; i++) {
				if (lightUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(lightUniforms[i]);
			}

			if (isHandleValid(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)].id))
				renderCtx->setTexture(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)], lsUniforms[5]); //volume Tex

			Medium* m = im->model->materials.at(0)->medium;
			GridMedia* gm = (GridMedia*)m;
			VolumeBSDF* volbsdf = (VolumeBSDF*)im->model->materials.at(0)->bsdf->bxdf[0];
			HG* hg = (HG*)volbsdf->phaseFunction;
			g = &hg->g;
			avgMeanPath = 1.0f / (avg(gm->extinction) * gm->densityMultiplier);
			
			if (InputManager::getSelf()->eventTriggered("TESTING_KEY"))
				lshasPointsBeenGenerated = false;

			if (!lshasPointsBeenGenerated) {
				lshasPointsBeenGenerated = true;
				for (int i = 0; i < lsnPointsToSample; i++) 
					lsPoints[i] = hg->sample(glm::vec3(0, 0, 1));

				//handPickLobeSampling();
			}

			scattering = &gm->scattering;
			absorption = &gm->absorption;
			density = &gm->densityMultiplier;

			renderCtx->updateUniform(lsUniforms[8], { (uint8_t*)scattering, sizeof(glm::vec3) });
			renderCtx->updateUniform(lsUniforms[9], { (uint8_t*)absorption, sizeof(glm::vec3) });
			renderCtx->updateUniform(lsUniforms[10], { (uint8_t*)density, sizeof(float) });
			renderCtx->updateUniform(lsUniforms[13], { (uint8_t*)&gm->invMaxDensity, sizeof(float) });
			renderCtx->updateUniform(lsUniforms[11], { (uint8_t*)g, sizeof(float) });

			renderCtx->setTexture(background, lsUniforms[6]); //background Tex
			renderCtx->setTexture(backgroundDepth, lsUniforms[7]); //backgroundDepth Tex

			renderCtx->updateUniform(lsUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) }); //model
			renderCtx->updateUniform(lsUniforms[1], { (uint8_t*)&im->invTransformToWCS[0][0], sizeof(glm::mat4) }); //invmodel
			renderCtx->updateUniform(lsUniforms[4], { (uint8_t*)(camera->getPosition()), sizeof(glm::vec3) });
			renderCtx->setModel(mh);

			renderCtx->setFrameBuffer(fbh);
			//glDisable(GL_DEPTH_TEST); //TODO temporary, should not expose GL here plus it should be enabled only on volRederingMode = 1 (PATH TRACING MODE)	
			renderCtx->render(lsProgram);
			//glEnable(GL_DEPTH_TEST);
		}

		void prepareRayMarching(Camera *camera, ModelHandler mh, InstancedModel *im, FrameBufferHandler fbh,TextureHandler background, TextureHandler backgroundDepth) {
			updateLights(); //TODO should be invoked only once a changed has been detected

			for (int i = 0; i < maxUniforms; i++) {
				if (rmUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(rmUniforms[i]);
			}

			for (int i = 0; i < maxUniforms; i++) {
				if (lightUniforms[i].id == INVALID_HANDLE)
					break;
				renderCtx->setUniform(lightUniforms[i]);
			}

			if (isHandleValid(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)].id))
				renderCtx->setTexture(mh.meshes.at(0).material.textures[ctz(TextureName::TEX_1)], rmUniforms[5]); //volume Tex

			Medium* m = im->model->materials.at(0)->medium;
			GridMedia* gm = (GridMedia*)m;
			VolumeBSDF* volbsdf = (VolumeBSDF*)im->model->materials.at(0)->bsdf->bxdf[0];
			HG* hg = (HG*)volbsdf->phaseFunction;
			g = &hg->g;

			scattering = &gm->scattering; //TODO  divide by gm->densityMultiplier
			absorption = &gm->absorption;
			density = &gm->densityMultiplier;

			renderCtx->updateUniform(rmUniforms[8], { (uint8_t*)scattering, sizeof(glm::vec3) });
			renderCtx->updateUniform(rmUniforms[9], { (uint8_t*)absorption, sizeof(glm::vec3) });
			renderCtx->updateUniform(rmUniforms[10], { (uint8_t*)density, sizeof(float) });
			renderCtx->updateUniform(rmUniforms[13], { (uint8_t*)g, sizeof(float) });

			renderCtx->setTexture(background, rmUniforms[6]); //background Tex
			renderCtx->setTexture(backgroundDepth, rmUniforms[7]); //backgroundDepth Tex

			renderCtx->updateUniform(rmUniforms[0], { (uint8_t*)(&im->transformToWCS[0][0]), sizeof(im->transformToWCS) }); //model
			renderCtx->updateUniform(rmUniforms[1], { (uint8_t*)&im->invTransformToWCS[0][0], sizeof(glm::mat4) }); //invmodel
			renderCtx->updateUniform(rmUniforms[4], { (uint8_t*)(camera->getPosition()), sizeof(glm::vec3) });
			renderCtx->setModel(mh);

			renderCtx->setFrameBuffer(fbh);
			//glDisable(GL_DEPTH_TEST); //TODO temporary, should not expose GL here plus it should be enabled only on volRederingMode = 1 (PATH TRACING MODE)	
			renderCtx->render(rmProgram);
			//glEnable(GL_DEPTH_TEST);
		}

		void prepare(Camera* camera, ModelHandler mh, InstancedModel* im, FrameBufferHandler fbh, int frameCount, TextureHandler previousFrame, TextureHandler background, TextureHandler backgroundDepth) {
			this->mcFrameCount = frameCount;
			if (currentMethod == RAY_MARCHING)
				prepareRayMarching(camera, mh, im, fbh, background, backgroundDepth);
			else if(currentMethod == LOBE_SAMPLING)
				prepareLobeSampling(camera, mh, im, fbh, background, backgroundDepth);
			else if(currentMethod == MONTE_CARLO)
				prepareMonteCarlo(camera, mh, im, fbh, previousFrame, background, backgroundDepth); 
		}
	};
}

