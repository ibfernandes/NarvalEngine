#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class ModelScene: public Scene{
public:
	ModelScene();
	~ModelScene();
	float nearPlane = 1;
	float farPlane = 60000;
	float projAngle = 45;
	glm::mat4 proj;
	glm::mat4 model = glm::mat4(1);
	Renderer *renderer;
	Camera *camera;
	std::string currentShader;
	std::string currentModel = "sponza";

	//Lightning
	glm::vec3 lightPosition = glm::vec3(0.0, 1.6, 4.0);
	glm::vec3 lightColor = glm::vec3(1.0, 1.0, 1.0);

	void init(GLint width, GLint height, Renderer *r, Camera *c) {
		camera = c;
		renderer = r;
		proj = glm::perspective(glm::radians(projAngle), (GLfloat)width / (GLfloat)height, nearPlane, farPlane);
	}

	void renderModel() {
		std::string currentShader = "phong";
		ResourceManager::getSelf()->getShader(currentShader).use();

		ResourceManager::getSelf()->getShader(currentShader).setMat4("model", model);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("proj", proj);
		ResourceManager::getSelf()->getShader(currentShader).setMat4("cam", *(camera->getCam()));
		ResourceManager::getSelf()->getShader(currentShader).setVec3("cameraPosition", *((*camera).getPosition()));

		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].position", lightPosition);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightPoints[0].constant", 1.0f);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightPoints[0].linear", 1.0f);
		ResourceManager::getSelf()->getShader(currentShader).setFloat("lightPoints[0].quadratic", 1.0f);
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].ambient", glm::vec3(0.5f));
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].diffuse", glm::vec3(0.5f));
		ResourceManager::getSelf()->getShader(currentShader).setVec3("lightPoints[0].specular", glm::vec3(0.5f));
		ResourceManager::getSelf()->getShader(currentShader).setInteger("numberOfLights", 1);

		for (Mesh m : ResourceManager::getSelf()->getModel(currentModel).meshes) {
			m.render(currentShader);
		}
	}

	void render() {
		renderModel();
	}
};

