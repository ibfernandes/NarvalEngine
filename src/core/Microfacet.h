#pragma once
#include "utils/Math.h"
#include <glm/glm.hpp>

namespace narvalengine {

	/*
		All vectors must be in SCS.
	*/
	class MicrofacetDistribution {
	public:
		/*
			Distribution function. 0 <= D(wh) <= Infinity.
		*/
		virtual float D(glm::vec3 microfacetNormal) = 0;
		/*
			Shadowing-Masking function. 0 <= G(wi, wo) <= 1
		*/
		virtual float G(glm::vec3 incoming, glm::vec3 outgoing) = 0;
		/*
			Samples scattered direction (the function itself samples the microfacet normal in order to generate the scattered direction)
		*/
		virtual glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal) = 0;
		virtual float pdf(glm::vec3 scattered, glm::vec3 microfacetNormal) = 0;
	};

	class GGXDistribution : public MicrofacetDistribution {
	public:
		float alpha;

		float D(glm::vec3 microfacetNormal);
		//From eq. Shlick-Beckmam: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
		float geometrySchlickGGX(float NdotV);
		float G(glm::vec3 incoming, glm::vec3 outgoing);
		/*
			Samples microfacet in Spherical Coordinates
		*/
		glm::vec3 sampleMicrofacet();

		/*
			Samples scattered direction in Spherical Coordinates
		*/
		glm::vec3 sample(glm::vec3 incoming, glm::vec3 surfaceNormal);

		float pdf(glm::vec3 scattered, glm::vec3 microfacetNormal);
	};

}