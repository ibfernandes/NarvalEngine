#define GLM_FORCE_LEFT_HANDED true
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

void generateCubeTestModel() {
	//Actually, 0..1, i'm suming 0.5 later down
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f, -0.5f,  0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,

		0.5f,  0.5f,  0.5f,
		0.5f,  0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f,  0.5f,
		0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f,  0.5f, -0.5f,
		0.5f,  0.5f, -0.5f,
		0.5f,  0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f
	};

	float texcoords[] = {
		 0.0f,  0.0f,
		1.0f,  0.0f,
		1.0f,  1.0f,
		1.0f,  1.0f,
		 0.0f,  1.0f,
		 0.0f,  0.0f,

		 0.0f,  0.0f,
		1.0f,  0.0f,
		1.0f,  1.0f,
		1.0f,  1.0f,
		 0.0f,  1.0f,
		 0.0f,  0.0f,

		 1.0f,  0.0f,
		 1.0f,  1.0f,
		 0.0f,  1.0f,
		 0.0f,  1.0f,
		 0.0f,  0.0f,
		 1.0f,  0.0f,

		1.0f,  0.0f,
		1.0f,  1.0f,
		0.0f,  1.0f,
		0.0f,  1.0f,
		0.0f,  0.0f,
		1.0f,  0.0f,

		 0.0f,  1.0f,
		1.0f,  1.0f,
		1.0f,  0.0f,
		1.0f,  0.0f,
		 0.0f,  0.0f,
		 0.0f,  1.0f,

		 0.0f,  1.0f,
		1.0f,  1.0f,
		1.0f,  0.0f,
		1.0f,  0.0f,
		 0.0f,  0.0f,
		 0.0f,  1.0f
	};

	std::vector<GLfloat> *values = new std::vector<GLfloat>();
	std::vector<int> *layout = new std::vector<int>();
	(*layout).push_back(3);
	(*layout).push_back(2);

	for (int i = 0; i < 6 * 6; i++) {
		(*values).push_back(vertices[(i * 3)]+0.5);
		(*values).push_back(vertices[(i * 3) + 1]+0.5);
		(*values).push_back(vertices[(i * 3) + 2]+0.5);
		(*values).push_back(texcoords[(i * 2)]);
		(*values).push_back(texcoords[(i * 2) + 1]);
	}


	Model model;
	model.loadVerAttrib(&(*values).front(), (*values).size(), &(*layout).front(), (*layout).size());
	ResourceManager::getSelf()->addModel("cubeTest", model);
}

int main(){	
	Engine3D *engine = new Engine3D();
	engine->init();

	GameStateManager gsm;
	ResourceManager::getSelf()->loadShader("monocolor", "shaders/monoColor.vert", "shaders/monoColor.frag", "");
	ResourceManager::getSelf()->loadShader("randomVisualizer", "shaders/random.vert", "shaders/random.frag", "");
	ResourceManager::getSelf()->loadShader("billboard", "shaders/billboard.vert", "shaders/billboard.frag", "");
	ResourceManager::getSelf()->loadShader("phong", "shaders/phong.vert", "shaders/phong.frag", "");
	ResourceManager::getSelf()->loadShader("pbr", "shaders/pbr.vert", "shaders/pbr.frag", "");
	ResourceManager::getSelf()->loadShader("cloudscape", "shaders/cloudscape.vert", "shaders/cloudscape.frag", "");
	ResourceManager::getSelf()->loadShader("visibility", "shaders/visibility.vert", "shaders/visibility.frag", "");
	ResourceManager::getSelf()->loadShader("shvolume", "shaders/shvolume.vert", "shaders/shvolume.frag", "");
	ResourceManager::getSelf()->loadShader("volume", "shaders/volume.vert", "shaders/volume.frag", "");
	ResourceManager::getSelf()->loadShader("volumewcs", "shaders/volumeWCS.vert", "shaders/volumeWCS.frag", "");
	ResourceManager::getSelf()->loadShader("simpleTexture", "shaders/simpleTexture.vert", "shaders/simpleTexture.frag", "");
	ResourceManager::getSelf()->loadShader("gamma", "shaders/gammaCorrection.vert", "shaders/gammaCorrection.frag", "");
	ResourceManager::getSelf()->loadShader("pathTracingLastPass", "shaders/pathTracingLastPass.vert", "shaders/pathTracingLastPass.frag", "");
	ResourceManager::getSelf()->loadTexture2D("cloudheights", "imgs/heights.png");
	ResourceManager::getSelf()->loadTexture2D("weather", "imgs/weather.png");
	ResourceManager::getSelf()->loadTexture2D("lightbulb", "imgs/light-bulb.png");
	ResourceManager::getSelf()->loadShader("screentex", "shaders/screenTex.vert", "shaders/screenTex.frag", "");
	ResourceManager::getSelf()->loadShader("gradientBackground", "shaders/gradientBackground.vert", "shaders/gradientBackground.frag", "");
	ResourceManager::getSelf()->loadShader("volumelbvh", "shaders/volumeLBVH.vert", "shaders/volumeLBVH.frag", "");
	ResourceManager::getSelf()->loadShader("simpleLightDepth", "shaders/simpleLightDepth.vert", "shaders/simpleLightDepth.frag", "");

	ResourceManager::getSelf()->loadVDBasTexture3D("cloud", "vdb/wdas_cloud_sixteenth.vdb"); //512
	ResourceManager::getSelf()->loadVDBasTexture3D("cloudlowres", "vdb/wdas_cloud_eighth.vdb"); //512
	ResourceManager::getSelf()->loadVDBasTexture3D("smoke", "vdb/colored_smoke.vdb");
	ResourceManager::getSelf()->loadVDBasTexture3D("fireball", "vdb/fireball.vdb");
	ResourceManager::getSelf()->loadVDBasTexture3D("bunny", "vdb/bunny_cloud.vdb");
	ResourceManager::getSelf()->loadVDBasTexture3D("explosion", "vdb/explosion.vdb");


	//ResourceManager::getSelf()->loadModel("hz", "models/xps/Aloy V2/", "plz.obj");
	ResourceManager::getSelf()->loadModel("xyzaxis", "models/xyzaxis/", "arrows.obj");
	ResourceManager::getSelf()->loadModel("sponza", "models/sponza/", "sponza.obj");

	generateCubeTestModel();
	generateTestModel();

	engine->mainLoop();
}
