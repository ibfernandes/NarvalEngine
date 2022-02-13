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
		glm::vec3 sample(const glm::vec3& incoming, const glm::vec3& normal, const RayIntersection& ri) override;
		float pdf(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) override;
		glm::vec3 eval(const glm::vec3 &incoming, const glm::vec3& scattered, const RayIntersection& ri) override;
	};

}

