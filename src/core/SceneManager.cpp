#include "core/SceneManager.h"

namespace narvalengine {
	SceneManager::SceneManager() {
	}

	SceneManager::~SceneManager() {
	}

	void SceneManager::addState(Scene *scene) {
		scenes.push_back(scene);
	}

	void SceneManager::removeState(int id) {
		scenes.erase(scenes.begin() + id);
	}

	void SceneManager::changeStateTo(int id) {
		if ((*scenes.at(currentScene)).shouldLoad) {
			(*scenes.at(currentScene)).load();
			(*scenes.at(currentScene)).shouldLoad = false;
		}
		currentScene = id;
	}

	void SceneManager::variableUpdate(float deltaTime) {
		(*scenes.at(currentScene)).variableUpdate(deltaTime);
	}

	void SceneManager::update(float deltaTime) {
		(*scenes.at(currentScene)).update(deltaTime);
	}

	void SceneManager::render() {
		(*scenes.at(currentScene)).render();
	}
}
