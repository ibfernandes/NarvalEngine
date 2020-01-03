#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "SphericalHarmonics.h"
#include "imgui.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class SHVolumeScene: public Scene
{
public:
	SHVolumeScene();
	~SHVolumeScene();

	float nearPlane = 1;
	float farPlane = 60000;
	float projAngle = 45;
	glm::mat4 proj;
	glm::mat4 model = glm::mat4(1);
	Renderer *renderer;
	Camera *camera;
	std::string currentShader;
	ImGuiWindowFlags window_flags = 0;

	bool showMainMenu = true;
	bool *p_open = &showMainMenu;

	//Lightning
	glm::vec3 lightPosition = glm::vec3(0.0, 1.6, 4.0);
	glm::vec3 lightColor = glm::vec3(1.0, 1.0, 1.0);
	float ambientStrength = 0.1;
	float lightMaxRadius = 1000.0f;

	//Volume's material
	glm::vec3 absorption = glm::vec3(0.001f);
	glm::vec3 scattering = glm::vec3(0.25, 0.5, 1.0);
	float densityCoef = 0.75;

	//Other
	bool lockScattering = true;
	int phaseFunctionOption = 2;
	float numberOfSteps = 128;
	float shadowSteps = 16;
	float g = 0.9;
	bool gammaCorrection = false;
	bool enableShadow = true;
	bool nightMode = false;

	int nSamples = 20;

	int lBands = 3;
	int gridResolution = 128;
	//Texture2D fbotexture;
	//FBO fbo;
	float *lightCoeffs;
	float *lightVis;
	Texture3D visibility;
	Texture3D light;
	int lod = 1;
	float currentSlice = 1;

	void initSphericalHarmonics() {
		srand(time(NULL));

		lightCoeffs = new float[gridResolution * gridResolution * gridResolution * lBands * lBands / lod];
		lightVis = new float[gridResolution * gridResolution * gridResolution * 4 / lod];
		visibility.generateWithData(gridResolution / lod, gridResolution / lod, gridResolution / lod, 4, lightVis);
	}

	void generateSphericalHarmonics() {
		Sampler *sampler = new Sampler;
		generateSamples(sampler, nSamples);
		precomputeSHCoefficients(sampler, lBands);
		projectLightFunction(lightCoeffs, sampler, lBands, gridResolution, lightVis, lod);

		light.generateWithData(gridResolution * lBands * lBands / lod, gridResolution / lod, gridResolution / lod, 1, lightCoeffs);
	}

	void init(GLint width, GLint height, Renderer *r, Camera *c) {
		camera = c;
		renderer = r;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)width / (GLfloat)height, nearPlane, farPlane);

		initSphericalHarmonics();
		preRender();
	}

	void renderVolumeSlices() {
		glDisable(GL_DEPTH_TEST);

		std::string currentShader = "visibility";
		ResourceManager::getSelf()->getShader(currentShader).use();
		ResourceManager::getSelf()->getShader(currentShader).setInteger("lod", lod);

		ResourceManager::getSelf()->getShader(currentShader).setInteger("volume", 0);
		glActiveTexture(GL_TEXTURE0);
		ResourceManager::getSelf()->getTexture3D("cloud").bind();

		//ResourceManager::getSelf()->getShader(currentShader).setInteger("visibility", 1);
		glActiveTexture(GL_TEXTURE1);
		visibility.bind();
		glBindImageTexture(1, visibility.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);


		ResourceManager::getSelf()->getShader(currentShader).setFloat("time", glfwGetTime());
		ResourceManager::getSelf()->getShader(currentShader).setFloat("numberOfSamples", 100);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("currentSlice", currentSlice);


		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));

		glEnable(GL_DEPTH_TEST);
	}

	void preRender() {
		/*for (int i = 0; i < 128 / lod; i++) {
			glViewport(0, 0, WIDTH, HEIGHT);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderVolumeSlices();
			currentSlice = i;

			glfwSwapBuffers(window);
		}

		generateSphericalHarmonics();*/
	}

	void updateUI() {
		currentShader = "volume";
		ResourceManager::getSelf()->getShader(currentShader).use();

		ResourceManager::getSelf()->getShader(currentShader).setVec3("absorption", absorption.x, absorption.y, absorption.z);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("scattering", scattering.x, scattering.y, scattering.z);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightColor", lightColor.x, lightColor.y, lightColor.z);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);

		ResourceManager::getSelf()->getShader(currentShader).setFloat("densityCoef", densityCoef);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightMaxRadius", lightMaxRadius);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("ambientStrength", ambientStrength);

		ResourceManager::getSelf()->getShader(currentShader).setFloat("phaseFunctionOption", phaseFunctionOption);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("numberOfSteps", numberOfSteps);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("shadowSteps", shadowSteps);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("g", g);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("gammaCorrection", gammaCorrection);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("enableShadow", enableShadow);
	}

	void renderImGUI();

	void renderVolume() {
		currentShader = "shvolume";
		ResourceManager::getSelf()->getShader(currentShader).use();
		ResourceManager::getSelf()->getShader(currentShader).setInteger("lod", lod);
		model = glm::mat4(1);
		model = glm::translate(model, { -0.5, -0.5, -0.5 });

		ResourceManager::getSelf()->getShader(currentShader).setInteger("visibility", 0);
		glActiveTexture(GL_TEXTURE0);
		visibility.bind();

		ResourceManager::getSelf()->getShader(currentShader).setInteger("light", 1);
		glActiveTexture(GL_TEXTURE1);
		light.bind();


		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *(camera->getCam()));
		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
		glm::vec3 camPos = *(camera->getPosition());
		ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", camPos.x, camPos.y, camPos.z);

		(*renderer).render(ResourceManager::getSelf()->getModel("cubeTest"));
	}

	void renderLightPosition() {

		std::string currentShader = "billboard";
		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, lightPosition);
		model = glm::scale(model, { 0.1, 0.1, 0.1 });

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *(camera->getCam()));
		ResourceManager::getSelf()->getShader(currentShader).setVec3("position", lightPosition);

		glActiveTexture(GL_TEXTURE0);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("material.diffuse", 0);
		ResourceManager::getSelf()->getTexture2D("lightbulb").bind();

		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));
	}

	void updateVariable(float deltaTime) {

	}

	void update(float deltaTime) {
		updateUI();
	}

	void render() {
		renderImGUI();
		renderVolume();
		renderLightPosition();
	}
};

