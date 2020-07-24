#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Texture1D.h"
#include "FBO.h"
#include "imgui.h"
#include "LBVH.h"
#include "LBVH2.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class VolumeScene: public Scene
{
public:
	VolumeScene();
	~VolumeScene();

	float nearPlane = 1;
	float farPlane = 60000;
	float projAngle = 45;
	glm::mat4 proj;
	glm::mat4 staticCam;
	glm::mat4 orthoProj;
	glm::mat4 model = glm::mat4(1);
	Renderer *renderer;
	Camera *camera;
	std::string currentShader;
	ImGuiWindowFlags window_flags = 0;

	bool showMainMenu = true;
	bool *p_open = &showMainMenu;

	//Lightning
	glm::vec3 lightPosition = glm::vec3(0.0, 1.6, -4.0);
	glm::vec3 lightColor = glm::vec3(1.0, 1.0, 1.0);
	float ambientStrength = 300.0;
	float Kc = 1.0, Kl = 0.7, Kq = 1.8;

	//Volume's material
	glm::vec3 modelTranslation = glm::vec3(0,0,0);
	glm::vec3 absorption = glm::vec3(0.001f);
	glm::vec3 scattering = glm::vec3(0.25, 0.5, 1.0);
	float densityCoef = 0.75;

	//Other
	bool lockScattering = true;
	int phaseFunctionOption = 0;
	float numberOfSteps = 128;
	float shadowSteps = 16;
	float g = 0.9;
	bool gammaCorrection = false;
	bool enableShadow = true;
	bool nightMode = false; 
	int frameCount = 1;

	FBO fbo;
	FBO cloudFbo[2];
	Texture2D cloudTex[2];
	int currentCloudFrame = 0;
	Texture2D fboTex;
	Texture2D fboDepthTex;
	glm::vec3 gradientDownNight = glm::vec3(57 / 255.0f, 65 / 255.0f, 73 / 255.0f);
	glm::vec3 gradientUpNight = glm::vec3(19 / 155.0f, 21 / 255.0f, 23 / 255.0f);
	glm::vec3 gradientDownDay = glm::vec3(1);
	glm::vec3 gradientUpDay = glm::vec3(1);

	GLint WIDTH, HEIGHT;
	float aspectRatio;
	
	bool firstPass = true;
	int SPP = 1;
	bool clearRayTracing = true;
	int currentModel = 0;
	const char* models[6] = {"cloud", "dragonHavard", "fireball", "explosion", "bunny_cloud", "explosion"};
	std::string currentModelResolution = "";

	int currentRenderMode = 0;
	const char* renderModes[3] = { "Ray marching grid", "Path tracing MC", "LBVH"};
	const char* phaseFunctionOptions[3] = { "Isotropic", "Rayleigh", "Henyey-Greenstein" };

	//LBVH
	LBVH *lbvh;
	glm::vec3 lbvhres = glm::vec3(512, 512, 512);
	Texture3D nodes;

	//LBVH2
	LBVH2 *lbvh2;

	void init(GLint width, GLint height, Renderer *r, Camera *c) {
		this->WIDTH = width;
		this->HEIGHT = height;
		aspectRatio = (float)width / height;
		camera = c;
		c->setSpeed(3);
		renderer = r;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
		orthoProj = glm::ortho(0.f, -(float)width, 0.f, (float)height, -1.f, 1.f);

		float *vec = new float[width * height * 4];
		float *vec2 = new float[width * height * 4];
		float *vec3 = new float[width * height * 4];
		fboTex.generateWithData(width, height, 4, vec);
		cloudTex[0].generateWithData(width, height, 4, vec2);
		cloudTex[1].generateWithData(width, height, 4, vec3);
		fboDepthTex.generateWithData(width, height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

		fbo.attachTexture(fboTex);
		fbo.attachDepthAndStencil(fboDepthTex.id);
		cloudFbo[0].attachTexture(cloudTex[0]);
		cloudFbo[1].attachTexture(cloudTex[1]);

		glm::vec3 position(0, 0, 0);
		glm::vec3 front = { 0.0f, 0.0f, 1.0f };
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 side = glm::cross(front, up);
		staticCam = glm::lookAt(position, position + front, up);
	}

	void lbvhBin() {
		lbvhres = ResourceManager::getSelf()->getTexture3D(models[currentModel])->getResolution();
		lbvhres = glm::vec3(256, 256, 256);
		std::cout << "LBVH size: " << lbvhres.x << ", " << lbvhres.y << ", " << lbvhres.z << std::endl;
		lbvh = new LBVH(lbvhres);

		nodes.loadToMemory(lbvhres.x, lbvhres.y, lbvhres.z, 3, lbvh->data);
		std::cout << lbvh->data[3];

		GLuint ssboMax;
		glGenBuffers(1, &ssboMax);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboMax);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * lbvhres.x * lbvhres.y * lbvhres.z * 3, lbvh->data, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboMax);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void lbvhBucket() {
		lbvhres = ResourceManager::getSelf()->getTexture3D(models[currentModel])->getResolution();
		std::cout << "LBVH size: " << lbvhres.x << ", " << lbvhres.y << ", " << lbvhres.z << std::endl;
		lbvh2 = new LBVH2(ResourceManager::getSelf()->getTexture3D(models[currentModel])->floatData, lbvhres);

		GLuint ssboNodeData;
		glGenBuffers(1, &ssboNodeData);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboNodeData);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * lbvh2->dataSize, lbvh2->node, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboNodeData);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		GLuint ssboOffsets;
		glGenBuffers(1, &ssboOffsets);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboOffsets);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * lbvh2->amountOfBuckets, lbvh2->offsets, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssboOffsets);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		GLuint ssboMax;
		glGenBuffers(1, &ssboMax);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboMax);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * lbvh2->mortonCodesSize, lbvh2->mortonCodes, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssboMax);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void load() {
		lbvhBin();
		//lbvhBucket();
	}

	void updateUI() {

		currentShader = "volumelbvh";
		ResourceManager::getSelf()->getShader(currentShader).use();

		ResourceManager::getSelf()->getShader(currentShader).setVec3("absorption", absorption.x, absorption.y, absorption.z);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("scattering", scattering.x, scattering.y, scattering.z);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightColor", lightColor.x, lightColor.y, lightColor.z);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);

		ResourceManager::getSelf()->getShader(currentShader).setFloat("densityCoef", densityCoef);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("ambientStrength", ambientStrength);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("Kc", Kc);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("Kl", Kl);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("Kq", Kq);

		ResourceManager::getSelf()->getShader(currentShader).setFloat("phaseFunctionOption", phaseFunctionOption);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("numberOfSteps", numberOfSteps);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("shadowSteps", shadowSteps);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("g", g);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("gammaCorrection", gammaCorrection);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("enableShadow", enableShadow);

		glm::vec3 r = ResourceManager::getSelf()->getTexture3D(models[currentModel])->getResolution();
		currentModelResolution = "";
		currentModelResolution = "[" + std::to_string((int)r.x) + ", " + std::to_string((int)r.y) + ", " + std::to_string((int)r.z) + "]";

		ResourceManager::getSelf()->getShader(currentShader).setInteger("renderingMode", currentRenderMode);
	}

	void renderImGUI();

	void renderVolume(Texture2D previousFrame) {

		currentShader = "volumelbvh";
		ResourceManager::getSelf()->getShader(currentShader).use();
		glm::vec3 camPos = *(camera->getPosition());
		

		glm::vec3 resolution = ResourceManager::getSelf()->getTexture3D(models[currentModel])->getResolution();
		resolution = resolution / resolution.z;
		resolution = 1.0f * resolution; 

		model = glm::mat4(1);
		model = glm::translate(model, - resolution/2.0f);

		model = glm::scale(model, resolution);

		glActiveTexture(GL_TEXTURE0);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("volume", 0);
		ResourceManager::getSelf()->getTexture3D(models[currentModel])->bind();

		glActiveTexture(GL_TEXTURE1);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("background", 1);
		fboTex.bind();

		glActiveTexture(GL_TEXTURE2);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("backgroundDepth", 2);
		fboDepthTex.bind();

		glActiveTexture(GL_TEXTURE3);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("previousCloud", 3);
		previousFrame.bind();

		ResourceManager::getSelf()->getShader(currentShader).setIntegerVec3("lbvhSize", lbvhres);

		float time = ((sin(glm::radians(glfwGetTime() * 100)) + 1) / 2) * 1;
		float continuosTime = glfwGetTime() / 6;
		ResourceManager::getSelf()->getShader(currentShader).setVec2("screenRes", WIDTH, HEIGHT);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("resolution", resolution);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("time", time);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("continuosTime", continuosTime);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *(camera->getCam()));
		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("invmodel", glm::inverse(model));
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("time", glfwGetTime());
		ResourceManager::getSelf()->getShader(currentShader).setInteger("SPP", SPP);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("frameCount", frameCount);
		//ResourceManager::getSelf()->getShader(currentShader).setInteger("levels", lbvh2->levels);
		//ResourceManager::getSelf()->getShader(currentShader).setInteger("nodesSize", lbvh2->nodesSize);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", camPos.x, camPos.y, camPos.z);

		(*renderer).render(ResourceManager::getSelf()->getModel("cubeTest"));
		frameCount++;
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

	void renderGradientBackground() {
		glDisable(GL_DEPTH_TEST);
		std::string currentShader = "gradientBackground";
		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, { -WIDTH / 2, HEIGHT / 2, 0 });
		model = glm::scale(model, { WIDTH / 2 , HEIGHT /2 , 1 });

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", orthoProj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);

		if (nightMode) {
			ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientUp", gradientUpNight.x, gradientUpNight.y, gradientUpNight.z);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientDown", gradientDownNight.x, gradientDownNight.y, gradientDownNight.z);
		}
		else {
			ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientUp", gradientUpDay.x, gradientUpDay.y, gradientUpDay.z);
			ResourceManager::getSelf()->getShader(currentShader).setVec3("gradientDown", gradientDownDay.x, gradientDownDay.y, gradientDownDay.z);
		}

		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));

		glEnable(GL_DEPTH_TEST);
	}

	void renderTex(Texture2D tex) {
		glDisable(GL_DEPTH_TEST);
		std::string currentShader = "simpleTexture";
		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, { -WIDTH/2, HEIGHT / 2, 0 });
		model = glm::scale(model, { -WIDTH / 2, -HEIGHT / 2, 1 });

		glActiveTexture(GL_TEXTURE0);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("tex", 0);
		tex.bind();

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", orthoProj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);

		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));
		glEnable(GL_DEPTH_TEST);
	}

	void renderRayPath() {
		std::string currentShader = "monocolor";
		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, {0,0,0});
		model = glm::scale(model, { 1,1,1});

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("projection", proj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *(camera->getCam()));
		ResourceManager::getSelf()->getShader(currentShader).setVec4("rgbColor", glm::vec4(1,0,0,1));

		(*renderer).renderLine(glm::vec3(0,0,0), glm::vec3(0,0,40));
	}

	void updateVariable(float deltaTime) {
	}

	void update(float deltaTime) {
		updateUI();
	}

	bool shouldClear() {
		if (*camera->getPosition() != *camera->getPreviousPosition() || (currentRenderMode != 1 && currentRenderMode != 2))
			return true;

		return false;
	}

	void renderNoise() {
		glDisable(GL_DEPTH_TEST);
		std::string currentShader = "randomVisualizer";
		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, { -WIDTH / 2, HEIGHT / 2, 0 });
		model = glm::scale(model, { -WIDTH / 2, -HEIGHT / 2, 1 });

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", orthoProj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);
		ResourceManager::getSelf()->getShader(currentShader).setVec2("screenRes", WIDTH, HEIGHT);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("time", glfwGetTime());

		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));
		glEnable(GL_DEPTH_TEST);
	}

	void render(){
		renderImGUI();
		fbo.bind();
		fbo.clear();
		renderGradientBackground();
		renderLightPosition();
		fbo.unbind();

		//TODO: should clear one?
		if (shouldClear()) {
			frameCount = 1;
			cloudFbo[0].bind();
			cloudFbo[0].clear();
			cloudFbo[0].unbind();

			cloudFbo[1].bind();
			cloudFbo[1].clear();
			cloudFbo[1].unbind();
		}

		int next = (currentCloudFrame + 1) % 2;
		cloudFbo[currentCloudFrame].bind();
		renderVolume(cloudTex[next]);
		cloudFbo[currentCloudFrame].unbind();

		renderTex(fboTex);
		renderTex(cloudTex[currentCloudFrame]);
		
		currentCloudFrame = next;

		//renderRayPath();
		//renderNoise();
	}
};

