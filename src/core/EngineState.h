#pragma once

namespace narvalengine {
	class EngineState {
	private:
		bool mustBeInitialized = true;
	public:
		virtual void init();
		virtual void update(float deltaTime);
		virtual void variableUpdate(float deltaTime);
		virtual void render();
		bool shouldInit();
		EngineState();
		~EngineState();
	};
}
