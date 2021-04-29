#include "TestPlayground.h"
#include "tinyexr.h"

namespace narvalengine {
	void TestPlayground::testEXR() {
		std::string path = RESOURCES_DIR + std::string("scenes/pbrt/cornellbox.exr");
		const char* input = path.c_str();
		float* out; // width * height * RGBA
		int width;
		int height;
		const char* err = NULL; // or nullptr in C++11

		int ret = LoadEXR(&out, &width, &height, input, &err);

		if (ret != TINYEXR_SUCCESS) {
			if (err) {
				fprintf(stderr, "ERR : %s\n", err);
				FreeEXRErrorMessage(err); // release memory of error message.
			}
		}
		else {
			free(out); // release memory of image data
		}
	}

	TestPlayground::~TestPlayground() {
	}
}
