#pragma once
#include "Ray.h"

class Material;
struct Hit {
	float t;
	int modelId;
	glm::vec3 p;
	glm::vec3 normal;
	Material *material;
};

class Geometry{
public:
	Material *material;
	virtual bool hit(Ray &r, float tMin, float tMax, Hit &hit);
	virtual Geometry* clone() const = 0;

	Geometry();
	~Geometry();
};

