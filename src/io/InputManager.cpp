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

		this->modifier = modifier;
		if (action == GLFW_REPEAT)
			action = GLFW_PRESS;

		currentState[key] = action;

		if (action == GLFW_PRESS)
			previousState[key] = GLFW_PRESS;

		/*std::cout << key << std::endl;
		if (action == GLFW_PRESS) {
			std::cout << "currentState: GLFW_PRESS" << std::endl;
			std::cout << "previousState: GLFW_PRESS" << std::endl;
		}
		else if (action == GLFW_RELEASE) {
			std::cout << "currentState: GLFW_RELEASE" << std::endl;
			if (previousState[key] == GLFW_PRESS)
				std::cout << "previousState: GLFW_PRESS" << std::endl;
			else
				std::cout << "previousState: GLFW_RELEASE" << std::endl;
		}*/
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
		if (modifier == 0 || keyWithNoMod<0 || keyWithNoMod > (numberOfKeys / sizeof(uint32_t)))
			return false;

		bool validCombo = false;
		if (modifier == GLFW_MOD_CONTROL && NE_INPUT_MOD_GET(key, NE_INPUT_MOD_CTRL))
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
}