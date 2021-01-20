#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "materials/Material.h"

namespace narvalengine {

	class PhaseFunction {
	public:
		/*
			Returns scattered direction
		*/
		virtual glm::vec3 sample(glm::vec3 incomingDir) = 0;
		virtual float pdf() = 0;
		/*
			Evals the phase function
		*/
		virtual float eval() = 0;
	};

	class IsotropicPhaseFunction : public PhaseFunction {
	public:
		glm::vec3 sample(glm::vec3 incomingDir) {
			return sampleUnitSphere(random(), random());
		}

		float pdf() {
			return 1.0f / (4.0f * PI);
		}

		float eval() {
			return 1.0f / (4.0f * PI);
		}
	};

	class Medium {
	public:
		virtual glm::vec3 Tr(Ray incoming, RayIntersection ri) = 0;
		virtual glm::vec3 Tr(float distance) = 0;
		virtual glm::vec3 sample(Ray incoming, Ray &scattered, RayIntersection intersection) = 0;
	};
}

