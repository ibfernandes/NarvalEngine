#pragma once
#include "Ray.h"
#include "Model.h"
#include "BRDF.h"

class Material {
public:
	bool hasSpecularLobe = false;
	BRDF *brdf;

	float schlick(float cos, float refractionIndex) {
		float r0 = (1 - refractionIndex) / (1 + refractionIndex);
		r0 = r0 * r0;
		return r0 + (1 - r0)*pow((1 - cos), 5);
	}

	glm::vec3 reflect(glm::vec3 &v, glm::vec3 &n) {
		return v - (2.0f * glm::dot(v,n) * n);
	}
	
	bool refract(glm::vec3 &v, glm::vec3 &n, float ni, glm::vec3 &refracted) {
		glm::vec3 uv = glm::normalize(v);
		float d = glm::dot(uv, n);
		float discriminant = 1.0f - ni * ni * (1 - d * d);
		if (discriminant > 0) {
			refracted = ni * (uv - n * d) - n * sqrt(discriminant);
			return true;
		}
		else
			return false;
	}
	virtual glm::vec3 emitted(float u, float v, glm::vec3 p) {
		return glm::vec3(0);
	}

	virtual bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) = 0;
};