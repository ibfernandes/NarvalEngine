#pragma once
#include "InputManager.h"
#include "defines.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

class Camera
{
public:
	Camera();
	~Camera();
	
	void init() {

	}

	void update() {
		if (InputManager::getSelf()->eventTriggered("MOVE_FORWARD"))
			position += front * movementSpeed;
		if (InputManager::getSelf()->eventTriggered("MOVE_BACKWARDS")) {
			position -= front * movementSpeed;
		}

		if (InputManager::getSelf()->eventTriggered("MOVE_RIGHT"))
			position += side * movementSpeed;
		if (InputManager::getSelf()->eventTriggered("MOVE_LEFT"))
			position -= side * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("MOVE_UPWARDS"))
			position += up * movementSpeed;
		if (InputManager::getSelf()->eventTriggered("MOVE_DOWNWARDS"))
			position -= up * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("PITCH_UP"))
			pitch += rotationSpeed;
		if (InputManager::getSelf()->eventTriggered("PITCH_DOWN"))
			pitch -= rotationSpeed;

		if (InputManager::getSelf()->eventTriggered("YAW_RIGHT"))
			yaw += rotationSpeed;
		if (InputManager::getSelf()->eventTriggered("YAW_LEFT"))
			yaw -= rotationSpeed;


		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		front = glm::normalize(front);

		cam = glm::lookAt(position, position + front, up);
	}


	glm::mat4 *getCam() {
		return &cam;
	}

	void setPosition(glm::vec3 pos) {
		this->position = pos;
	}

	glm::vec3 *getPosition() {
		return &position;
	}

private:
	glm::vec3 position;
	glm::vec3 front = { 0.0f, 0.0f, 1.0f };
	glm::vec3 up = { 0.0f, 1.0f, 0.0f };
	glm::vec3 side = glm::cross(front, up);
	glm::mat4 cam;
	float yaw = 89;
	float pitch = 0;
	float movementSpeed = 1.0f / TARGET_UPS;
	float rotationSpeed = 10.0f / TARGET_UPS;
};

