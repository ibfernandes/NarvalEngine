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

	/*glm::mat4 model(1);
	model = glm::translate(model, {0, 0, 300 });
	
	glm::mat4 view(1);
	glm::vec3 up(0, 1, 0);
	glm::vec3 front(0, 0, 1);
	glm::vec3 side = glm::cross(front, up);
	glm::vec3 camPosition(0, 0, 0);
	view = glm::lookAt(camPosition, camPosition + front, up);

	glm::mat4 proj(1);
	proj = glm::perspective( (float)glm::radians((float)45), (float)400/(float)400, (float)1, (float)1000);


	glm::vec4 res = glm::vec4(1) * proj * view * model;


	return 0;*/



	
	Engine3D *engine = new Engine3D();
	engine->init();

	GameStateManager gsm;
	ResourceManager::getSelf()->loadShader("monocolor", "shaders/monoColor.vert", "shaders/monoColor.frag", "");
	ResourceManager::getSelf()->loadShader("cloudscape", "shaders/cloudscape.vert", "shaders/cloudscape.frag", "");
	ResourceManager::getSelf()->loadTexture2D("cloudheights", "imgs/heights.png");
	ResourceManager::getSelf()->loadTexture2D("weather", "imgs/weather.png");

	generateTestModel();

	engine->mainLoop();
}
