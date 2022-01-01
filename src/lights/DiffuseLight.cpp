#include "lights/DiffuseLight.h"

namespace narvalengine {
	glm::vec3 DiffuseLight::Li() {
		return li;
	}

	glm::vec3 DiffuseLight::sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
		wo.o = intersec.hitPoint;

		glm::vec3 pointOnLightPrimitive = primitive->samplePointOnSurface(intersec, transformToWCS); //sample point and pdf needs intersec, as its sample is "view" dependent

		glm::vec3 d = pointOnLightPrimitive - wo.o;
		float distanceSquared = glm::length(d);

		wo.d = glm::normalize(d);
		lightPdf = primitive->pdf(intersec, transformToWCS);

		return li;
	}

	glm::vec3 DiffuseLight::sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf) {
		lightDirPdf = 0;
		lightPosPdf = 0;

		return le;
	}
}
