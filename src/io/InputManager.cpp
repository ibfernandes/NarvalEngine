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

	void InputManager::key_callback(int key, int scancode, int action, int mode) {
		if (key<0 || key>(numberOfKeys / sizeof(uint32_t)))
			return;
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

	void InputManager::mouse_key_callback(int key, int action, int mode) {
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

	bool InputManager::eventTriggered(std::string actionName) {
		if (keyMapping.count(actionName) == 0)
			return false;

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