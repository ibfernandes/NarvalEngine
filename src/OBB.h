#pragma once
#include "Geometry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//source: https://www.shadertoy.com/view/ld23DV
// and https://iquilezles.org/www/articles/boxfunctions/boxfunctions.htm
class OBB : public Geometry{
public:
	glm::vec3 center, size, rotation;
	glm::mat4 model;

	//this->size = half-size
	OBB(glm::vec3 center, glm::vec3 size, glm::vec3 rotation, Material *material) {
		this->center = center;
		this->size = size / 2.0f;
		this->rotation = rotation;
		this->material = material;
		model = glm::translate(glm::mat4(1.0f), center);
		model = model * rotate(rotation);
	};

	//Angle in degrees
	//v axis to be rotated
	glm::mat4 rotateAxis(glm::vec3 v, float angle) {
		float angleRad = glm::radians(angle);

		float s = sin(angleRad);
		float c = cos(angleRad);
		float ic = 1.0 - c;

		return glm::mat4(v.x*v.x*ic + c, v.y*v.x*ic - s * v.z, v.z*v.x*ic + s * v.y, 0.0,
			v.x*v.y*ic + s * v.z, v.y*v.y*ic + c, v.z*v.y*ic - s * v.x, 0.0,
			v.x*v.z*ic - s * v.y, v.y*v.z*ic + s * v.x, v.z*v.z*ic + c, 0.0,
			0.0, 0.0, 0.0, 1.0);
	}

	glm::mat4 rotate(glm::vec3 angles) {
		return rotateAxis(glm::vec3(1, 0, 0), angles.x) * rotateAxis(glm::vec3(0, 1, 0), angles.y) * rotateAxis(glm::vec3(0, 0, 1), angles.z);
	}

	glm::vec3 collision(Ray r, float &tNear, float &tFar){
		tNear = -1;
		tFar = -1;
		// convert from ray to box space
		glm::vec3 rdd = glm::vec3( glm::inverse(model) * glm::vec4(r.direction, 0.0));
		glm::vec3 roo = glm::vec3( glm::inverse(model) * glm::vec4(r.origin, 1.0));

		// ray-box intersection in box space
		glm::vec3 m = 1.0f / rdd; //TODO: take Care of div by 0
		glm::vec3 n = m * roo;
		glm::vec3 k = abs(m) * size;

		glm::vec3 t1 = -n - k;
		glm::vec3 t2 = -n + k;

		float tN = glm::max(glm::max(t1.x, t1.y), t1.z);
		float tF = glm::min(glm::min(t2.x, t2.y), t2.z);

		if (tN > tF || tF < 0.0) return glm::vec3(-1.0);

		glm::vec3 nor = -glm::sign(rdd) * glm::step(glm::vec3(t1.y, t1.z, t1.x), t1) * glm::step(glm::vec3(t1.z, t1.x, t1.y), t1);

		// convert to ray space
		nor = glm::vec3(model * glm::vec4(nor, 0.0));
		tNear = glm::max(tN, 0.0f); //Not most elegant solution
		tFar = tF;
		

		return glm::normalize(nor);
	}


	bool hit(Ray &r, float tMin, float tMax, Hit &hit) {
		float tNear; 
		float tFar;
		glm::vec3 v = collision(r, tNear, tFar);
		//if tMax < 0, whole AABB is behind ray origin
		//if tMin < 0, ray origin is inside AABB
		if (/*tNear >= 0 &&*/ tNear > tMin && tNear < tMax && tMax > 0) {
			hit.t = tNear;
			hit.tFar = tFar;
			hit.p = r.o + tNear * r.d;
			hit.normal = v;
			hit.material = this->material;
			hit.collider = this;
			return true;
		}else
			return false;
	}

	virtual OBB *clone() const {
		return new OBB(*this);
	}

	OBB();
	~OBB();
};

