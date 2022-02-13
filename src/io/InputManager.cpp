#include "InputManager.h"

namespace narvalengine {
	InputManager *InputManager::self = 0;

	InputManager* InputManager::getSelf() {
		if (self == 0)
			self = new InputManager();
		return self;
	}

	InputManager::InputManager() {
		this->init();
	}

	void InputManager::key_callback(int key, int scancode, int action, int modifier) {
		if (key<0 || key>(numberOfKeys / sizeof(uint32_t)))
			return;

		this->currentModifier = modifier;
		if (action == GLFW_REPEAT)
			action = GLFW_PRESS;

		currentState[key] = action;

		if (action == GLFW_PRESS)
			previousState[key] = GLFW_PRESS;
	}

	void InputManager::mouse_key_callback(int key, int action, int modifier) {
		if (key<0 || key>(numberOfKeys / sizeof(uint32_t)))
			return;

		if (action == GLFW_REPEAT)
			action = GLFW_PRESS;

		currentState[key] = action;

		if (action == GLFW_PRESS)
			previousState[key] = GLFW_PRESS;
	}

	void InputManager::mouse_pos_callback(double x, double y) {
		mousePosition = glm::vec2(x, y);
	}

	bool InputManager::keyReleased(int key) {
		if (key<0 || key>(numberOfKeys / sizeof(uint32_t)))
			return false;

		if (currentState[key] == GLFW_RELEASE && previousState[key] == GLFW_PRESS){
			previousState[key] = GLFW_RELEASE;
			return true;
		}else
			return false;
	}

	bool InputManager::isKeyBeingHoldDown(int key) {
		if (key<0 || key>(numberOfKeys / sizeof(uint32_t)))
			return false;

		if (currentState[key] == GLFW_PRESS)
			return true;
		else
			return false;
	}

	bool InputManager::keyReleasedCombo(int key) {
		int keyWithNoMod = key & NE_INPUT_KEY_MASK;
		if (currentModifier == 0 || keyWithNoMod<0 || keyWithNoMod > (numberOfKeys / sizeof(uint32_t)))
			return false;

		bool validCombo = false;
		if (currentModifier == GLFW_MOD_CONTROL && NE_INPUT_MOD_GET(key, NE_INPUT_MOD_CTRL))
			validCombo = true;

		if (currentState[keyWithNoMod] == GLFW_RELEASE && previousState[keyWithNoMod] == GLFW_PRESS) {
			previousState[keyWithNoMod] = GLFW_RELEASE;

			if (validCombo)
				return true;
			else
				return false;
		}else
			return false;
	}

	bool InputManager::eventTriggered(std::string actionName) {
		if (keyMapping.count(actionName) == 0)
			return false;

		if (isCombo(keyMapping.at(actionName).second))
			return keyReleasedCombo(keyMapping.at(actionName).second);
		if (keyMapping.at(actionName).first == KeyHolding)
			return isKeyBeingHoldDown(keyMapping.at(actionName).second);
		else if (keyMapping.at(actionName).first == KeyRelease)
			return keyReleased(keyMapping.at(actionName).second);

		return false;
	}

	glm::vec2 InputManager::getMousePosition() {
		return mousePosition;
	}

	bool InputManager::isCombo(int v) {
		return NE_INPUT_MOD_GET(v, NE_INPUT_MOD_SHIFT) == 1 || NE_INPUT_MOD_GET(v, NE_INPUT_MOD_CTRL) == 1 || NE_INPUT_MOD_GET(v, NE_INPUT_MOD_ALT) == 1;
	}

	void InputManager::init() {
		std::fill_n(previousState, numberOfKeys, GLFW_RELEASE);
		std::fill_n(currentState, numberOfKeys, GLFW_RELEASE);

		keyMapping.insert({ "MOVE_FORWARD", {KeyHolding, GLFW_KEY_W} });
		keyMapping.insert({ "MOVE_BACKWARDS", {KeyHolding, GLFW_KEY_S} });
		keyMapping.insert({ "MOVE_LEFT", {KeyHolding, GLFW_KEY_A} });
		keyMapping.insert({ "MOVE_RIGHT", {KeyHolding, GLFW_KEY_D} });
		keyMapping.insert({ "MOVE_UPWARDS", {KeyHolding, GLFW_KEY_Q} });
		keyMapping.insert({ "MOVE_DOWNWARDS", {KeyHolding, GLFW_KEY_E} });

		keyMapping.insert({ "PITCH_UP", {KeyHolding, GLFW_KEY_T} });
		keyMapping.insert({ "PITCH_DOWN", {KeyHolding, GLFW_KEY_G} });
		keyMapping.insert({ "YAW_RIGHT", {KeyHolding, GLFW_KEY_F} });
		keyMapping.insert({ "YAW_LEFT", {KeyHolding, GLFW_KEY_H} });

		keyMapping.insert({ "MOUSE_MOVE_CAMERA", {KeyHolding, GLFW_MOUSE_BUTTON_MIDDLE} });

		keyMapping.insert({ "SELECT_OBJECT", {KeyRelease, GLFW_MOUSE_BUTTON_LEFT} });

		keyMapping.insert({ "ROTATE_OBJECT", {KeyRelease, NE_INPUT_MOD_SET(GLFW_KEY_R, NE_INPUT_MOD_CTRL)} });
		keyMapping.insert({ "SCALE_OBJECT", {KeyRelease, NE_INPUT_MOD_SET(GLFW_KEY_S, NE_INPUT_MOD_CTRL)} });
		keyMapping.insert({ "TRANSLATE_OBJECT", {KeyRelease, NE_INPUT_MOD_SET(GLFW_KEY_T, NE_INPUT_MOD_CTRL)} });

		keyMapping.insert({ "TESTING_KEY", {KeyHolding, GLFW_KEY_1} });

		keyMapping.insert({ "NODE_EDITOR_ADD_NEW_NODE", {KeyRelease, GLFW_MOUSE_BUTTON_RIGHT} });

	}
}