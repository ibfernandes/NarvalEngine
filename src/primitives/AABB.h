#pragma once
#include "primitives/Primitive.h"
#include "primitives/Ray.h"

namespace narvalengine {
	class AABB : public Primitive {
	public:
		float *vertexData[2];

		AABB() {

		}
		AABB(float *v1, float *v2);
		AABB(glm::vec3 v1, glm::vec3 v2);
		glm::vec3 getSize();
		glm::vec3 getCenter();
		glm::vec3 getVertex(int n);
		/*
			Checks if Ray r intesercts this primitive. If true, stores its values on hit.
			Ray must be in OCS.
		*/
		bool intersect(Ray r, RayIntersection &hit);
		//TODO: implement
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		//TODO: implement
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
	};
}