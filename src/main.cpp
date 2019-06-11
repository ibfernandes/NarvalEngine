#include "Engine3D.h"
#include "GameStateManager.h"
#include "ResourceManager.h"
#include <iostream>

int main(){
	Engine3D *engine = new Engine3D();
	GameStateManager gsm;
	engine->startGLFW();

	ResourceManager::getSelf()->loadShader("monocolor", "shaders/monoColor.vert", "shaders/monoColor.frag", "");
	
	while (true) {
		engine->mainLoop();
	}
}
