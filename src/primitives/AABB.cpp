#include "primitives/AABB.h"

namespace narvalengine {

	AABB::AABB(float *v1, float *v2) {
		vertexData[0] = v1;
		vertexData[1] = v2;
	}

	AABB::AABB(glm::vec3 v1, glm::vec3 v2) {
		vertexData[0] = new float[3];
		vertexData[1] = new float[3];

		*(vertexData[0]) = v1.x;
		*(vertexData[0] + 1) = v1.y;
		*(vertexData[0] + 2) = v1.z;

		*(vertexData[1]) = v2.x;
		*(vertexData[1] + 1) = v2.y;
		*(vertexData[1] + 2) = v2.z;
	}

	glm::vec3 AABB::getSize() {
		glm::vec3 v0 = getVertex(0);
		glm::vec3 v1 = getVertex(1);

		for (int i = 0; i < 3; i++)
			if (v0[i] == v1[i])
				v0[i] = v1[i] = 0;

		return glm::abs(v0) + glm::abs(v1);
	}

	glm::vec3 AABB::getCenter() {
		glm::vec3 size = glm::abs(getVertex(0)) + glm::abs(getVertex(1));

		//Assumes that v[1] is the AABB's max
		return getVertex(1) - size / 2.0f;
	}

	glm::vec3 AABB::getVertex(int n) {
		return glm::vec3(*vertexData[n], *(vertexData[n] + 1), *(vertexData[n] + 2));
	}

	/*
		Checks if Ray r intesercts this primitive. If true, stores its values on hit.
		Ray must be in OCS. Assumes the box is centered at origin.
		Source: https://iquilezles.org/www/articles/boxfunctions/boxfunctions.htm
	*/
	bool AABB::intersect(Ray r, RayIntersection &hit) {
		/*glm::vec3 m = 1.0f / r.d;
		glm::vec3 n = m * r.o;
		glm::vec3 k = abs(m) * 0.5f;
		glm::vec3 t1 = -n - k;
		glm::vec3 t2 = -n + k;

		float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
		float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);

		if (tNear > tFar) return false;

		hit.normal = -glm::sign(r.d) * glm::step(glm::vec3(t1.y, t1.z, t1.x), t1) * glm::step(glm::vec3(t1.z, t1.x, t1.y), t1);
		hit.tNear = tNear;
		hit.tFar = tFar;
		hit.hitPoint = r.getPointAt(hit.tNear);
		hit.uv = glm::vec2(0, 0);
		hit.primitive = this;*/

		glm::vec3 s = glm::vec3((r.d.x < 0.0) ? 1.0 : -1.0,
			(r.d.y < 0.0) ? 1.0 : -1.0,
			(r.d.z < 0.0) ? 1.0 : -1.0);

		glm::vec3 invD = 1.0f / r.d;

		//TODO: Not the most elegant solution for arbitrary origin
		glm::vec3 t1 = (getCenter() - r.o + s * getSize() / 2.0f) * invD;
		glm::vec3 t2 = (getCenter() - r.o - s * getSize() / 2.0f) * invD;

		glm::vec3 tmin = glm::min(t1, t2);
		glm::vec3 tmax = glm::max(t1, t2);

		float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
		float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

		if (tNear > tFar) return false;

		hit.normal = -glm::sign(r.d) * glm::step(glm::vec3(t1.y, t1.z, t1.x), t1) * glm::step(glm::vec3(t1.z, t1.x, t1.y), t1);
		hit.tNear = tNear;
		hit.tFar = tFar;
		hit.hitPoint = r.getPointAt(hit.tNear);
		hit.uv = glm::vec2(0, 0);
		hit.primitive = this;

		return true;
	}

	//TODO: implement
	glm::vec3 AABB::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		return glm::vec3(0);
	}

	//TODO: implement
	glm::vec2 AABB::samplePointOnTexture(glm::vec3 pointOnSurface) {
		return glm::vec2(0, 0);
	}

	float AABB::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		return 1;
	}
}