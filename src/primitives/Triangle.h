#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"
#include <glog/logging.h>

namespace narvalengine {
	class Triangle : public Primitive {
	public:
		/**
		 * Pointers to Model OCS vertex data
		 * The layout for each vertex can vary, as per Model layout options.
		 */
		float *vertexData[3]{};
		float *normal = nullptr;

		Triangle();
		~Triangle();
		/**
		 * Uses three float vectors made of 3 floating-points each to create the vertices that make this triangle.
		 * 
		 * @param index1 a pointer to a float[3].
		 * @param index2 a pointer to a float[3].
		 * @param index3 a pointer to a float[3].
		 * @param material pointer to this triangle material.
		 * @param normal
		 */
		Triangle(float* index1, float* index2, float* index3, Material* material, float* normal);

		/**
		 * Uses three float vectors made of 3 floating-points each to create the vertices that make this triangle.
		 * 
		 * @param index1 a pointer to a float[3].
		 * @param index2 a pointer to a float[3].
		 * @param index3 a pointer to a float[3].
		 * @param material pointer to this triangle material.
		 */
		Triangle(float* index1, float* index2, float* index3, Material* material);

		/**
		 * Uses three float vectors made of 3 floating-points each to create the vertices that make this triangle.
		 * 
		 * @param index1 a pointer to a float[3].
		 * @param index2 a pointer to a float[3].
		 * @param index3 a pointer to a float[3].
		 */
		Triangle(float* index1, float* index2, float* index3);

		glm::vec3 getVertex(int n);

		/**
		 * Checks if Ray r intesercts this primitive. If true, stores its values on hit.
		 * Ray must be in Object Coordinate Space (OCS).
		 * Source: https://www.iquilezles.org/www/articles/intersectors/intersectors.htm.
		 * 
		 * @param ray in Object Coordinate Space (OCS).
		 * @param hit in which to store the hit data, if any.
		 * @return true if hit. False otherwise.
		 */
		bool intersect(Ray ray, RayIntersection &hit) override;
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) override;
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface) override;
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS) override;
		glm::vec3 barycentricCoordinates(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c);
	};
}