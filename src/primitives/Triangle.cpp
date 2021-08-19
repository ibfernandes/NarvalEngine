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
		hit.uv = samplePointOnTexture(hit.hitPoint);
		hit.normal = glm::normalize(normal);
		hit.primitive = this;

		return true;
	}

	//TODO not tested yet
	glm::vec3 Triangle::barycentricCoordinates(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
		glm::vec3 uvw;

		glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
		float d00 = glm::dot(v0, v0);
		float d01 = glm::dot(v0, v1);
		float d11 = glm::dot(v1, v1);
		float d20 = glm::dot(v2, v0);
		float d21 = glm::dot(v2, v1);
		float denom = d00 * d11 - d01 * d01;
		uvw[1] = (d11 * d20 - d01 * d21) / denom;
		uvw[2] = (d00 * d21 - d01 * d20) / denom;
		uvw[0] = 1.0f - uvw[1] - uvw[2];

		return uvw;
	}

	glm::vec3 Triangle::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		return glm::vec3(0);
	}

	glm::vec2 Triangle::samplePointOnTexture(glm::vec3 pointOnSurface) {
		if (vertexLayout == nullptr || !vertexLayout->contains(VertexAttrib::TexCoord0))
			return glm::vec3(0);
		else {
			glm::vec2 uv[3];
			int stride = vertexLayout->stride / sizeof(float);
			int texOffset = vertexLayout->offset[VertexAttrib::TexCoord0] / sizeof(float);
			int vertexOffset = vertexLayout->offset[VertexAttrib::Position] / sizeof(float);
			float* texPointer = vertexData[0] + texOffset;

			uv[0] = *((glm::vec2*)texPointer);
			uv[1] = *((glm::vec2*)(texPointer + stride * 1));
			uv[2] = *((glm::vec2*)(texPointer + stride * 2));

			glm::vec3 vertices[3];
			vertices[0] = *((glm::vec3*)vertexData[0]);
			vertices[1] = *((glm::vec3*)(vertexData[0] + stride * 1));
			vertices[2] = *((glm::vec3*)(vertexData[0] + stride * 2));

			glm::vec3 baryCoord = barycentricCoordinates(pointOnSurface, vertices[0], vertices[1], vertices[2]);
			glm::vec3 lambda = glm::vec3(baryCoord);

			glm::vec2 res;
			res = glm::vec2(lambda.x * uv[0] + lambda.y * uv[1] + lambda.z * uv[2]);

			return res;
		}
	}

	float Triangle::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		return 1;
	}
}