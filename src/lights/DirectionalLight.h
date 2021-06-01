#pragma once
#include "lights/Light.h"

namespace narvalengine {
	class DirectionalLight: public Light{
	public:
		glm::vec3 direction; //points FROM the light

		/*
			Implemented only by Infinite Area Lights
		*/
		glm::vec3 Le(Ray ray, glm::mat4 invTransformWCS) override{
			return le;
		}

		glm::vec3 Li() override{
			return le;
		}

		glm::vec3 sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
			wo.o = intersec.hitPoint;
			wo.d = -direction;

			lightPdf = 1; //TODO correct?

			return li;
		}

		glm::vec3 sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf) {
			lightDirPdf = 0;
			lightPosPdf = 0;

			return le;
		}
	};
}

