#include "core/Camera.h"

namespace narvalengine {
	Camera::Camera() {
	}

	Camera::Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float vfov, float aspectRatio, float aperture, float focusDistance) {
		this->aperture = aperture;
		lensRadius = aperture / 2.0f;
		this->vfov = vfov;
		halfHeight = tan(glm::radians(vfov) / 2.0f);
		halfWidth = aspectRatio * halfHeight;
		this->focusDistance = focusDistance;

		position = lookFrom;
		lookAtPoint = lookAt;
		this->up = up;
		front = glm::normalize(lookFrom - lookAt);
		side = glm::normalize(glm::cross(up, front));

		lowerLeft = position - focusDistance * halfWidth * -side - focusDistance * halfHeight * this->up - focusDistance * -front;
		horizontal = 2.0f * focusDistance * halfWidth * -side;
		vertical = 2.0f * focusDistance * halfHeight * this->up;

		cam = glm::lookAt(position, position + front, up);
	}

	Camera::~Camera(){
	}

	void Camera::changeHeading(float deg) {
		if (pitch > 90 && pitch < 270 || (pitch < -90 && pitch > -270)) 
			heading -= deg;	
		else 
			heading += deg;
		
		if (heading > 360.0f) 
			heading -= 360.0f;
		else if (heading < -360.0f) 
			heading += 360.0f;
	}

	void Camera::changePitch(float deg) {
		pitch += deg;

		if (pitch > 60)
			pitch = 60;
		else if (pitch < -60)
			pitch = -60;

		if (pitch > 360.0f) 
			pitch -= 360.0f;
		else if (pitch < -360.0f) 
			pitch += 360.0f;
	}

	void Camera::moveUsingMouse() {
		glm::vec2 mousePos = InputManager::getSelf()->getMousePosition();
		glm::vec2 delta = previousMousePos - mousePos;

		if (InputManager::getSelf()->eventTriggered("MOUSE_MOVE_CAMERA")) {
			pitch = prevPitch;
			heading = prevHeading;
			changeHeading(0.08f * delta.x);
			changePitch(0.08f * delta.y);
		}else {
			previousMousePos = InputManager::getSelf()->getMousePosition();
			prevPitch = pitch;
			prevHeading = heading;
		}
	}

	void Camera::update() {
		previousPosition = position;

		if (InputManager::getSelf()->eventTriggered("MOVE_FORWARD"))
			position += front * movementSpeed;
		else if (InputManager::getSelf()->eventTriggered("MOVE_BACKWARDS"))
			position -= front * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("MOVE_RIGHT"))
			position += side * movementSpeed;
		else if (InputManager::getSelf()->eventTriggered("MOVE_LEFT"))
			position -= side * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("MOVE_UPWARDS"))
			position += up * movementSpeed;
		else if (InputManager::getSelf()->eventTriggered("MOVE_DOWNWARDS"))
			position -= up * movementSpeed;

		if (InputManager::getSelf()->eventTriggered("PITCH_UP"))
			changePitch(rotationSpeed);
		else if (InputManager::getSelf()->eventTriggered("PITCH_DOWN"))
			changePitch(-rotationSpeed);

		if (InputManager::getSelf()->eventTriggered("YAW_RIGHT"))
			changeHeading(rotationSpeed);
		else if (InputManager::getSelf()->eventTriggered("YAW_LEFT"))
			changeHeading(-rotationSpeed);

		moveUsingMouse();

		side = glm::cross(front, up);
		glm::quat pitch_quat = glm::angleAxis(glm::radians(pitch), side);
		glm::quat heading_quat = glm::angleAxis(glm::radians(heading), up);
		glm::quat temp = glm::cross(pitch_quat, heading_quat);
		temp = glm::normalize(temp);
		front = glm::rotate(temp, glm::vec3(0,0,1));

		lookAtPoint = position + front * 1.0f;

		cam = glm::lookAt(position, position + front, up);

		lowerLeft = position - focusDistance * halfWidth * -side - focusDistance * halfHeight * this->up - focusDistance * front;
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

	Ray Camera::getRayPassingThrough(float x, float y) {
		glm::vec3 rd = lensRadius * randomInUnitDisk();
		glm::vec3 offset = side * rd.x + up * rd.y;
		return Ray(position + offset, -glm::normalize(lowerLeft + x * horizontal + y * vertical - position - offset));
	}
}