#pragma once
#include <glm/glm.hpp>

namespace narvalengine {
	class Primitive;
	class InstancedModel;

	struct RayIntersection {
		glm::vec3 hitPoint;
		glm::vec3 normal;
		glm::vec2 uv;
		float tNear, tFar;
		InstancedModel* instancedModel;
		Primitive* primitive;
	};

	class Ray {
	public:
		union { glm::vec3 o, origin; };
		union { glm::vec3 d, direction; };

		Ray() {};

		glm::vec3 getPointAt(float t) {
			return o + t * d;
		}

		Ray(glm::vec3 origin, glm::vec3 direction) {
			this->o = origin;
			this->d = direction;
		}
	};
}