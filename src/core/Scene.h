#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "core/Settings.h"
#include "primitives/InstancedModel.h"
#include <vector>

namespace narvalengine {
	class Scene {
	public:
		Scene();
		~Scene();
		bool shouldLoad = true;
		std::vector<InstancedModel*> instancedModels;
		std::vector<InstancedModel*> lights; //Light ou primitive 
		Settings settings;

		virtual void load();
		virtual void update(float deltaTime);
		virtual void variableUpdate(float deltaTime);
		virtual void render();
		bool intersectScene(Ray r, RayIntersection &hit, float tMin, float tMax);
	};
}

