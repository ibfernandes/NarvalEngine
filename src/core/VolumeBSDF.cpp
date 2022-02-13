#include "VolumeBSDF.h"

namespace narvalengine {
	VolumeBSDF::VolumeBSDF(PhaseFunction* phase) {
		this->phaseFunction = phase;
		bxdftype = BxDFType(BxDF_TRANSMISSION);
	}

	VolumeBSDF::~VolumeBSDF(){
		delete phaseFunction;
	}

	glm::vec3 VolumeBSDF::sample(const glm::vec3& incoming, const glm::vec3& normal, const RayIntersection& ri) {
		return phaseFunction->sample(incoming);
	}

	float VolumeBSDF::pdf(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) {
		return phaseFunction->pdf(incoming, scattered);
	}

	glm::vec3 VolumeBSDF::eval(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) {
		return glm::vec3(phaseFunction->eval(incoming, scattered));
	}
}