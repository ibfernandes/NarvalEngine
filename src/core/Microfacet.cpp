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

	//From eq. Shlick-Beckmam: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
	float GGXDistribution::geometrySchlickGGX(float NdotV) {
		float k = alpha / 2.0f;

		float nom = NdotV;
		float denom = glm::max(EPSILON, NdotV * (1.0 - k) + k);

		return nom / denom;
	}

	float GGXDistribution::G(glm::vec3 incoming, glm::vec3 outgoing) {
		glm::vec3 normal = glm::normalize(-incoming + outgoing);
		float NoV = glm::dot(normal, -incoming);
		float NoL = glm::dot(normal, outgoing);
		float NdotV = glm::max(NoV, 0.0f);
		float NdotL = glm::max(NoL, 0.0f);
		float ggx1 = geometrySchlickGGX(NdotV);
		float ggx2 = geometrySchlickGGX(NdotL);

		return ggx1 * ggx2;
	}

	/*
		Samples microfacet in Spherical Coordinates
		Validated through Scilab.
	*/
	glm::vec3 GGXDistribution::sampleMicrofacet() {
		glm::vec2 r = glm::vec2(random(), random());

		// GGX NDF sampling
		float a2 = alpha * alpha;
		//if r = 0, sqrt acos(inf) = NaN
		//if r = 1, sqrt acos(0) = sqrt PI/2 = 1.57
		float theta = std::acos(
			glm::min(
			1.0f,
			std::sqrt(
			(1.0f - r.x) / glm::max(float(EPSILON), (r.x * (a2 - 1.0f) + 1.0f))
			)
		)
		);
		//full circle, 0 < x < 2PI
		float phi = TWO_PI * r.y;

		// Get our GGX NDF sample (i.e., the half vector)
		return glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
	}

	/*
		Samples scattered direction in Spherical Coordinates
	*/
	glm::vec3 GGXDistribution::sample(glm::vec3 incoming, glm::vec3 surfaceNormal) {
		glm::vec3 microfacetNormal = sampleMicrofacet();
		
		//make y axis point up instead of the default z up from SCS
		//float y = microfacetNormal.y;
		//microfacetNormal.y = microfacetNormal.z;
		//microfacetNormal.z = y;
		
		//convert
		//glm::vec3 ss, ts;
		//generateOrthonormalCS(surfaceNormal, ss, ts);
		//microfacetNormal = toLCS(microfacetNormal, surfaceNormal, ss, ts);

		//printVec3(microfacetNormal, "microfacet: ");
		glm::vec3 scatteredDir = glm::reflect(glm::normalize(incoming), glm::normalize(microfacetNormal));
		//TODO: after eq 8.5, easier way to calc reflect for SCS: http://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission.html

		//if (incoming.x < 0 && incoming.y < 0 && incoming.z < 0 && scatteredDir.x < 0 && scatteredDir.y < 0 && scatteredDir.z < 0)
		//	float d = 0;

		return scatteredDir;
	}

	float GGXDistribution::pdf(glm::vec3 scattered, glm::vec3 microfacetNormal) {
		//float NdotH = glm::dot(N, H);
		float NdotH = cosTheta(microfacetNormal);
		float VdotH = glm::dot(scattered, microfacetNormal);

		float D_ggxpdf = D(microfacetNormal) * NdotH;

		return D_ggxpdf / (4 * VdotH);
	}
}