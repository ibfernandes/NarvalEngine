#pragma once
#include "lights/Light.h"

namespace narvalengine {
	class DiffuseLight : public Light {
	public:
		glm::vec3 Li();
		glm::vec3 sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf);
		glm::vec3 sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf);
	};
}