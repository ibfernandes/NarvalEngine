#pragma once
#include <map>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class InputManager {
public:
	enum EventType {
		KeyHolding, KeyRelease
	};

	std::map<std::string, std::pair<EventType, int>> keyMapping;
	static InputManager *getSelf();

	static void key_callback_handler(GLFWwindow* window, int key, int scancode, int action, int mode) {
		InputManager::getSelf()->key_callback(key, scancode, action, mode);
	}

	void key_callback(int key, int scancode, int action, int mode) {
		if (key<0 || key>(sizeof(pressingKeys) / sizeof(pressingKeys[0])))
			return;
		pressingKeys[key] = action != GLFW_RELEASE;
	}

	bool isKeyBeingHoldDown(int key) {
		if (key<0 || key>(sizeof(pressingKeys) / sizeof(pressingKeys[0])))
			return false;
		return pressingKeys[key];
	}

	bool eventTriggered(std::string actionName) {
		if (keyMapping.count(actionName) == 0)
			return false;

		if (keyMapping.at(actionName).first == KeyHolding)
			return isKeyBeingHoldDown(keyMapping.at(actionName).second);

		return false;
	}

private:
	InputManager();
	static InputManager *self;
	bool pressingKeys[512] = {false};
	
	void init() {
		keyMapping.insert({ "MOVE_FORWARD", {KeyHolding, GLFW_KEY_W} });
		keyMapping.insert({ "MOVE_BACKWARDS", {KeyHolding, GLFW_KEY_S} });
		keyMapping.insert({ "MOVE_LEFT", {KeyHolding, GLFW_KEY_A} });
		keyMapping.insert({ "MOVE_RIGHT", {KeyHolding, GLFW_KEY_D} });
		keyMapping.insert({ "MOVE_UPWARDS", {KeyHolding, GLFW_KEY_Q} });
		keyMapping.insert({ "MOVE_DOWNWARDS", {KeyHolding, GLFW_KEY_E} });

		keyMapping.insert({ "PITCH_UP", {KeyHolding, GLFW_KEY_KP_8} });
		keyMapping.insert({ "PITCH_DOWN", {KeyHolding, GLFW_KEY_KP_2} });

		keyMapping.insert({ "YAW_RIGHT", {KeyHolding, GLFW_KEY_KP_6} });
		keyMapping.insert({ "YAW_LEFT", {KeyHolding, GLFW_KEY_KP_4} });
	}
	
};