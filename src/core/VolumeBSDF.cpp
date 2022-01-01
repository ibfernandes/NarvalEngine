#include "VolumeBSDF.h"

namespace narvalengine {
	VolumeBSDF::VolumeBSDF(PhaseFunction* phase) {
		this->phaseFunction = phase;
		bxdftype = BxDFType(BxDF_TRANSMISSION);
	}

	VolumeBSDF::~VolumeBSDF(){
		delete phaseFunction;
	}

	glm::vec3 VolumeBSDF::sample(glm::vec3 incoming, glm::vec3 normal) {
		return phaseFunction->sample(incoming);
	}

	float VolumeBSDF::pdf(glm::vec3 incoming, glm::vec3 scattered) {
		return phaseFunction->pdf(incoming, scattered);
	}

	glm::vec3 VolumeBSDF::eval(glm::vec3 incoming, glm::vec3 scattered, RayIntersection ri) {
		return glm::vec3(phaseFunction->eval(incoming, scattered));
	}
}