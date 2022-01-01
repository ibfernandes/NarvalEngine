#pragma once
#include "primitives/Model.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/StringID.h"

namespace narvalengine {
	/**
	 * An instance of a model with its own world transformation matrix.
	 */
	class InstancedModel {
	private:
		InstancedModel();
	public:
		StringID modelID = NE_INVALID_STRING_ID;
		bool isCollisionEnabled = true;
		Model *model = nullptr;
		glm::mat4 transformToWCS;
		glm::mat4 invTransformToWCS;

		InstancedModel(Model* model, StringID modelID, glm::mat4 transformToWCS);
		InstancedModel(Model* model, glm::mat4 transformToWCS);
		/**
		 * Receives a ray in World Coordinate System (WCS), converts it to OCS and tests intersection with this model.
		 * 
		 * @param ray in World Coordinate System (WCS).
		 * @param hit in which to store the intersection data.
		 * @param tMin minimum distance for a valid intersection to occur.
		 * @param tMax maximum distance for a valid intersection to occur.
		 * @return true if intersects this model. False otherwise.
		 */
		bool intersect(Ray ray, RayIntersection& hit, float& tMin, float& tMax);
	};
}