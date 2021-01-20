#include "Sphere.h"

namespace narvalengine {

	Sphere::Sphere() {
	}


	Sphere::~Sphere() {
	}

	bool Sphere::intersect(Ray r, RayIntersection &hit) {
		glm::vec3 center = glm::vec3(*vertexData[0], *(vertexData[0] + 1), *(vertexData[0] + 2));
		glm::vec3 oc = r.origin - center;
		float a = glm::dot(r.direction, r.direction);
		float b = glm::dot(oc, r.direction);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - a * c;

		if (discriminant >= 0) {
			float temp1 = (-b - sqrt(discriminant)) / a;
			float temp2 = (-b + sqrt(discriminant)) / a;

			hit.tNear = glm::min(temp1, temp2);
			hit.tFar = glm::max(temp1, temp2);
			hit.hitPoint = r.getPointAt(hit.tNear);
			hit.normal = glm::normalize((hit.hitPoint - center) / radius);
			hit.uv = glm::vec2(0, 0);//TODO
			hit.primitive = this;

			return true;
		}

		return false;
	}

	glm::vec3 Sphere::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 sphereCenter = glm::vec3(*vertexData[0], *(vertexData[0] + 1), *(vertexData[0] + 2));
		sphereCenter = transformToWCS * glm::vec4(sphereCenter, 1.0f);

		glm::vec3 w = sphereCenter - interaction.hitPoint;
		float distanceToCenter = glm::length(w);
		w = glm::normalize(w);

		float q = glm::max(0.0, std::sqrt(1.0 - (radius / distanceToCenter) * (radius / distanceToCenter)));
		glm::vec3 v, u;
		generateOrthonormalCS(w, v, u);

		float r1 = random();
		float r2 = random();
		float theta = std::acos(1 - r1 + r1 * q);
		float phi = TWO_PI * r2;

		glm::vec3 local = sphericalToCartesian(radius, theta, phi);
		//direction to x'
		glm::vec3 nwp = glm::normalize(toWorld(glm::normalize(local), w, v, u));

		return toWorld(local, w, v, u); //TODO is this that correct?
	}

	glm::vec2 Sphere::samplePointOnTexture(glm::vec3 pointOnSurface) {
		return glm::vec2(0);
	}

	float Sphere::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 sphereCenter = glm::vec3(*vertexData[0], *(vertexData[0] + 1), *(vertexData[0] + 2));
		sphereCenter = transformToWCS * glm::vec4(sphereCenter, 1.0f);
		glm::vec3 w = sphereCenter - interaction.hitPoint;
		float distanceToCenter = glm::length(w);
		float q = glm::max(0.0, std::sqrt(1.0 - (radius / distanceToCenter) * (radius / distanceToCenter)));

		//Uniform cone PDF
		return 1.0f / (2.0f * PI * (1.0f - q));
	}
}