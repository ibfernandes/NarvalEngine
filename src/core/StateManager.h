#pragma once
#include "core/EngineState.h"
#include <vector>
#include <iostream>

namespace narvalengine {
	class StateManager {
	private:
		int currentState = 0;
		std::vector<EngineState*> states;
	public:
		StateManager();
		~StateManager();
		void addState(EngineState *state);
		void removeState(int id);
		void changeStateTo(int id);
		void variableUpdate(float deltaTime);
		void update(float deltaTime);
		void render();
	};
}