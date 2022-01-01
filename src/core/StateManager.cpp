#include "core/StateManager.h"

namespace narvalengine {
	StateManager::StateManager() {
	}

	StateManager::~StateManager() {
	}

	void StateManager::addState(EngineState *state) {
		states.push_back(state);
	}

	void StateManager::removeState(int id) {
		states.erase(states.begin() + id);
	}

	void StateManager::changeStateTo(int id) {
		if ((*states.at(currentState)).shouldInit()) {
			(*states.at(currentState)).init();
		}
		currentState = id;
	}

	void StateManager::variableUpdate(float deltaTime) {
		(*states.at(currentState)).variableUpdate(deltaTime);
	}

	void StateManager::update(float deltaTime) {
		(*states.at(currentState)).update(deltaTime);
	}

	void StateManager::render() {
		(*states.at(currentState)).render();
	}
}
