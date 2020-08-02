#pragma once
#include "Material.h"
#include "RoughConductorBRDF.h"
#include "Math.h"

class MaterialBRDF : public Material {
public:
	glm::vec3 albedo;

	MaterialBRDF(BRDF *brdf, glm::vec3 albedo) {
		this->brdf = brdf;
		this->albedo = albedo;
	}

	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {

		glm::vec3 microfacetNormal = ((RoughConductorBRDF*)brdf)->microfacetSample();
		glm::vec3 u, v;
		glm::mat3 orthonormalCS = generateOrthonormalCS(hit.normal, u, v);
		glm::vec3 scatteredDir = toWorld(microfacetNormal, hit.normal, u, v);
		
		scatteredDir = glm::reflect(glm::normalize(rayIn.d), glm::normalize(scatteredDir));

		scattered = Ray(hit.p, scatteredDir);
		attenuation = albedo;

		return true;
	}

	glm::vec3 getAlbedo(float u, float v, glm::vec3 p) {
		return albedo;
	}

	MaterialBRDF();
	~MaterialBRDF();
};