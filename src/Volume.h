#pragma once
#include "Model.h"
#include "Ray.h"
#include "Material.h"
#include "GridMaterial.h"
#include "BucketLBVH.h"
#include "Math.h"
#include <bitset>
#include <limits>
#include <glm/glm.hpp>
//#define DEBUG_VOLUME false

class Volume : public Model {
public:
	float radius;
	GridMaterial *material;
	BucketLBVH *lbvh;

	Volume(GridMaterial *material) {
		this->lbvh = material->lbvh;
		this->material = material;
	};

	bool regularTracking(Ray &r, float tMin, float tMax, Hit &hit) {
		glm::vec2 thit = lbvh->intersectOuterAABB(r);

		if (thit.x > thit.y)
			return false;


		if (thit.x > tMin && thit.x < tMax) {
			float t = -std::log(1 - random()) ;

			if (t > thit.y)
				return false;

			hit.t = thit.x + t;
			hit.p = r.getPointAt(hit.t);
			hit.normal = -r.direction;
			hit.material = material;
			return true;
		}

		return false;
	}

	/*bool workingOne(Ray &r, float tMin, float tMax, Hit &hit) {
		glm::vec2 t = lbvh->intersectOuterAABB(r);


		if (t.x > t.y)
			return false;


		if (t.x > tMin && t.x < tMax) {
			hit.t = t.x + random();
			hit.p = r.getPointAt(hit.t);
			hit.normal = -r.direction;
			hit.material = material;
			return true;
		}
		return false;
	}
	*/
	inline bool hit(Ray &r, float tMin, float tMax, Hit &hit) {
		return regularTracking(r, tMin, tMax, hit);
	} 
};