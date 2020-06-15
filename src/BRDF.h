#pragma once
#include "Geometry.h"

class BRDF{
public:
	/*
		Calculates outgoing scattered ray and its pdf
	*/
	virtual bool sample(Ray &rayIn, Ray &scattered, glm::vec3 hitpoint, glm::vec3 normal, glm::vec3 albedo, float &pdfVal) = 0;

	/*
		Calculates the microfacet BRDF fr(wi, wo, theta) equation and returns it
	*/
	virtual glm::vec3 eval(Ray rayIn, Ray scattered, Hit hit, glm::vec3 attenuation) = 0;
	BRDF();
	~BRDF();
};

