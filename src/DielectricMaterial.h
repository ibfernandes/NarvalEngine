#pragma once
#include "Material.h"

class DielectricMaterial: public Material {
public:
	float refractionIndex;

	DielectricMaterial(float refractionIndex) {
		this->refractionIndex = refractionIndex;
	}

	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {
		glm::vec3 outwardNormal;
		glm::vec3 refracted;
		glm::vec3 reflected = reflect(rayIn.direction, hit.normal);
		float rIndex;
		attenuation = glm::vec3(1, 1, 1);

		float cos;
		float reflectProb;

		//if rayIn and normal are not perpendicullar
		if (glm::dot(rayIn.direction, hit.normal) > 0) {
			outwardNormal = -hit.normal;
			rIndex = refractionIndex;
			cos = refractionIndex * glm::dot(rayIn.direction, hit.normal) / rayIn.direction.length();
		}else {
			outwardNormal = hit.normal;
			rIndex = 1.0f/ refractionIndex;
			cos = -glm::dot(rayIn.direction, hit.normal) / rayIn.direction.length();
		}

		if (refract(rayIn.direction, outwardNormal, rIndex, refracted)) 
			reflectProb = schlick(cos, refractionIndex);
		else
			reflectProb = 1.0f;

		if (random() < reflectProb)
			scattered = Ray(hit.p, reflected);
		else
			scattered = Ray(hit.p, refracted);

		return true;
	}
};