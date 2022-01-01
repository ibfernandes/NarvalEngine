#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"

namespace narvalengine {
	class Light {
	public:
		Primitive *primitive = nullptr;
		glm::vec3 li = glm::vec3(0);
		glm::vec3 le = glm::vec3(0);

		/**
		 * Implemented only by Infinite Area Lights.
		 * 
		 * @param ray in the World Coordinate System (WCS).
		 * @param invTransformWCS matrix to transform the ray back to Object Object Coordinate System (OCS).
		 * @return the Le term.
		 */
		virtual glm::vec3 Le(Ray ray, glm::mat4 invTransformWCS) {
			return le;
		}

		/**
		 * Return the Li term measured as Radiance.
		 * 
		 * @return the radiance Li.
		 */
		virtual glm::vec3 Li() {
			return li;
		}

		/**
		 * Samples a point in this light {@code primitive} and returns its emitted Radiance,
		 * ray pointing towards the sampled point and its PDF.
		 * 
		 * @param intersec intersection on a surface in the World Coordinate System (WCS).
		 * @param transformToWCS transform matrix for this light {@code primitive}.
		 * @param wo outgoing ray with a sampled wo.origin point at {@code primitive}'s surface and direction pointing towards wo.origin.
		 * @param lightPdf the PDF of sampling the wo.origin point.
		 * @return the emitted radiance at wo.origin.
		 */
		virtual glm::vec3 sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
			lightPdf = 0;
			return li;
		}

		virtual glm::vec3 sampleLe(Ray& fromLight, glm::mat4 transformToWCS, float& lightDirPdf, float& lightPosPdf) {
			lightDirPdf = 0;
			lightPosPdf = 0;
			return le;
		}
	};
}