#pragma once
#include "materials/Texture.h"
#include "core/BSDF.h"
#include "core/Microfacet.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "materials/Material.h"

namespace narvalengine {
	/**
	 * Glossy BRDF using microfacet theory.
	 */
	class GlossyBSDF : public BxDF {
	public:
		GGXDistribution *distribution;
		Fresnel *fresnel;

		GlossyBSDF(GGXDistribution *distribution, Fresnel *fresnel);
		glm::vec3 sample(const glm::vec3 &incoming, const glm::vec3& normal, const RayIntersection& ri) override;
		float pdf(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) override;
		glm::vec3 eval(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) override;
	};
}
