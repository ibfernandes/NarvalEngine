#include "primitives/InstancedModel.h"

namespace narvalengine {
	InstancedModel::InstancedModel() {
		modelID = NE_INVALID_STRING_ID;
		model = nullptr;
		transformToWCS = glm::mat4(1);
		invTransformToWCS = glm::mat4(1);
	}

	InstancedModel::InstancedModel(Model* model, StringID modelID, glm::mat4 transformToWCS) {
		this->model = model;
		this->modelID = modelID;
		this->transformToWCS = transformToWCS;
		this->invTransformToWCS = glm::inverse(transformToWCS);
	}

	InstancedModel::InstancedModel(Model* model, glm::mat4 transformToWCS) {
		this->model = model;
		this->transformToWCS = transformToWCS;
		this->invTransformToWCS = glm::inverse(transformToWCS);
	}

	bool InstancedModel::intersect(Ray ray, RayIntersection& hit, float& tMin, float& tMax) {
		if (!isCollisionEnabled)
			return false;
		ray = transformRay(ray, invTransformToWCS);

		bool didIntersect = model->intersect(ray, hit, tMin, tMax);

		if (didIntersect) {
			hit.hitPoint = transformToWCS * glm::vec4(hit.hitPoint, 1);
			hit.normal = glm::normalize(transformToWCS * glm::vec4(hit.normal, 0));
			hit.instancedModel = this;
		}

		return didIntersect;
	}
};