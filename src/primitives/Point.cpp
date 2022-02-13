#include "primitives/Point.h"

namespace narvalengine {
	Point::Point() {
	}

	Point::~Point() {
	}

	bool Point::intersect(Ray r, RayIntersection& hit) {
		return false;
	}

	glm::vec3 Point::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 point = glm::vec3(*vertexData[0], *(vertexData[0] + 1), *(vertexData[0] + 2));
		point = transformToWCS * glm::vec4(point, 1.0f);
		return point;
	}

	glm::vec2 Point::samplePointOnTexture(glm::vec3 pointOnSurface) {
		return glm::vec2(0);
	}

	float Point::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		return 1;
	}

	void Point::calculateAABB(glm::vec3& min, glm::vec3& max) {
		min = *((glm::vec3*)vertexData[0]);
		max = *((glm::vec3*)vertexData[0]);
	}
}