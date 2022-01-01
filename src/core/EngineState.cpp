#include "core/EngineState.h" 

namespace narvalengine {
	void EngineState::init() {
		mustBeInitialized = false;
	}

	bool EngineState::shouldInit() {
		return mustBeInitialized;
	}

	void EngineState::update(float deltaTime) {

	}

	void EngineState::variableUpdate(float deltaTime) {

	}

	void EngineState::render() {
	}

	EngineState::EngineState() {

	}

	EngineState::~EngineState() {

	}
}
