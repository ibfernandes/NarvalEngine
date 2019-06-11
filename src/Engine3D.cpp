#include "Engine3D.h"
#include <GLFW/glfw3.h>

Engine3D::Engine3D(){
	this->startTime = glfwGetTime();
	this->previousUpdateTime = startTime;
}


Engine3D::~Engine3D(){
}
