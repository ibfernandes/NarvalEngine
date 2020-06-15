#pragma once
#include "Material.h"
#include "Math.h"

class MetalMaterial : public Material {
public:
	glm::vec3 albedo;

	MetalMaterial(glm::vec3 albedo) {
		this->albedo = albedo;
	};

	/*
		NOTE: we could just as well only scatter with some probability p and have attenuation be albedo/p.
	*/
	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered,float &pdf) {
		glm::vec3 target = hit.p + hit.normal + sampleUnitSphere();
		glm::vec3 reflected = reflect(glm::normalize(rayIn.direction), hit.normal);
		scattered = Ray(hit.p, reflected);
		attenuation = albedo;
		return (glm::dot(scattered.direction, hit.normal) > 0);
	};
};