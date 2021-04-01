#pragma once
#include "core/Scene.h"
#include <vector>
#include <iostream>

namespace narvalengine {
	class SceneManager {
	public:
		std::vector<Scene*> scenes;
		int currentScene = 0;

		SceneManager();
		~SceneManager();
		void addState(Scene *scene);
		void removeState(int id);
		void changeStateTo(int id);
		void variableUpdate(float deltaTime);
		void update(float deltaTime);
		void render();
	};
}