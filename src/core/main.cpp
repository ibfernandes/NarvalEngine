#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_ALIGNED_GENTYPES
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SIMD_AVX2
#define TINYEXR_IMPLEMENTATION
#define NOMINMAX
#include "core/Engine3D.h"


using namespace narvalengine;

int main() {
	Settings settings{};
	settings.api = RendererAPIName::OPENGL;
	settings.apiVersionMajor = 4;
	settings.apiVersionMinor = 3;
	settings.resolution = glm::vec2(1280, 720);

	Engine3D::getSelf()->init(settings, new SceneEditor());
	Engine3D::getSelf()->mainLoop();
	return 0;
}