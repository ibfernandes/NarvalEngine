#pragma once
#include "primitives/Primitive.h"

namespace narvalengine {
	class Point : public Primitive {
	public:
		float* vertexData[1];

		Point();
		~Point();
		/*
			Checks if Ray r intesercts this primitive. If true, stores its values on hit.
			Ray must be in OCS.
		*/
		bool intersect(Ray r, RayIntersection& hit);

		/*
			Samples point on this sphere relative to the interaction point. Relates to uniform cone sampling.
		*/
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);

		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);

		/*
			interaction coordinates systems must be in this primitive OCS
			shirley1996 - Monte Carlo Techniques for Direct Lighting Calculations
		*/
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
	};

}
