#include "core/GlossyBSDF.h"

namespace narvalengine {
	GlossyBSDF::GlossyBSDF(GGXDistribution *distribution, Fresnel *fresnel) {
		this->distribution = distribution;
		this->fresnel = fresnel;
		bxdftype = BxDFType(BxDF_GLOSSY | BxDF_DIFFUSE);
	}

	glm::vec3 GlossyBSDF::sample(const glm::vec3& incoming, const glm::vec3& normal, const RayIntersection& ri) {
		float roughness = ri.primitive->material->sampleMaterial(TextureName::ROUGHNESS, ri.uv.x, ri.uv.y).x;
		distribution->alpha = roughness * roughness;
		return distribution->sample(incoming, normal);
	}

	float GlossyBSDF::pdf(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) {
		glm::vec3 microfacetNormal = glm::normalize(incoming + scattered);
		float roughness = ri.primitive->material->sampleMaterial(TextureName::ROUGHNESS, ri.uv.x, ri.uv.y).x;
		distribution->alpha = roughness * roughness;
		return distribution->pdf(scattered, microfacetNormal);

	}

	glm::vec3 GlossyBSDF::eval(const glm::vec3& incoming, const glm::vec3& scattered, const RayIntersection& ri) {
		float roughness = ri.primitive->material->sampleMaterial(TextureName::ROUGHNESS, ri.uv.x, ri.uv.y).x;
		distribution->alpha = roughness * roughness;

		glm::vec3 H = glm::normalize(incoming + scattered);
		float HdotV = glm::dot(incoming, H);
		float NdotV = cosTheta(incoming);
		float NdotL = cosTheta(scattered);

		float  D = distribution->D(H);
		float  G = distribution->G(incoming, scattered);
		glm::vec3 F = fresnel->eval(HdotV);

		glm::vec3 numerator = D * G * F;
		float denominator = 4.0f * glm::max(NdotV, 0.0f) * glm::max(NdotL, 0.0f);
		glm::vec3 specular = numerator / glm::max(denominator, float(EPSILON));
		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0) - kS;

		float metallic = ri.primitive->material->sampleMaterial(METALLIC, ri.uv.x, ri.uv.y).x;
		glm::vec3 albedo = ri.primitive->material->sampleMaterial(ALBEDO, ri.uv.x, ri.uv.y);

		kD *= 1.0 - metallic;
		return ((albedo / float(PI)) * kD + specular) * NdotL;
	}
}