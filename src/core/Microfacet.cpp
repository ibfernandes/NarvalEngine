#include "Microfacet.h"

namespace narvalengine {

	float GGXDistribution::D(glm::vec3 microfacetNormal) {
		float a2 = alpha * alpha;
		float NdotH = cosTheta(microfacetNormal);
		float NdotH2 = NdotH * NdotH;

		float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
		denom = glm::max(EPSILON, PI * denom * denom);

		return a2 / denom;
	}

	float GGXDistribution::geometrySchlickGGX(float NdotV) {
		float k = alpha / 2.0f;

		float nom = NdotV;
		float denom = glm::max(EPSILON, NdotV * (1.0 - k) + k);

		return nom / denom;
	}

	float GGXDistribution::G(glm::vec3 incoming, glm::vec3 outgoing) {
		glm::vec3 normal = glm::normalize(incoming + outgoing);
		float NoV = glm::dot(normal, incoming);
		float NoL = glm::dot(normal, outgoing);
		float NdotV = glm::max(NoV, 0.0f);
		float NdotL = glm::max(NoL, 0.0f);
		float ggx1 = geometrySchlickGGX(NdotV);
		float ggx2 = geometrySchlickGGX(NdotL);

		return ggx1 * ggx2;
	}

	glm::vec3 GGXDistribution::sampleMicrofacet() {
		glm::vec2 r = glm::vec2(random(), random());

		// GGX NDF sampling.
		float a2 = alpha * alpha;
		//if r = 0, sqrt acos(inf) = NaN.
		//   r = 1, sqrt acos(0) = sqrt PI/2 = 1.57.
		float theta = std::acos(glm::min(1.0f, std::sqrt((1.0f - r.x) / glm::max(float(EPSILON), (r.x * (a2 - 1.0f) + 1.0f)))));
		//Full circle, 0 < x < 2PI.
		float phi = TWO_PI * r.y;

		// Get our GGX NDF sample (i.e., the half vector).
		return glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
	}

	glm::vec3 GGXDistribution::sample(glm::vec3 incoming, glm::vec3 surfaceNormal) {
		glm::vec3 microfacetNormal = sampleMicrofacet();
		glm::vec3 scatteredDir = glm::reflect(glm::normalize(incoming), glm::normalize(microfacetNormal));
		return scatteredDir;
	}

	float GGXDistribution::pdf(glm::vec3 scattered, glm::vec3 microfacetNormal) {
		float NdotH = cosTheta(microfacetNormal);
		float VdotH = glm::dot(scattered, microfacetNormal);

		float D_ggxpdf = D(microfacetNormal) * NdotH;

		return D_ggxpdf / (4 * VdotH);
	}
}