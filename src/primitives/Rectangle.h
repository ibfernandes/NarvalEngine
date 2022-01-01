#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"

namespace narvalengine {
	class Rectangle : public Primitive {
	public:
		float *vertexData[2];
		glm::vec3 normal = glm::vec3(0, 0, -1);

		glm::vec3 getVertex(int n);

		glm::vec3 getSize();
		glm::vec3 getCenter();

		Rectangle();
		bool containsPoint(glm::vec3 point);
		bool intersect(Ray r, RayIntersection &hit);
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec4 barycentricCoordinates(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
	};
}