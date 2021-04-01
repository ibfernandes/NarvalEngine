#pragma once
#include "core/BSDF.h"
#include "materials/Medium.h"
#include "primitives/Primitive.h"

namespace narvalengine {
	class VolumeBSDF : public BxDF {
	public:
		PhaseFunction *phaseFunction;

		VolumeBSDF(PhaseFunction *phase) {
			this->phaseFunction = phase;
			bxdftype = BxDFType(BxDF_TRANSMISSION);
		}

		/*
			Sample scattered direction given incoming direction in SCS
		*/
		glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal) {
			return phaseFunction->sample(incoming);
		}

		/*
			Calculates the Probability Density Function(PDF) of sampling this scattered direction in SCS
		*/
		float pdf(glm::vec3 incoming, glm::vec3 scattered) {
			return phaseFunction->pdf(incoming, scattered);
		}

		/*
			Evals BSDF function Fr(x, w_i, w_o) in SCS
		*/
		glm::vec3 eval(glm::vec3 incoming, glm::vec3 scattered, RayIntersection ri) {
			return glm::vec3(phaseFunction->eval(incoming, scattered));
		}
	};

}

