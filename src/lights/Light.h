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
		virtual glm::vec3 Le(Ray ray, glm::mat4 invTransformWCS) {
			return le;
		}

		virtual glm::vec3 Li() {
			return li;
		}

		/*
			Samples a point on this light primitive and returns its emitted Radiance, Ray pointing to this point and its Pdf
		*/
		virtual glm::vec3 sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
			lightPdf = 0;
			return li;
		}

		/*
			Samples a point on this light primitive and returns its emitted Radiance, Ray pointing from the Light and its Pdf
		*/
		virtual glm::vec3 sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf) {
			lightDirPdf = 0;
			lightPosPdf = 0;
			return le;
		}
	};
}