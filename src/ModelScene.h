#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "FBO.h"
#include "ResourceManager.h"
#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class ModelScene: public Scene{
public:
	ModelScene();
	~ModelScene();
	float nearPlane = 1;
	float farPlane = 60000;
	float projAngle = 45;
	glm::mat4 proj, orthoProj, staticCam;
	glm::mat4 model = glm::mat4(1);
	Renderer *renderer;
	Camera *camera;
	std::string currentShader;
	std::string currentModel = "sponza";
	bool showMainMenu = true;
	bool *p_open = &showMainMenu;
	ImGuiWindowFlags window_flags = 0;
	GLint WIDTH, HEIGHT;
	FBO renderFBO, depthMapFBO;
	Texture2D renderTex, renderDepthTex, depthMapTex;
	glm::vec2 depthMapTexRes = glm::vec2(2048, 2048);

	//Lightning
	glm::vec3 lightPosition = glm::vec3(-100, 250, 50);
	glm::vec3 lightLookAt = glm::vec3(0, 0, 0);
	glm::vec3 lightDir = glm::vec3(0, 0, 0);
	glm::vec3 ambient = glm::vec3(0.1f);
	glm::vec3 diffuse = glm::vec3(0.5f);
	glm::vec3 specular = glm::vec3(0.0f);
	float Kc = 1.0f, Kl = 0.001f, Kq = 0.001f;

	int currentRenderMode = 0;
	const char* renderModes[2] = { "Phong", "PBR" };

	int currentViewMode = 0;
	const char* viewModes[3] = {"None", "Diffuse Map", "Normal Map" };

	bool gammaCorrection = true;
	float gamma = 2.2f;
	float exposure = 1.0f;

	bool normalMapping = true;

	glm::mat4 lightProjection, lightView, lightSpaceMatrix;

	void init(GLint width, GLint height, Renderer *r, Camera *c) {
		this->WIDTH = width;
		this->HEIGHT = height;
		camera = c;
		//c->setSpeed(40);
		renderer = r;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
		orthoProj = glm::ortho(0.f, -(float)width, 0.f, (float)height, -1.f, 1.f);
		glm::vec3 position(0, 0, 0);
		glm::vec3 front = { 0.0f, 0.0f, 1.0f };
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 side = glm::cross(front, up);
		staticCam = glm::lookAt(position, position + front, up);

		renderTex.generateWithData(width, height, GL_RGB16F, GL_RGB, GL_FLOAT, NULL);
		renderDepthTex.generateWithData(width, height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

		renderFBO.attachTextureColorAndDepthStencil(renderTex.id, renderDepthTex.id);

		depthMapTex.generateWithData(depthMapTexRes.x, depthMapTexRes.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		depthMapFBO.bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex.id, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		depthMapFBO.unbind();

		lightProjection = glm::ortho(-(float)width/2.0f, (float)width/2.0f, -(float)height/2.0f, (float)height/2.0f, 0.f, 1000.f);

		lightView = glm::lookAt(lightPosition, lightLookAt + front, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void renderImGUI();

	void renderPhong() {
		std::string currentShader = "phong";
		ResourceManager::getSelf()->getShader(currentShader).use();

		model = glm::mat4(1);
		if (currentModel == "sponza")
			model = glm::scale(model, glm::vec3(0.1f));

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *(camera->getCam()));
		ResourceManager::getSelf()->getShader(currentShader).setMat4("lightProj", lightProjection);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("lightCam", lightView);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", *((*camera).getPosition()));

		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].position", lightPosition);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightPoints[0].constant", Kc);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightPoints[0].linear", Kl);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightPoints[0].quadratic", Kq);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].ambient", ambient);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].diffuse", diffuse);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].specular", specular);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("numberOfLights", 1);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("viewMode", currentViewMode);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("normalMapping", normalMapping);

		glActiveTexture(GL_TEXTURE12);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("shadowMap", 12);
		depthMapTex.bind();
		
		for (Mesh m : ResourceManager::getSelf()->getModel(currentModel).meshes) 
			m.render(currentShader);
	}

	void update(float deltaTime) {
		lightView = glm::lookAtLH(lightPosition, lightLookAt + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void renderLightDepthMap() {
		std::string currentShader = "simpleLightDepth";
		ResourceManager::getSelf()->getShader(currentShader).use();

		model = glm::mat4(1);
		if (currentModel == "sponza")
			model = glm::scale(model, glm::vec3(0.1f));
		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("view", lightView);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", lightProjection);

		for (Mesh m : ResourceManager::getSelf()->getModel(currentModel).meshes) 
			m.renderVertices();
	}

	void renderPostProcessing(Texture2D tex) {
		glDisable(GL_DEPTH_TEST);
		std::string currentShader = "gamma";

		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, { -WIDTH / 2, HEIGHT / 2, 0 });
		model = glm::scale(model, { -WIDTH/ 2, -HEIGHT / 2, 1 });

		glActiveTexture(GL_TEXTURE0);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("tex", 0);
		tex.bind();

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", orthoProj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("gamma", gamma);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("exposure", exposure);

		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));
		glEnable(GL_DEPTH_TEST);
	}

	void renderLightPreview(Texture2D tex) {
		glDisable(GL_DEPTH_TEST);
		std::string currentShader = "simpleTexture";

		ResourceManager::getSelf()->getShader(currentShader).use();
		model = glm::mat4(1);
		model = glm::translate(model, { -WIDTH + 256/2, 256/2, 0 });
		model = glm::scale(model, { -256/2, -256/2, 1 });

		glActiveTexture(GL_TEXTURE0);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("tex", 0);
		tex.bind();

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", orthoProj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", staticCam);
		ResourceManager::getSelf()->getShader(currentShader).setInteger("mode", 4);

		(*renderer).render(ResourceManager::getSelf()->getModel("quadTest"));
		glEnable(GL_DEPTH_TEST);
	}

	void render() {
		renderImGUI();

		glViewport(0, 0, depthMapTexRes.x, depthMapTexRes.y);
		depthMapFBO.bind();
		glClear(GL_DEPTH_BUFFER_BIT);
			renderLightDepthMap();
		depthMapFBO.unbind();
			   
		glViewport(0, 0, WIDTH, HEIGHT);
		renderFBO.bind();
		renderFBO.clear();
			renderPhong();
		renderFBO.unbind();

		renderPostProcessing(renderTex);
		renderLightPreview(depthMapTex);

		
	}
};

