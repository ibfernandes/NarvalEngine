#pragma once
#include "Ray.h"

class Material;
class Geometry;
struct Hit {
	float t;
	float tFar;
	int modelId;
	glm::vec3 p;
	glm::vec3 normal;
	Material *material;
	Geometry *collider;
};

class Geometry{
public:
	Material *material;
	virtual bool hit(Ray &r, float tMin, float tMax, Hit &hit);
	virtual Geometry* clone() const = 0;

	Geometry();
	~Geometry();
};

