#pragma once
#include "primitives/Primitive.h"

namespace narvalengine {
	class Point : public Primitive {
	public:
		float* vertexData[1];

		Point();
		~Point();
		bool intersect(Ray r, RayIntersection& hit);
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
	};

}
