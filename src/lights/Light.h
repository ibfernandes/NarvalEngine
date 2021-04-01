#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"

namespace narvalengine {
	class Light {
	public:
		Primitive *primitive;
		glm::vec3 li = glm::vec3(0);
		glm::vec3 le = glm::vec3(0);

		/*
			Implemented only by Infinite Area Lights
		*/
		virtual glm::vec3 Le() {
			return le;
		}

		virtual glm::vec3 Li() {
			return li;
		}

		/*
			Samples a point on this light primitive and returns its emitted Radiance, Ray pointing to this point and its Pdf
		*/
		virtual void sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
			lightPdf = 0;
		}
	};
}