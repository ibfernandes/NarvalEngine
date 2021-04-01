#include "materials/HomogeneousMedia.h"

namespace narvalengine {

	HomogeneousMedia::HomogeneousMedia(glm::vec3 scattering, glm::vec3 absorption, float density) {
		this->scattering = scattering;
		this->absorption = absorption;
		this->extinction = absorption + scattering;
		this->density = density;
	}

	HomogeneousMedia::~HomogeneousMedia() {
	}

	glm::vec3 HomogeneousMedia::Tr(float distance) {
		glm::vec3 v = extinction * distance * density;
		return exp(-v);
	}

	glm::vec3 HomogeneousMedia::Tr(Ray incoming, RayIntersection ri) {
		return Tr(ri.tFar - ri.tNear);
	}

	/*
		intersection must be inside or at the boundary of the volume
	*/
	glm::vec3 HomogeneousMedia::sample(Ray incoming, Ray &scattered, RayIntersection intersection) {
		float t = -std::log(1 - random()) / avg(extinction);
		float distInsideVolume = intersection.tFar - intersection.tNear;
		t = t / distInsideVolume;

		bool sampledMedia = t < distInsideVolume;

		//point is inside volume
		if (sampledMedia) {
			//move ray to near intersection at voxel
			scattered.o = incoming.getPointAt(t);
			scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, intersection.normal);
		} else {
			scattered.o = incoming.getPointAt(distInsideVolume + 0.001f);
			scattered.d = incoming.d;

			//return glm::vec3(0, 1, 0);
		}

		glm::vec3 Tr = this->Tr(t);

		glm::vec3 density = sampledMedia ? (extinction * Tr) : Tr;

		float pdf = avg(density);

		if (pdf == 0) pdf = 1;

		glm::vec3 result = sampledMedia ? (Tr * scattering / pdf) : Tr / pdf;
		return result;
	}
}