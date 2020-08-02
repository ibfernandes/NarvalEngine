#pragma once
#include "Geometry.h"

class BRDF{
public:
	/*
		Calculates outgoing scattered ray and its pdf
	*/
	virtual bool sample(Ray rayIn, Ray &scattered, Hit h) = 0;

	/*
		Calculates the microfacet BRDF fr(wi, wo, theta) equation and returns it
	*/
	virtual glm::vec3 eval(Ray rayIn, Ray scattered, Hit hit, float &pdf) = 0;

	virtual float pdf() = 0;

	BRDF();
	~BRDF();
};

