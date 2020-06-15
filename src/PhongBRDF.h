#pragma once
#include "BRDF.h"

class PhongBRDF : public BRDF {
public:

	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {
	}

	PhongBRDF();
	~PhongBRDF();
};

