#pragma once
#include "materials/Texture.h"
#include "core/BSDF.h"
#include "core/Microfacet.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "materials/Material.h"

namespace narvalengine {
	/*
		Glossy BRDF using microfacet theory.
	*/
	class GlossyBSDF : public BxDF {
	public:
		MicrofacetDistribution *distribution;
		Fresnel *fresnel;

		GlossyBSDF(MicrofacetDistribution *distribution, Fresnel *fresnel);
		/*
			Samples scattered direction in Spherical Coordinates
		*/
		glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal);
		float pdf(glm::vec3 incoming, glm::vec3 scattered);
		// V = w_i = incoming
		// L = w_o = scattered
		//both in Spherical Coordinates (?)
		glm::vec3 eval(glm::vec3 incoming, glm::vec3 scattered, RayIntersection ri);
	};
}
