#include "Engine3D.h"
#include "GameStateManager.h"
#include <iostream>

int main(){
	Engine3D *engine = new Engine3D();
	GameStateManager gsm;
	engine->startGLFW();
	
	while (true) {
		engine->mainLoop();
	}
}
