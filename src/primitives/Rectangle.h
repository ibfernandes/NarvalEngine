#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"

namespace narvalengine {
	class Rectangle : public Primitive {
	public:
		float *vertexData[2]; //TODO why is it not contiguous? Should be for performance reasons.
		glm::vec3 normal = glm::vec3(0, 0, -1);

		glm::vec3 getVertex(int n);

		glm::vec3 getSize();
		glm::vec3 getCenter();

		Rectangle();

		//TODO: check same hemisphere (rectangle one sided)
		//TODO: I could strictly define the plane on a xy axis since the ray must be in OCS
		bool containsPoint(glm::vec3 point);

		/*
			Checks if Ray r intesercts this primitive. If true, stores its values on hit.
			Ray must be in OCS.
		*/
		bool intersect(Ray r, RayIntersection &hit);

		//TODO abs of width of both vertices
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		//Given a hit point on surface, convert it to texture coordinates
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec4 barycentricCoordinates(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
	};
}