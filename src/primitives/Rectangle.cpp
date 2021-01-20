#include "Rectangle.h"

namespace narvalengine {
	Rectangle::Rectangle() {
	}

	glm::vec3 Rectangle::getVertex(int n) {
		return glm::vec3(*vertexData[n], *(vertexData[n] + 1), *(vertexData[n] + 2));
	}

	glm::vec3 Rectangle::getSize() {
		glm::vec3 v0 = getVertex(0);
		glm::vec3 v1 = getVertex(1);

		for (int i = 0; i < 3; i++)
			if (v0[i] == v1[i])
				v0[i] = v1[i] = 0;

		return glm::abs(v0) + glm::abs(v1);
	}

	glm::vec3 Rectangle::getCenter() {

		//Assumes that v[1] is the Rectangle's max
		return getVertex(1) - getSize() / 2.0f;
	}

	//TODO: check same hemisphere (rectangle one sided)
	//TODO: I could strictly define the plane on a xy axis since the ray must be in OCS
	bool Rectangle::containsPoint(glm::vec3 point) {
		glm::vec3 planeVertex1 = glm::vec3(*vertexData[0], *(vertexData[0] + 1), *(vertexData[0] + 2));
		glm::vec3 planeVertex2 = glm::vec3(*vertexData[1], *(vertexData[1] + 1), *(vertexData[1] + 2));

		glm::vec3 min = glm::min(planeVertex1, planeVertex2);
		glm::vec3 max = glm::max(planeVertex1, planeVertex2);
		glm::vec3 size = getSize();

		if (size.x > 0 && (point.x < min.x || point.x > max.x))
			return false;
		if (size.y > 0 && (point.y < min.y || point.y > max.y))
			return false;
		if (size.z > 0 && (point.z < min.z || point.z >  max.z))
			return false;

		return true;
	}

	/*
		Checks if Ray r intesercts this primitive. If true, stores its values on hit.
		Ray must be in OCS.
	*/
	bool Rectangle::intersect(Ray r, RayIntersection &hit) {
		//glm::vec3 planeVertex = getCenter();
		glm::vec3 planeVertex = getVertex(1);

		float denom = glm::dot(normal, r.direction);
		if (glm::abs(denom) < EPSILON)
			return false;

		float num = glm::dot(normal, planeVertex - r.origin);
		float t = num / denom;

		if (t < 0)
			return false;

		glm::vec3 pointOnPlaneAlongRay = r.getPointAt(t);
		if (containsPoint(pointOnPlaneAlongRay)) {

			hit.tNear = t;
			hit.tFar = t;
			hit.normal = normal;
			hit.hitPoint = pointOnPlaneAlongRay;
			hit.primitive = this;
			hit.uv = glm::vec2(0, 0);//TODO: barycentric coordinates
			return true;
		}

		return false;
	}

	//TODO abs of width of both vertices
	glm::vec3 Rectangle::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 e(random(), random(), random());
		glm::vec3 v0 = getVertex(0);
		glm::vec3 size = getSize();
		glm::vec3 pointOnSurface = glm::vec3(v0.x + size.x * e[0], v0.y + size.y * e[1], v0.z + size.z * e[2]);

		//TODO barycentric or something more elegant than this 
		return pointOnSurface;
	}

	//Given a hit point on surface, convert it to texture coordinates
	glm::vec2 Rectangle::samplePointOnTexture(glm::vec3 pointOnSurface) {
		return glm::vec3(0);
	}

	float Rectangle::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 sizeWCS = glm::vec3(transformToWCS[0][0], transformToWCS[1][1], transformToWCS[2][2]) * getSize();
		//TODO confirm this math here

		float area = 1;

		for (int i = 0; i < 3; i++)
			if (sizeWCS[i] != 0)
				area = area * sizeWCS[i];

		// 1 / area
		return 1.0f / area;
	}
}