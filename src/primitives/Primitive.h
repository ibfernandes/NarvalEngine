#pragma once
#include "primitives/Ray.h"
#include "core/RendererAPI.h"
#include <glm/glm.hpp>

namespace narvalengine {
	class Material;

	class Primitive {
	public:
		/**
		 * Material applied to this primitive.
		 */
		Material *material = nullptr;
		/**
		 * VertexLayout defining how to interpret the vertex data for this primitive.
		 */
		VertexLayout *vertexLayout = nullptr;

		bool hasMaterial() {
			if (material)
				return true;
			else
				return false;
		}

		/**
		 * Checks if Ray {@code ray} intesercts this primitive.
		 * 
		 * @param ray in Object Space System (OCS).
		 * @param hit stores data about the hit.
		 * @return true if ray intersects this primitive. False otherwise.
		 */
		virtual bool intersect(Ray ray, RayIntersection &hit) = 0;

		/**
		 * Samples a random point in this primitive's surface. The point is returned in World Coordinate System (WCS).
		 *
		 * @param interaction
		 * @param transformToWCS transform matrix for this {@code primitive}.
		 * @return sampled point in World Coordinate System (WCS).
		 */
		virtual glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) = 0;
		
		/**
		 * Given a hit point on this primitive surface, convert it to UV normalized texture coordinate.
		 * 
		 * @param pointOnSurface in Object Coordinate System (OCS).
		 * @return UV coordinates.
		 */
		virtual glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface) = 0;
		
		/**
		 * The Probability Density Function (PDF) of choosing a point in this primitive
		 * relative to a given {@code interaction}.
		 * 
		 * @param interaction in another primitive.
		 * @param transformToWCS transform matrix for this {@code primitive}.
		 * @return the PDF.
		 */
		virtual float pdf(RayIntersection interaction, glm::mat4 transformToWCS) = 0;
	};
}
