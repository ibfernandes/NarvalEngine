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
		float area = 0;

		Triangle();
		~Triangle();
		float calculateArea();
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
		
		glm::vec2 *getUV(int n);

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
		/**
		 * Samples a point in this triangle's surface using two random uniformly sampled numbers.
		 * Formula: P = (1 − √a)v1 + (√a(1 − b)v2 + (b√a)v3. 
		 * Where v1, v2 and v3 are the vertices and a and b are random numbers in the range [0,1].
		 * 
		 * @param interaction
		 * @param transformToWCS
		 * @return 
		 */
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) override;
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface) override;
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS) override;
		glm::vec3 barycentricCoordinates(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);
		/**
		 * Verifies if a point is within this triangle's surface.
		 * For a great visualization watch "Gamedev Maths: point in triangle" by Sebastian Lague.
		 * 
		 * @param point
		 * @return 
		 */
		bool contains(glm::vec3 point);

		glm::vec3 getCentroid();
		void calculateAABB(glm::vec3& min, glm::vec3& max);
	};
}