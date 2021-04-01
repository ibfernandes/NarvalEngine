#include "core/Scene.h"
#include <iostream>

namespace narvalengine {

	Scene::Scene() {
	}

	Scene::~Scene() {
	}

	void Scene::update(float deltaTime) {
	}

	void Scene::variableUpdate(float deltaTime) {
	}

	void Scene::load() {
	}

	void Scene::render() {
	}

	/*
		Receives a ray in WCS
	*/
	bool Scene::intersectScene(Ray r, RayIntersection &hit, float tMin, float tMax) {
		bool didIntersect = false;
		hit.primitive = nullptr;
		RayIntersection tempIntersec;
		float currentMin = tMin;
		float currentMax = tMax;

		for (int i = 0; i < instancedModels.size(); i++) {
			bool thisIntersected = instancedModels.at(i)->intersect(r, tempIntersec, currentMin, currentMax);

			if (thisIntersected) {
				hit = tempIntersec;
				//hit.instancedModel = instancedModels.at(i);
			}

			didIntersect = didIntersect || thisIntersected;
		}

		for (int i = 0; i < lights.size(); i++) {
			bool thisIntersected = lights.at(i)->intersect(r, tempIntersec, currentMin, currentMax);

			if (thisIntersected) {
				hit = tempIntersec;
				//hit.instancedModel = instancedModels.at(i);
			}

			didIntersect = didIntersect || thisIntersected;
		}

		return didIntersect;
	}
}