#pragma once
#include "primitives/Model.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/StringID.h"

namespace narvalengine {
	class InstancedModel {
	public:
		StringID modelID = NE_INVALID_STRING_ID;
		Model *model;
		glm::mat4 transformToWCS;
		glm::mat4 invTransformToWCS;

		InstancedModel();
		InstancedModel(Model* model, StringID modelID, glm::mat4 transformToWCS);
		InstancedModel(Model* model, glm::mat4 transformToWCS);
		/*
			Receives a ray in WCS, converts it to OCS and tests intersection with this model's primitives
		*/
		bool intersect(Ray ray, RayIntersection& hit, float& tMin, float& tMax);
	};
}