#include "core/Camera.h"

namespace narvalengine {
	Camera::Camera() {
	}

	Camera::Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float vfov, float aspectRatio, float aperture, float focusDistance) {
		this->aperture = aperture;
		lensRadius = aperture / 2.0f;
		this->vfov = vfov;
		theta = glm::radians(vfov);
		halfHeight = tan(theta / 2.0f);
		halfWidth = aspectRatio * halfHeight;
		this->focusDistance = focusDistance;

		position = lookFrom;
		front = glm::normalize(lookFrom - lookAt);
		side = glm::normalize(glm::cross(up, front));
		this->up = glm::cross(front, side);

		lowerLeft = position - focusDistance * halfWidth * -side - focusDistance * halfHeight * this->up - focusDistance * -front;
		horizontal = 2.0f * focusDistance * halfWidth * -side;
		vertical = 2.0f * focusDistance * halfHeight * this->up;

		cam = glm::lookAt(position, position + front, up);
	}

	Camera::~Camera(){
	}

	void Camera::update() {
		previousPosition = position;

		if (InputManager::getSelf()->eventTriggered("MOVE_FORWARD"))
			position -= front * movementSpeed;
		else if (InputManager::getSelf()->eventTriggered("MOVE_BACKWARDS"))
			position += front * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("MOVE_RIGHT"))
			position += side * movementSpeed;
		else if (InputManager::getSelf()->eventTriggered("MOVE_LEFT"))
			position -= side * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("MOVE_UPWARDS"))
			position += up * movementSpeed;
		else if (InputManager::getSelf()->eventTriggered("MOVE_DOWNWARDS"))
			position -= up * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("PITCH_UP"))
			pitch -= rotationSpeed;
		else if (InputManager::getSelf()->eventTriggered("PITCH_DOWN"))
			pitch += rotationSpeed;

		if (InputManager::getSelf()->eventTriggered("YAW_RIGHT"))
			yaw += rotationSpeed;
		else if (InputManager::getSelf()->eventTriggered("YAW_LEFT"))
			yaw -= rotationSpeed;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = -cos(glm::radians(pitch)) * sin(glm::radians(yaw));

		front = glm::normalize(front);
		cam = glm::lookAt(position, position - front, up);
		side = -glm::cross(front, up);

		lowerLeft = position - focusDistance * halfWidth * -side - focusDistance * halfHeight * this->up - focusDistance * -front;
		horizontal = 2.0f * focusDistance * halfWidth * -side;
		vertical = 2.0f * focusDistance * halfHeight * this->up;
	}


	glm::mat4* Camera::getCam() {
		return &cam;
	}

	void Camera::setSpeed(float speed) {
		this->movementSpeed = speed / TARGET_UPS;
	}

	void  Camera::setPosition(glm::vec3 pos) {
		this->position = pos;
	}

	glm::vec3* Camera::getPosition() {
		return &position;
	}

	glm::vec3* Camera::getPreviousPosition() {
		return &previousPosition;
	}

	Ray Camera::getRayPassingThrough(float s, float t) {
		glm::vec3 rd = lensRadius * randomInUnitDisk();
		glm::vec3 offset = side * rd.x + up * rd.y;
		return Ray(position + offset, -glm::normalize(lowerLeft + s * horizontal + t * vertical - position - offset));
	}
}