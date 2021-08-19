#pragma once
#include "io/InputManager.h"
#include "primitives/Ray.h"
#include "utils/Math.h"
#include "defines.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace narvalengine {

	struct Lens {
		float mm;
		float aperture;
	};

	class Camera{
	public:
		glm::vec3 position;
		glm::vec3 previousPosition;
		glm::vec2 previousMousePos;
		glm::vec3 lookAtPoint;
		glm::vec3 front = { 0.0f, 0.0f, 1.0f };
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 side = glm::cross(front, up);
		glm::mat4 cam;
		float vfov;
		float aperture;
		float pitch = 0;
		float heading = 0;
		float prevPitch = 0;
		float prevHeading = 0;
		float movementSpeed = 6.0f / TARGET_UPS;
		float rotationSpeed = 180.0f / TARGET_UPS;

		float lensRadius;
		float halfHeight;
		float halfWidth;
		float focusDistance;

		glm::vec3 lowerLeft;
		glm::vec3 horizontal;
		glm::vec3 vertical;

	public:
		Camera();
		~Camera();
		Camera(glm::vec3 lookFrom, glm::vec3 lookAt, glm::vec3 up, float vfov, float aspectRatio, float aperture, float focusDistance);
		void changeHeading(float deg);
		void changePitch(float deg);
		void moveUsingMouse();
		void update();
		glm::mat4 *getCam();
		void setSpeed(float speed);
		void setPosition(glm::vec3 pos);
		glm::vec3 *getPosition();
		glm::vec3 *getPreviousPosition();
		Ray getRayPassingThrough(float s, float t);
	};

}