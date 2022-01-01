#pragma once
#include "lights/Light.h"

namespace narvalengine {
	class DirectionalLight: public Light{
	public:
		//Direction pointing FROM THE LIGHT.
		glm::vec3 direction;

		glm::vec3 Le(Ray ray, glm::mat4 invTransformWCS);
		glm::vec3 Li();
		glm::vec3 sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf);
		glm::vec3 sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf);
	};
}

