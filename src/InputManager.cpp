#include "InputManager.h"

InputManager *InputManager::self = 0;

InputManager* InputManager::getSelf() {
	if (self == 0)
		self = new InputManager();
	return self;
}

InputManager::InputManager() {
	this->init();
}

