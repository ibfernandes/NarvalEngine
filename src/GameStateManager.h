#pragma once
#include "Scene.h"
#include <vector>
#include <iostream>

class GameStateManager {
public:
	GameStateManager();
	~GameStateManager();

	std::vector<Scene*> scenes;
	int currentScene = 0;

	void addState(Scene *scene) {
		scenes.push_back(scene);
	}

	void removeState(int id) {
		scenes.erase(scenes.begin() + id);
	}

	void changeStateTo(int id) {
		currentScene = id;
	}

	void variableUpdate(float deltaTime) {
		(*scenes.at(currentScene)).variableUpdate(deltaTime);
	}

	void update(float deltaTime) {
		(*scenes.at(currentScene)).update(deltaTime);
	}

	void render() {
		(*scenes.at(currentScene)).render();
	}
};
