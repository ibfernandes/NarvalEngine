#define GLM_FORCE_LEFT_HANDED true
#define GLM_FORCE_ALIGNED_GENTYPES true
#define GLM_FORCE_INTRINSICS true
#define GLM_FORCE_SIMD_AVX2 true
#include "core/Engine3D.h"
#include "core/OfflineEngine.h"
#include "core/SceneManager.h"
#include "core/ResourceManager.h"
#include <iostream>
#include <vector>
#include "io/SceneReader.h"
#include "TestPlayground.h"
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

using namespace narvalengine;

int main() {
	//SceneReader scene("scenes/sponza.json", false);
	SceneReader scene("scenes/empty.json", false);
	//SceneReader scene("scenes/volumeLamp.json", false);
	Settings *settings = scene.getSettings();
	settings->renderMode = REALTIME_RENDERING_MODE;
	//TestPlayground* tp = new TestPlayground();

	Engine3D *engine = new Engine3D();
	OfflineEngine *offEngine;

	if (settings->renderMode == REALTIME_RENDERING_MODE)
		engine->init();
	else if (settings->renderMode == OFFLINE_RENDERING_MODE)
		offEngine = new OfflineEngine(*scene.getMainCamera(), *scene.getSettings(), *scene.getScene());

	if (settings->renderMode == REALTIME_RENDERING_MODE) {
		ResourceManager::getSelf()->loadModel("quad", "models/", "quad.obj");
		ResourceManager::getSelf()->loadModel("cube", "models/", "cube.obj");
		ResourceManager::getSelf()->loadTexture("default", "imgs/checkboard.png");
		ResourceManager::getSelf()->loadTexture("defaultAlt", "imgs/checkboard2.png");
		//generateTestModel();
		//generateCubeTestModel();
		//must changfe to pointer
		//ResourceManager::getSelf()->getTexture3D("cloud")->loadToOpenGL();

		ResourceManager::getSelf()->loadShader("volumeMonteCarlo", "shaders/volumeMonteCarlo.vert", "shaders/volumeMonteCarlo.frag", "");
		ResourceManager::getSelf()->loadShader("imageDifference", "shaders/imageDifference.vert", "shaders/imageDifference.frag", "");
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
		ResourceManager::getSelf()->loadTexture("cloudheights", "imgs/heights.png");
		ResourceManager::getSelf()->loadTexture("weather", "imgs/weather.png");
		ResourceManager::getSelf()->loadTexture("lightbulb", "imgs/light-bulb.png");
		ResourceManager::getSelf()->loadShader("screentex", "shaders/screenTex.vert", "shaders/screenTex.frag", "");
		ResourceManager::getSelf()->loadShader("gradientBackground", "shaders/gradientBackground.vert", "shaders/gradientBackground.frag", "");
		ResourceManager::getSelf()->loadShader("volumelbvh", "shaders/volumeLBVH.vert", "shaders/volumeLBVH.frag", "");
		ResourceManager::getSelf()->loadShader("simpleLightDepth", "shaders/simpleLightDepth.vert", "shaders/simpleLightDepth.frag", "");
		ResourceManager::getSelf()->loadShader("composeTex", "shaders/composeTex.vert", "shaders/composeTex.frag", "");

		ResourceManager::getSelf()->loadShader("MS_rayMarching", "shaders/MS_rayMarching.vert", "shaders/MS_rayMarching.frag", "");
		ResourceManager::getSelf()->loadShader("MS_lobeSampling", "shaders/MS_lobeSampling.vert", "shaders/MS_lobeSampling.frag", "");
		ResourceManager::getSelf()->loadShader("MS_monteCarlo", "shaders/MS_monteCarlo.vert", "shaders/MS_monteCarlo.frag", "");
		ResourceManager::getSelf()->loadShader("postProcessing", "shaders/postProcessing.vert", "shaders/postProcessing.frag", "");

		//ResourceManager::getSelf()->loadVDBasTexture3D("cloud", "vdb/wdas_cloud_sixteenth.vdb"); //512
		//ResourceManager::getSelf()->getTexture3D("cloud")->loadToOpenGL();
		//ResourceManager::getSelf()->loadVDBasTexture3D("dragonHavard", "vdb/dragonHavard.vdb"); //512
		//ResourceManager::getSelf()->getTexture3D("dragonHavard")->loadToOpenGL();
		//ResourceManager::getSelf()->loadVDBasTexture3D("fireball", "vdb/fireball.vdb"); //512
		//ResourceManager::getSelf()->getTexture3D("fireball")->loadToOpenGL();
		//ResourceManager::getSelf()->loadVDBasTexture3D("explosion", "vdb/explosion.vdb"); //512
		//ResourceManager::getSelf()->getTexture3D("explosion")->loadToOpenGL();
		//ResourceManager::getSelf()->loadVDBasTexture3D("bunny_cloud", "vdb/bunny_cloud.vdb"); //512
		//ResourceManager::getSelf()->getTexture3D("bunny_cloud")->loadToOpenGL();

		//ResourceManager::getSelf()->loadVDBasTexture3D("cloudlowres", "vdb/wdas_cloud_eighth.vdb"); //512
		//ResourceManager::getSelf()->loadVDBasTexture3D("smoke", "vdb/colored_smoke.vdb");
		//ResourceManager::getSelf()->loadVDBasTexture3D("fireball", "vdb/fireball.vdb");
		//ResourceManager::getSelf()->loadVDBasTexture3D("bunny", "vdb/bunny_cloud.vdb");
		//ResourceManager::getSelf()->loadVDBasTexture3D("explosion", "vdb/explosion.vdb");

		//ResourceManager::getSelf()->loadModel("hz", "models/xps/Aloy V2/", "plz.obj");
		//ResourceManager::getSelf()->loadModel("xyzaxis", "models/xyzaxis/", "arrows.obj");
		//ResourceManager::getSelf()->loadModel("sponza", "models/sponza/", "sponza.obj"); //TODO: memory is going TO 15GB... leaf somewhere
	}

	if (settings->renderMode == REALTIME_RENDERING_MODE)
		engine->mainLoop();
	else if (settings->renderMode == OFFLINE_RENDERING_MODE)
		offEngine->coreLoop();

	return 0;
}