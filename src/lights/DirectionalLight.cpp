#include "lights/DirectionalLight.h" 

namespace narvalengine{
	glm::vec3 DirectionalLight::Le(Ray ray, glm::mat4 invTransformWCS) {
		return le;
	}

	glm::vec3 DirectionalLight::Li() {
		return le;
	}

	glm::vec3 DirectionalLight::sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
		wo.o = intersec.hitPoint;
		wo.d = -direction;

		lightPdf = 1;

		return li;
	}

	glm::vec3 DirectionalLight::sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf) {
		lightDirPdf = 0;
		lightPosPdf = 0;

		return le;
	}
}
