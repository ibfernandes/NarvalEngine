#pragma once
#include "primitives/BucketLBVH.h"
#include "materials/Texture.h"
#include "utils/Math.h"
#include "materials/Medium.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "core/BSDF.h"
#include <vector>
#include <math.h>

namespace narvalengine {
	class GridMedia : public Medium {
	public:
		glm::vec3 absorption;
		glm::vec3 scattering;
		glm::vec3 extinction;
		BucketLBVH *lbvh;
		float density =  1.0f;
		float invMaxDensity = 1;
		float t = 0;
		glm::vec3 tr = glm::vec3(1);

		GridMedia(glm::vec3 scattering, glm::vec3 absorption, std::string material, float density) {
			this->scattering = scattering;
			this->absorption = absorption;
			this->extinction = this->absorption + this->scattering;
			this->density = density;

			Texture* tex = ResourceManager::getSelf()->getTexture(material);

			lbvh = new BucketLBVH((float*)tex->mem.data, tex->getResolution());
			invMaxDensity = 1.0f / (lbvh->maxDensity /* * density*/);
		}

		glm::vec3 Tr(float d) {
			//glm::vec3 v = extinction * d * densityMultiplier;
			glm::vec3 v = extinction * d ;
			return exp(-v);
		}

		/*
			Incoming Ray must be in OCS.
		*/
		glm::vec3 Tr(Ray incoming, RayIntersection ri) {
			//glm::vec3 hit = lbvh->traverseTreeUntil(incoming, 99999);
			//if (hit.x > hit.y) {
			//	return glm::vec3(1.0f);
			//}

			//incoming.o = incoming.getPointAt(hit.x);
			//ri.tFar = hit.y - hit.x;
			//ri.tNear = 0;

			incoming.o = incoming.getPointAt(ri.tNear);
			ri.tFar = ri.tFar - ri.tNear;
			ri.tNear = 0;

			// Perform ratio tracking to estimate the transmittance value
			//pbrt requires a spectrally uniform attenuation coeff , thus why extinction.x
			float Tr = 1, t = ri.tNear;
			//std::cout << "before ratio " << Tr << std::endl;
			while (true) {
				t -= std::log(1 - random()) * invMaxDensity / avg(extinction * density);
				if (t >= ri.tFar) break;
				float density = lbvh->sampleAt(incoming, t);
				Tr *= 1 - std::max(0.0f, density * invMaxDensity);
				const float rrThreshold = .1;
				if (Tr < rrThreshold) {
					float q = std::max(0.05f, 1.0f - Tr);
					if (random() < q) return glm::vec3(0.0f);
					Tr /= 1 - q;
				}
			}

			//std::cout << "after ratio " << Tr << std::endl;
			return glm::vec3(Tr);
		}

		//delta tracking
		glm::vec3 sample(Ray incoming, Ray& scattered, RayIntersection intersection) {
			scattered.o = incoming.o;
			scattered.d = incoming.d;

			//travverse now uses OCS
			/*glm::vec3 hit = lbvh->traverseTreeUntil(incoming, 99999);
			//if the ray doesn't collide with any voxel within the tree, there is no scattering.
			if (hit.x > hit.y) {
				float distAABB = intersection.tFar - intersection.tNear;
				scattered.o = incoming.getPointAt(distAABB + 0.001f);
				scattered.d = incoming.d;
				return glm::vec3(1.0f,0,0);
			}*/

			// Run delta-tracking iterations to sample a medium interaction
			//float t = hit.x;
			float t = intersection.tNear;
			//float dist = hit.y - hit.x;
			while (true) {
				float r = random();
				float sampledDist = std::log(1 - r) * invMaxDensity / avg(extinction * density);
				t -= sampledDist;
				if (t >= intersection.tFar) {
					scattered.o = incoming.getPointAt(intersection.tFar + 0.0001f);
					break;
				}
				this->t -= sampledDist;

				float density = lbvh->sampleAt(incoming, t);
				float ra = random();
				if (density * invMaxDensity > ra) {
					scattered.o = incoming.getPointAt(t);
					scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, glm::vec3(0,0,1)); //-incoming.d
					tr *= scattering / extinction;
					return scattering / extinction;
				}
			}

			return glm::vec3(1.0f);
		}
	};
}