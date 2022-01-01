#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "core/Settings.h"
#include "primitives/InstancedModel.h"
#include <vector>

namespace narvalengine {
	/**
	 * A scene is responsible for storing numerous instanced models and thus, compose a scene.
	 */
	class Scene {
	public:
		Scene();
		~Scene();
		bool shouldLoad = true;
		std::vector<InstancedModel*> instancedModels;
		std::vector<InstancedModel*> lights;
		SceneSettings settings{};

		virtual void load();
		virtual void update(float deltaTime);
		virtual void variableUpdate(float deltaTime);
		virtual void render();

		/**
		 * Tests if a ray intersects this scene. Ray must be in the World Coordinate System (WCS).
		 * 
		 * @param ray in the World Coordinate System (WCS).
		 * @param hit where the hit information will be stored.
		 * @param tMin minimun distance t for a collission to be valid.
		 * @param tMax maximun distance t for a collision to be valid.
		 * @return true if intersected. False otherwise.
		 */
		bool intersectScene(Ray ray, RayIntersection &hit, float tMin, float tMax);
	};
}

