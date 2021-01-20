#include "Triangle.h"

namespace narvalengine {

	Triangle::Triangle() {
	}

	Triangle::~Triangle() {
	}

	glm::vec3 Triangle::getVertex(int n) {
		return glm::vec3(*vertexData[n], *(vertexData[n] + 1), *(vertexData[n] + 2));
	}

	/*
		Checks if Ray r intesercts this primitive. If true, stores its values on hit.
		Ray must be in OCS.
		Source: https://www.iquilezles.org/www/articles/intersectors/intersectors.htm
	*/
	bool Triangle::intersect(Ray r, RayIntersection &hit) {
		glm::vec3 v1v0 = getVertex(1) - getVertex(0);
		glm::vec3 v2v0 = getVertex(2) - getVertex(0);
		glm::vec3 rov0 = r.o - getVertex(0);

		glm::vec3 normal = glm::cross(v1v0, v2v0);
		glm::vec3 q = glm::cross(rov0, r.d);

		float denom = 1.0 / glm::dot(r.d, normal);

		float u = glm::dot(-q, v2v0) * denom;
		float v = glm::dot(q, v1v0) * denom;
		float t = glm::dot(-normal, rov0) * denom;

		if (u < 0.0f || u > 1.0f || v < 0.0f || (u + v) > 1.0f)
			return false;

		hit.tNear = t;
		hit.tFar = t;
		hit.hitPoint = r.getPointAt(hit.tNear);
		hit.uv = glm::vec2(0, 0);//TODO: barycentric coordinates
		hit.normal = glm::normalize(normal);
		hit.primitive = this;

		return true;
	}

	glm::vec3 Triangle::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		return glm::vec3(0);
	}

	glm::vec2 Triangle::samplePointOnTexture(glm::vec3 pointOnSurface) {
		if (true /*packet contains texCoord */) {
			glm::vec2 tex[3];
			//do barycentric stuff and get final point on texture in normalized space
		}
		return glm::vec2(0, 0);
	}

	float Triangle::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		return 1;
	}
}