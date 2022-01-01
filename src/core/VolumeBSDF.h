#pragma once
#include "core/BSDF.h"
#include "materials/Medium.h"
#include "primitives/Primitive.h"

namespace narvalengine {
	class VolumeBSDF : public BxDF {
	public:
		PhaseFunction *phaseFunction;

		VolumeBSDF(PhaseFunction* phase);
		~VolumeBSDF();
		glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal) override;
		float pdf(glm::vec3 incoming, glm::vec3 scattered) override;
		glm::vec3 eval(glm::vec3 incoming, glm::vec3 scattered, RayIntersection ri) override;
	};

}

