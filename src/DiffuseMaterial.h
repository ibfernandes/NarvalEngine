#pragma once
#include "Material.h"
#include "Math.h"

class DiffuseMaterial : public Material {
public:
	glm::vec3 albedo;

	DiffuseMaterial(glm::vec3 albedo) {
		this->albedo = albedo;
	};

	/*
		Note we could just as well only scatter with some probability p and have attenuation be albedo/p. Your choice. 
	*/
	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {
		glm::vec3 hemisphereDir = sampleUnitHemisphere();

		glm::vec3 u, v;
		glm::mat3 orthonormalCS = generateOrthonormalCS(hit.normal, u, v);
		glm::vec3 scatteredDir = toWorld(orthonormalCS, hemisphereDir);

		//glm::vec3 target = hit.p + hit.normal + sampleUnitSphere();
		//scatteredDir = target - hit.p;
		scattered = Ray(hit.p, scatteredDir);
		attenuation = albedo;
		return true;
	};
};