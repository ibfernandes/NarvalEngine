#include "materials/GridMedia.h"

namespace narvalengine {
	GridMedia::GridMedia(glm::vec3 scattering, glm::vec3 absorption, Texture* tex, float densityMultiplier) {
		this->scattering = scattering;
		this->absorption = absorption;
		this->extinction = this->absorption + this->scattering;
		this->densityMultiplier = densityMultiplier;
		this->resolution = tex->getResolution();
		grid = tex;

		invMaxDensity = 1.0f / calculateMaxDensity();
	}

	float GridMedia::density(glm::vec3 gridPoint) {
		//If out of bounds, the density is simply 0.
		if (gridPoint.x >= grid->width || gridPoint.y >= grid->height || gridPoint.z >= grid->depth)
			return 0;
		if (gridPoint.x < 0 || gridPoint.y < 0 || gridPoint.z < 0)
			LOG(FATAL) << "Grid point was negative. Value was: " << toString(gridPoint) << ".";

		return grid->sampleAtIndex(to1D(grid->width, grid->height, gridPoint.x, gridPoint.y, gridPoint.z)).x;
	}

	float GridMedia::interpolatedDensity(glm::vec3 gridPoint) {
		if (gridPoint.x < 0 && gridPoint.x + EPSILON3 > 0)
			gridPoint.x = 0;
		if (gridPoint.y < 0 && gridPoint.y + EPSILON3 > 0)
			gridPoint.y = 0;
		if (gridPoint.z < 0 && gridPoint.z + EPSILON3 > 0)
			gridPoint.z = 0;

		glm::ivec3 gridPointInt = glm::ivec3(glm::floor(gridPoint.x), glm::floor(gridPoint.y), glm::floor(gridPoint.z));
		glm::vec3 d = gridPoint - glm::vec3(gridPointInt);

		// Trilinearly interpolate density values to compute local density.
		float d00 = glm::lerp(density(gridPointInt), density(gridPointInt + glm::ivec3(1, 0, 0)), d.x);
		float d10 = glm::lerp(density(gridPointInt + glm::ivec3(0, 1, 0)), density(gridPointInt + glm::ivec3(1, 1, 0)), d.x);
		float d01 = glm::lerp(density(gridPointInt + glm::ivec3(0, 0, 1)), density(gridPointInt + glm::ivec3(1, 0, 1)), d.x);
		float d11 = glm::lerp(density(gridPointInt + glm::ivec3(0, 1, 1)), density(gridPointInt + glm::ivec3(1, 1, 1)), d.x);
		float d0 = glm::lerp(d00, d10, d.y);
		float d1 = glm::lerp(d01, d11, d.y);
		return glm::lerp(d0, d1, d.z);
	}

	float GridMedia::sampleAt(Ray &ray, float depth) {
		glm::vec3 gridPoint = ray.getPointAt(depth);
		return interpolatedDensity(fromOCStoGCS(gridPoint.x, gridPoint.y, gridPoint.z));
	}

	glm::vec3 GridMedia::Tr(Ray incoming, RayIntersection intersection) {
		//Convert the incoming ray to Object Coordinate System (OCS).
		incoming = transformRay(incoming, intersection.instancedModel->invTransformToWCS);

		incoming.o = incoming.getPointAt(intersection.tNear);
		intersection.tFar = intersection.tFar - intersection.tNear;
		intersection.tNear = 0;

		// Perform ratio tracking to estimate the transmittance value.
		float Tr = 1, t = intersection.tNear;
		while (true) {
			t -= std::log(1 - random()) * invMaxDensity / avg(extinction * densityMultiplier);
			if (t >= intersection.tFar) break;
			float density = sampleAt(incoming, t);
			Tr *= 1 - std::max(0.0f, density * invMaxDensity);
			const float rrThreshold = .1;
			if (Tr < rrThreshold) {
				float q = std::max(0.05f, 1.0f - Tr);
				if (random() < q) return glm::vec3(0.0f);
				Tr /= 1 - q;
			}
		}

		return glm::vec3(Tr);
	}

	glm::vec3 GridMedia::sample(Ray incoming, Ray& scattered, RayIntersection intersection) {
		scattered.o = incoming.o;
		scattered.d = incoming.d;

		//Convert the incoming ray to Object Coordinate System (OCS).
		incoming = transformRay(incoming, intersection.instancedModel->invTransformToWCS);

		// Run delta-tracking iterations to sample a medium interaction.
		float t = intersection.tNear;
		while (true) {
			float r = random();
			float sampledDist = std::log(1 - r) * invMaxDensity / avg(extinction * densityMultiplier);
			t -= sampledDist;
			if (t >= intersection.tFar) {
				scattered.o = incoming.getPointAt(intersection.tFar + 0.0001f);
				break;
			}

			float density = sampleAt(incoming, t);
			float ra = random();
			if (density * invMaxDensity > ra) {
				scattered.o = incoming.getPointAt(t);
				scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, glm::vec3(0, 0, 1));
				scattered = transformRay(scattered, intersection.instancedModel->transformToWCS);
				return scattering / extinction;
			}
		}

		return glm::vec3(1.0f);
	}
};
