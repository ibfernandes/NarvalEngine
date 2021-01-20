#pragma once
#include "primitives/BucketLBVH.h"
#include "materials/Texture.h"
#include "utils/Math.h"
#include "materials/Medium.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include <vector>
#include <math.h>

namespace narvalengine {
	class GridMedia : public Medium {
	public:
		glm::vec3 absorption;
		glm::vec3 scattering;
		glm::vec3 extinction;
		BucketLBVH *lbvh;
		float densityMultiplier =  1.0f;

		GridMedia(glm::vec3 scattering, glm::vec3 absorption, std::string material, float density) {
			this->scattering = scattering;
			this->absorption = absorption;
			this->extinction = absorption + scattering;
			this->densityMultiplier = density;

			Texture* tex = ResourceManager::getSelf()->getTexture(material);

			lbvh = new BucketLBVH((float*)tex->mem.data, tex->getResolution());
		}

		glm::vec3 Tr(float d) {
			glm::vec3 v = extinction * d * densityMultiplier;
			return exp(-v);
		}

		//should also account for in scattering?
		glm::vec3 Tr(Ray incoming, RayIntersection ri) {
			glm::vec3 hit = lbvh->traverseTreeUntil(incoming, 99999);

			//if hit volume calculate Tr
			if (hit.x <= hit.y) {
				float sampledDensity = hit.z;
				float distInsideVol = hit.y - hit.x;
				return Tr(distInsideVol * hit.z);
			} else
				return Tr(0);
		}

		/*
			intersection must be inside the volume
		*/
		glm::vec3 sample(Ray incoming, Ray &scattered, RayIntersection intersection) {
			float t = -std::log(1 - random()) / avg(extinction);
			glm::vec3 hit = lbvh->traverseTreeUntil(incoming, 99999);
			float distVoxels = hit.y - hit.x;
			float distAABB = intersection.tFar - intersection.tNear;
			t = t / distVoxels;

			//if missed all voxels
			if (hit.x > hit.y) {
				scattered.o = incoming.getPointAt(distAABB + 0.001f);
				scattered.d = incoming.d;
				return glm::vec3(1.5,0,0);
				//return glm::vec3(-1);
			}

			bool sampledMedia = t < distVoxels;

			//point is inside volume
			if (sampledMedia) {
				incoming.o = incoming.getPointAt(hit.x);
				scattered.o = incoming.getPointAt(t);
				scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, intersection.normal);
			} else {
				scattered.o = incoming.getPointAt(distAABB + 0.001f);
				scattered.d = incoming.d;
				//return glm::vec3(-1);
			}

			glm::vec3 Tr = this->Tr(t * hit.z);

			glm::vec3 density = sampledMedia ? (extinction * Tr) : Tr;

			float pdf = avg(density);

			if (pdf == 0) pdf = 1;

			return sampledMedia ? (Tr * scattering / pdf) : Tr / pdf;
		}
	};
}