#pragma once
#include "Material.h"

class DiffuseLight: public Material
{
public:
	glm::vec3 color;

	virtual glm::vec3 emitted(float u, float v, glm::vec3 p) {
		return color;
	}

	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {
		return false;
	}

	DiffuseLight(glm::vec3 color);
	~DiffuseLight();
};

