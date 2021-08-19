#pragma once
#include <iostream>
#include <map>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#define NE_INPUT_KEY_MASK 0x1FF
#define NE_INPUT_MOD_SHIFT 0x010
#define NE_INPUT_MOD_CTRL 0x011
#define NE_INPUT_MOD_ALT 0x012

#define NE_INPUT_MOD_SET(v, shift) ( ((1) << shift) + v)
#define NE_INPUT_MOD_GET(v, shift) ((v >> shift) & 1)

namespace narvalengine {
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
		static void mouse_key_callback_handler(GLFWwindow* window, int button, int action, int mods) {
			InputManager::getSelf()->mouse_key_callback(button, action, mods);
		}
		static void mouse_pos_callback_handler(GLFWwindow* window, double x, double y) {
			InputManager::getSelf()->mouse_pos_callback(x, y);
		}

		void key_callback(int key, int scancode, int action, int modifier);
		void mouse_key_callback(int button, int action, int modifier);
		void mouse_pos_callback(double x, double y);
		bool isKeyBeingHoldDown(int key);
		bool keyReleased(int key);
		bool keyReleasedCombo(int key);
		bool eventTriggered(std::string actionName);
		glm::vec2 getMousePosition();

	private:
		InputManager();
		static InputManager *self;
		static const int numberOfKeys = 512;
		int previousState[numberOfKeys];
		int currentState[numberOfKeys];
		int modifier = 0;
		glm::vec2 mousePosition;

		bool isCombo(int v) {
			return NE_INPUT_MOD_GET(v, NE_INPUT_MOD_SHIFT) == 1 || NE_INPUT_MOD_GET(v, NE_INPUT_MOD_CTRL) == 1 || NE_INPUT_MOD_GET(v, NE_INPUT_MOD_ALT) == 1;
		}

		void init() {
			std::fill_n(previousState, numberOfKeys, GLFW_RELEASE);
			std::fill_n(currentState, numberOfKeys, GLFW_RELEASE);

			//TODO: implement combos (ALT+R etc...)
			keyMapping.insert({ "MOVE_FORWARD", {KeyHolding, GLFW_KEY_W} });
			keyMapping.insert({ "MOVE_BACKWARDS", {KeyHolding, GLFW_KEY_S} });
			keyMapping.insert({ "MOVE_LEFT", {KeyHolding, GLFW_KEY_A} });
			keyMapping.insert({ "MOVE_RIGHT", {KeyHolding, GLFW_KEY_D} });
			keyMapping.insert({ "MOVE_UPWARDS", {KeyHolding, GLFW_KEY_Q} });
			keyMapping.insert({ "MOVE_DOWNWARDS", {KeyHolding, GLFW_KEY_E} });

			//keyMapping.insert({ "PITCH_UP", {KeyHolding, GLFW_KEY_KP_8} });
			keyMapping.insert({ "PITCH_UP", {KeyHolding, GLFW_KEY_T} });
			//keyMapping.insert({ "PITCH_UP", {KeyHolding, GLFW_KEY_UP} });

			//keyMapping.insert({ "PITCH_DOWN", {KeyHolding, GLFW_KEY_KP_2} });
			keyMapping.insert({ "PITCH_DOWN", {KeyHolding, GLFW_KEY_G} });
			//keyMapping.insert({ "PITCH_DOWN", {KeyHolding, GLFW_KEY_DOWN} });

			//keyMapping.insert({ "YAW_RIGHT", {KeyHolding, GLFW_KEY_KP_6} });
			keyMapping.insert({ "YAW_RIGHT", {KeyHolding, GLFW_KEY_F} });
			//keyMapping.insert({ "YAW_RIGHT", {KeyHolding, GLFW_KEY_RIGHT} });

			//keyMapping.insert({ "YAW_LEFT", {KeyHolding, GLFW_KEY_KP_4} });
			keyMapping.insert({ "YAW_LEFT", {KeyHolding, GLFW_KEY_H} });
			//keyMapping.insert({ "YAW_LEFT", {KeyHolding, GLFW_KEY_LEFT} });

			keyMapping.insert({ "MOUSE_MOVE_CAMERA", {KeyHolding, GLFW_MOUSE_BUTTON_MIDDLE} });

			keyMapping.insert({ "SELECT_OBJECT", {KeyRelease, GLFW_MOUSE_BUTTON_LEFT} });

			keyMapping.insert({ "ROTATE_OBJECT", {KeyRelease, NE_INPUT_MOD_SET(GLFW_KEY_R, NE_INPUT_MOD_CTRL)} });
			keyMapping.insert({ "SCALE_OBJECT", {KeyRelease, NE_INPUT_MOD_SET(GLFW_KEY_S, NE_INPUT_MOD_CTRL)} });
			keyMapping.insert({ "TRANSLATE_OBJECT", {KeyRelease, NE_INPUT_MOD_SET(GLFW_KEY_T, NE_INPUT_MOD_CTRL)} });
		}

	};
}