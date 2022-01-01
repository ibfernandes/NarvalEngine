#pragma once
#include "primitives/BucketLBVH.h"
#include "materials/Texture.h"
#include "materials/Medium.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "primitives/InstancedModel.h"
#include "core/BSDF.h"
#include <vector>
#include <math.h>

namespace narvalengine {

	class GridMedia : public Medium {
	private:
		float calculateMaxDensity() {
			float max = 0;
			for (int i = 0; i < grid->depth * grid->height * grid->width; i++)
				max = glm::max(max , grid->sampleAtIndex(i).x);
			return max;
		}

		/**
		 * Converts from Object Coordinate System (OCS) to Grid Coordinate System (GCS).
		 * Note that for easy of implementation the OCS is assumed to be an AABB centered at origin with width = 1.
		 * Hence, the OCS must be in between [-0.5, -0.5, -0.5] and [0.5, 0.5, 0.5].
		 *
		 * @param x in Object Coordinate System (OCS).
		 * @param y in Object Coordinate System (OCS).
		 * @param z in Object Coordinate System (OCS).
		 * @return the coordinates in GCS, varying from [0, 0, 0] and [width, height, depth].
		 */
		glm::vec3 fromOCStoGCS(float x, float y, float z) {
			return (glm::vec3(x, y, z) + glm::vec3(0.5)) * resolution;
		}

		/**
		 * Samples density in this Grid Media at {@code ray.getPointAt(depth)}.
		 * 
		 * @param ray in Object Coordinate System (OCS).
		 * @param depth distance t.
		 * @return sampled density.
		 */
		float sampleAt(Ray &ray, float depth);

		/**
		 * Samples the volume density at {@code gridPoint}.
		 *
		 * @param gridPoint in Grid Coordinate System (GCS) ranging from [0,0,0] to [width, height, depth].
		 * @return sampled density.
		 */
		float density(glm::vec3 gridPoint);

		/**
		 *  Samples the volume density at {@code gridPoint} using trilinear interpolation.
		 *
		 * @param gridPoint in Grid Coordinate System (GCS) ranging from [0,0,0] to [width, height, depth].
		 * @return sampled density.
		 */
		float interpolatedDensity(glm::vec3 gridPoint);
	public:
		glm::vec3 absorption;
		glm::vec3 scattering;
		glm::vec3 extinction;
		glm::vec3 resolution;
		Texture *grid;
		float densityMultiplier =  1.0f;
		float invMaxDensity = 1;

		GridMedia(glm::vec3 scattering, glm::vec3 absorption, Texture *tex, float density);

		/**
		 * Perform Ratio Tracking to estimate the transmittance value between {@code intersection.tNear} and {@code intersection.tFar}.
		 * 
		 * @param incoming ray in World Coordinate Space (WCS).
		 * @param intersection.
		 * @return 
		 */
		glm::vec3 Tr(Ray incoming, RayIntersection intersection);

		/**
		 * Samples the transmittance and scattered event using Delta Tracking.
		 * 
		 * @param incoming ray in World Coordinate Space (WCS).
		 * @param scattered ray in World Coordinate Space (WCS) where scattered.origin is the point in which the scattering event occured and scattered.direction the sampled direction using the current phase function.
		 * @param intersection inside or at this volume's border.
		 * @return calculated transmittance.
		 */
		glm::vec3 sample(Ray incoming, Ray& scattered, RayIntersection intersection);
	};
}