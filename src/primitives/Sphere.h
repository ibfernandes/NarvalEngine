#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"

namespace narvalengine {
	class Sphere : public Primitive {
	public:
		float *vertexData[1];
		float radius;

		Sphere();
		~Sphere();

		glm::vec3 getCenter();
		bool intersect(Ray r, RayIntersection &hit);
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
	};
}