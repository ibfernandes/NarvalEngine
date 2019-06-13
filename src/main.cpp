#include "Engine3D.h"
#include "GameStateManager.h"
#include "ResourceManager.h"
#include <iostream>
#include <vector>

void generateTestModel(){
	float vertices[] = {
		-1, -1, 0,
		1, -1, 0,
		1, 1, 0,

		-1, -1, 0,
		1, 1, 0,
		-1, 1, 0
	};

	float texcoords[] = {
		0, 1,
		1, 1,
		1, 0,

		0, 1,
		1, 0,
		0, 0
	};

	std::vector<GLfloat> *values = new std::vector<GLfloat>();
	std::vector<int> *layout = new std::vector<int>();
	(*layout).push_back(3);
	(*layout).push_back(2);

	for (int i = 0; i < 6; i++) {
		(*values).push_back(vertices[(i * 3)]);
		(*values).push_back(vertices[(i * 3) + 1]);
		(*values).push_back(vertices[(i * 3) + 2]);
		(*values).push_back(texcoords[(i * 2)]);
		(*values).push_back(texcoords[(i * 2) + 1]);
	}


	Model model;
	model.loadVerAttrib(&(*values).front(), (*values).size(), &(*layout).front(), (*layout).size());
	ResourceManager::getSelf()->addModel("quadTest", model);
}

int main(){
	Engine3D *engine = new Engine3D();
	engine->init();

	GameStateManager gsm;
	ResourceManager::getSelf()->loadShader("monocolor", "shaders/monoColor.vert", "shaders/monoColor.frag", "");
	generateTestModel();

	engine->mainLoop();
}
