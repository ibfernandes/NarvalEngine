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
			pitch -= rotationSpeed;
		if (InputManager::getSelf()->eventTriggered("PITCH_DOWN"))
			pitch += rotationSpeed;

		float pitchCos = cos(glm::radians(pitch));
		float pitchSin = sin(glm::radians(pitch));

		glm::mat4 rp(1);
		rp[1][1] = pitchCos;
		rp[1][2] = -pitchSin;
		rp[2][1] = pitchSin;
		rp[2][2] = pitchCos;

		if (InputManager::getSelf()->eventTriggered("YAW_RIGHT"))
			yaw += rotationSpeed;
		if (InputManager::getSelf()->eventTriggered("YAW_LEFT"))
			yaw -= rotationSpeed;

		float yawCos = cos(glm::radians(yaw));
		float yawSin = sin(glm::radians(yaw));

		glm::mat4 ry(1);
		ry[0][0] = yawCos;
		ry[0][2] = -yawSin;
		ry[2][0] = yawSin;
		ry[2][2] = yawCos;

		cam = glm::lookAt(position, position + front, up) * rp * ry;
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
	float yaw = 0;
	float pitch = 0;
	float movementSpeed = 10.0f / TARGET_UPS;
	float rotationSpeed = 40.0f / TARGET_UPS;
};

