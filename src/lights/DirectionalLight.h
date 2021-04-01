#pragma once
#include "lights/Light.h"

namespace narvalengine {
	class DirectionalLight: public Light{
	public:
		glm::vec3 direction; //points FROM the light

		/*
			Implemented only by Infinite Area Lights
		*/
		glm::vec3 Le() override{
			return le;
		}

		glm::vec3 Li() override{
			return le;
		}

		void sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
			wo.o = intersec.hitPoint;
			wo.d = -direction;

			lightPdf = 1; //TODO correct?
		}
	};
}

