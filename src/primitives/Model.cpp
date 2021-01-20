#include "Model.h"

namespace narvalengine {
	Model::Model() {
	}

	Model::~Model() {
	}

	AABB* Model::getAABB() {
		glm::vec3 min(INFINITY);
		glm::vec3 max(-INFINITY);

		//TODO case sphere?
		//TODO offsed stride of vertexData (i + stride)
		for (int i = 0; i < vertexDataLength; i = i + 3) {
			glm::vec3 vertex = glm::vec3(vertexData[i], vertexData[i + 1], vertexData[i + 2]);
			min = glm::min(min, vertex);
			max = glm::max(max, vertex);
		}

		return new AABB(min, max);
	}

	/*
		Centers this AABB model at origin in OCS
	*/
	void Model::centralize() {
		AABB *aabb = getAABB();
		glm::vec3 aabbCenter = aabb->getCenter();

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, -aabbCenter);

		for (int i = 0; i < vertexDataLength; i = i + 3) {
			glm::vec3 vertex = glm::vec3(vertexData[i], vertexData[i + 1], vertexData[i + 2]);
			vertex = transform * glm::vec4(vertex, 1.0f);
			vertexData[i] = vertex.x;
			vertexData[i + 1] = vertex.y;
			vertexData[i + 2] = vertex.z;
		}
	}

	//TODO: add or generate primitive, to also guard putting the primitive in the worng list, i.e. ones with light MUST go to lights

	Primitive* Model::getRandomLightPrimitive() {
		if (lights.size() == 0)
			return nullptr;
		float r = random();
		int i = lights.size() * r;

		return lights.at(i);
	}
}
