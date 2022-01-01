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
	/**
	 * Singleton responsible for managing inputs.
	 */
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
		int currentModifier = 0;
		glm::vec2 mousePosition;

		void init();
		bool isCombo(int v);
	};
}