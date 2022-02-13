#include "core/Scene.h"
#include <iostream>

namespace narvalengine {

	Scene::Scene() {
	}

	void Scene::update(float deltaTime) {
	}

	void Scene::variableUpdate(float deltaTime) {
	}

	void Scene::load() {
	}

	void Scene::render() {
	}

	Scene::~Scene() {
		for (int i = 0; i < instancedModels.size(); i++)
			delete instancedModels[i];
		for (int i = 0; i < lights.size(); i++)
			delete lights[i];
		instancedModels.clear();
		lights.clear();
	}

	bool Scene::intersectScene(Ray ray, RayIntersection &hit, float tMin, float tMax) {
		bool didIntersect = false;
		hit.primitive = nullptr;
		RayIntersection tempIntersec;
		float currentMin = tMin;
		float currentMax = tMax;

		for (int i = 0; i < instancedModels.size(); i++) {
			bool thisIntersected = instancedModels.at(i)->intersect(ray, tempIntersec, currentMin, currentMax);

			if (thisIntersected) 
				hit = tempIntersec;

			didIntersect = didIntersect || thisIntersected;
		}

		for (int i = 0; i < lights.size(); i++) {
			bool thisIntersected = lights.at(i)->intersect(ray, tempIntersec, currentMin, currentMax);

			if (thisIntersected) 
				hit = tempIntersec;

			didIntersect = didIntersect || thisIntersected;
		}

		return didIntersect;
	}
}