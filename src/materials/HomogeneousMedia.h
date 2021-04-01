#pragma once
#include "utils/Math.h"
#include "core/BSDF.h"
#include "materials/Medium.h"

namespace narvalengine {
	class HomogeneousMedia : public Medium {
	public:
		glm::vec3 absorption;
		glm::vec3 scattering;
		glm::vec3 extinction;
		float density;

		HomogeneousMedia(glm::vec3 scattering, glm::vec3 absorption, float density);
		~HomogeneousMedia();

		glm::vec3 Tr(Ray incoming, RayIntersection ri);
		glm::vec3 Tr(float distance);

		/*
			intersection must be inside the volume
		*/
		glm::vec3 sample(Ray incoming, Ray &scattered, RayIntersection intersection);
	};
}