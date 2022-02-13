#include "primitives/Triangle.h"

namespace narvalengine {

	Triangle::Triangle() {
	}

	Triangle::~Triangle() {
	}

	void Triangle::calculateAABB(glm::vec3& min, glm::vec3& max) {
		min = glm::min(getVertex(0), glm::min(getVertex(1), getVertex(2)));
		max = glm::max(getVertex(0), glm::max(getVertex(1), getVertex(2)));
	}

	glm::vec3 Triangle::getCentroid() {
		return (getVertex(0) + getVertex(1) + getVertex(2)) / 3.0f;
	}

	Triangle::Triangle(float* index1, float* index2, float* index3, Material* material, float* normal) {
		this->material = material;
		vertexData[0] = index1;
		vertexData[1] = index2;
		vertexData[2] = index3;
		this->normal = normal;

		//Pre-calculate the area of this triangle.
		area = calculateArea();
	}

	Triangle::Triangle(float* index1, float* index2, float* index3, Material* material) : Triangle(index1, index2, index3, material, nullptr) {
	}

	Triangle::Triangle(float* index1, float* index2, float* index3) : Triangle(index1, index2, index3, nullptr, nullptr) {
	}

	glm::vec3 Triangle::getVertex(int n) {
		return glm::vec3(*vertexData[n], *(vertexData[n] + 1), *(vertexData[n] + 2));
	}

	glm::vec2 *Triangle::getUV(int n) {
		int stride = vertexLayout->stride / sizeof(float);
		int texOffset = vertexLayout->offset[VertexAttrib::TexCoord0] / sizeof(float);
		float* texPointer = vertexData[0] + texOffset;

		return (glm::vec2*)(texPointer + stride * n);
	}

	bool Triangle::intersect(Ray ray, RayIntersection &hit) {
		glm::vec3 v1v0 = getVertex(1) - getVertex(0);
		glm::vec3 v2v0 = getVertex(2) - getVertex(0);
		glm::vec3 rov0 = ray.o - getVertex(0);

		glm::vec3 normal = glm::cross(v1v0, v2v0);
		glm::vec3 q = glm::cross(rov0, ray.d);

		float denom = 1.0 / glm::dot(ray.d, normal);

		float u = glm::dot(-q, v2v0) * denom;
		float v = glm::dot(q, v1v0) * denom;
		float t = glm::dot(-normal, rov0) * denom;

		if (std::isnan(t) || t < 0)
			return false;

		if (u < 0.0f || u > 1.0f || v < 0.0f || (u + v) > 1.0f)
			return false;


		hit.tNear = t;
		hit.tFar = t;
		hit.hitPoint = ray.getPointAt(hit.tNear);
		hit.uv = samplePointOnTexture(hit.hitPoint);
		if (this->hasMaterial() && this->material->hasTexture(TextureName::NORMAL))
			hit.normal = convertNormalFromTextureMap(this->material->sampleMaterial(TextureName::NORMAL, hit.uv.x, hit.uv.y), glm::normalize(normal));
		else
			hit.normal = glm::normalize(normal);
		hit.primitive = this;

		return true;
	}

	bool Triangle::contains(glm::vec3 point) {

		glm::vec3 n = glm::cross(getVertex(1) - getVertex(0), getVertex(2) - getVertex(0));
		float lenSq = glm::length(n);
		float d = glm::dot(n, getVertex(1) - point);
		if (d * d > EPSILON3 * lenSq)
			return false;

		glm::vec3 uvw = barycentricCoordinates(point, getVertex(0), getVertex(1), getVertex(2));
		if (uvw.x >= 0 && uvw.y >= 0 && uvw.x + uvw.y <= 1)
			return true;
		return false;
	}

	glm::vec3 Triangle::barycentricCoordinates(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
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
		glm::vec2 e = {random(), random()};
		e = {1.0f - std::sqrt(e.x), e.y * std::sqrt(e.x)};

		return  e.x * getVertex(0) + e.y * getVertex(1) + (1 - e.x - e.y) * getVertex(2);
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

	float Triangle::calculateArea() {
		glm::vec3 v1 = getVertex(0);
		glm::vec3 v2 = getVertex(1);
		glm::vec3 v3 = getVertex(2);
		glm::vec3 v1v2 = v2 - v1;
		glm::vec3 v1v3 = v3 - v1;
		glm::vec3 n = glm::cross(v1v2, v1v3);
		return glm::length(n) / 2.0f;
	}

	float Triangle::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		return convertAreaToSolidAngle(1.0f / area, interaction.normal, interaction.hitPoint, samplePointOnSurface(interaction, transformToWCS));
	}
}