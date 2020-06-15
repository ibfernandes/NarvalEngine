#pragma once
#include "Geometry.h"
#include "Material.h"
#include <glm/glm.hpp>

class Sphere : public Geometry {
public:
	glm::vec3 center;
	float radius;

	Sphere(glm::vec3 center, float radius) {
		this->center = center;
		this->radius = radius;
	};

	Sphere(glm::vec3 center, float radius, Material *material) {
		this->center = center;
		this->radius = radius;
		this->material = material;
	};

	/*
		Sphere equation in vector form.
	*/

	bool hit(Ray &r, float tMin, float tMax, Hit &hit) {
		glm::vec3 oc = r.origin - center;
		float a = glm::dot(r.direction, r.direction);
		float b = glm::dot(oc, r.direction);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - a * c;

		if (discriminant >= 0) {
			float temp = (-b - sqrt(discriminant)) / a;
			if (temp > tMin && temp < tMax) {
				hit.t = temp;
				hit.p = r.getPointAt(hit.t);
				hit.normal = glm::normalize((hit.p - center)/radius);
				hit.material = material;
				return true;
			}

			temp = (-b + sqrt(discriminant)) / a;
			if (temp > tMin && temp < tMax) {
				hit.t = temp;
				hit.p = r.getPointAt(hit.t);
				hit.normal = glm::normalize((hit.p - center)/radius);
				hit.material = material;
				return true;
			}
		}

		return false;
	}

	virtual Sphere *clone() const {
		return new Sphere(*this);
	}
};