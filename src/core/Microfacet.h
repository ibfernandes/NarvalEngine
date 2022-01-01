#pragma once
#include "utils/Math.h"
#include <glm/glm.hpp>

namespace narvalengine {

	/**
	*	All vectors must be in the Spherical Coordinate System (SCS).
	*/
	class MicrofacetDistribution {
	public:
		/**
		 * Distribution function. 0 <= D(wh) <= Infinity.
		 * 
		 * @param microfacetNormal in the Spherical Coordinate System (SCS).
		 * @return eval.
		 */
		virtual float D(glm::vec3 microfacetNormal) = 0;

		/**
		 * Shadowing-Masking function. 0 <= G(wi, wo) <= 1.
		 * 
		 * @param incoming in the Spherical Coordinate System (SCS).
		 * @param outgoing in the Spherical Coordinate System (SCS).
		 * @return eval.
		 */
		virtual float G(glm::vec3 incoming, glm::vec3 outgoing) = 0;

		/**
		 * Samples scattered direction (the function itself samples the microfacet normal in order to generate the scattered direction).
		 * 
		 * @param incoming in the Spherical Coordinate System (SCS).
		 * @param normal in the Spherical Coordinate System (SCS).
		 * @return scattered direction in the Spherical Coordinate System (SCS). 
		 */
		virtual glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal) = 0;

		/**
		 * Calculates the Probability Density Function(PDF) of sampling this scattered direction in SCS.
		 * 
		 * @param scattered
		 * @param microfacetNormal
		 * @return pdf.
		 */
		virtual float pdf(glm::vec3 scattered, glm::vec3 microfacetNormal) = 0;
	};

	class GGXDistribution : public MicrofacetDistribution {
	public:
		float alpha;

		float D(glm::vec3 microfacetNormal) override;
		float G(glm::vec3 incoming, glm::vec3 outgoing) override;
		glm::vec3 sample(glm::vec3 incoming, glm::vec3 surfaceNormal) override;
		float pdf(glm::vec3 scattered, glm::vec3 microfacetNormal) override;

	private:
		/**
		 * Samples a microfacet direction in the Spherical Coordinate System (SCS). Validated through Scilab.
		 * 
		 * @return microfacet normal direction in the Spherical Coordinate System (SCS),
		 */
		glm::vec3 sampleMicrofacet();

		/**
		 * Evaluates the geometric term using SchlickGGX. 
		 * From eq.Shlick - Beckmam: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html.
		 * 
		 * @param NdotV dot product between the microfacet normal and incoming ray.
		 * @return eval.
		 */
		float geometrySchlickGGX(float NdotV);
	};

}