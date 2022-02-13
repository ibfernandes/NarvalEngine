#pragma once
#include "primitives/Primitive.h"
#include "primitives/Ray.h"
#include "utils/Math.h"

namespace narvalengine {
	class AABB : public Primitive {
	public:
		float *vertexData[2]{};

		AABB();
		AABB(float *v1, float *v2);
		AABB(glm::vec3 v1, glm::vec3 v2);
		/**
		 * Calculates the half width, height and depth of this AABB.
		 * 
		 * @return size/2.
		 */
		glm::vec3 getHalfSize();
		/**
		 * Calculates the width, height and depth of this AABB.
		 * 
		 * @return size.
		 */
		glm::vec3 getSize();
		/**
		 * Calculates the center point of this AABB.
		 * 
		 * @return point.
		 */
		glm::vec3 getCenter();
		glm::vec3 getVertex(int n);

		/**
		 * Checks if {@code ray} intesercts this primitive. If true, stores its values on hit.
		 * Ray must be in Object Coordiante System (OCS). Assumes the box is centered at origin.
		 * Source: https://iquilezles.org/www/articles/boxfunctions/boxfunctions.htm
		 *
		 * @param ray in Object Coordinate System (OCS).
		 * @param hit stores data about the hit.
		 * @return true if ray intersects this primitive. False otherwise.
		 */
		bool intersect(Ray ray, RayIntersection &hit);
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
		void calculateAABB(glm::vec3& min, glm::vec3& max);
	};
}