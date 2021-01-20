#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"

namespace narvalengine {
	class Light {
	public:
		Primitive *primitive;

		/*
			Implemented only by Infinite Area Lights
		*/
		virtual glm::vec3 Le(Ray ray) {
			return glm::vec3(0);
		}

		virtual glm::vec3 Li() {
			return glm::vec3(0);
		}

		/*
			Samples a point on this light primitive and returns its emitted Radiance, Ray pointing to this point and its Pdf
		*/
		void sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray &wo, float &lightPdf);
	};
}