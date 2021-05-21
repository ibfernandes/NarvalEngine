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

	void TestPlayground::oidnTest() {
		int width, height, channels;
		unsigned char* data = stbi_load((RESOURCES_DIR + std::string("imgs/cloud.png")).c_str(), &width, &height, &channels, STBI_rgb);
		float* fdata = new float[width * height * channels];
		float* outdata = new float[width * height * channels];

		for (int i = 0; i < width * height * channels; i++) {
			fdata[i] = ((int)data[i]) / 255.0f;
		}

		// Create an Intel Open Image Denoise device
		oidn::DeviceRef device = oidn::newDevice();
		device.commit();

		// Create a denoising filter
		oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
		filter.setImage("color", fdata, oidn::Format::Float3, width, height);
		filter.setImage("output", outdata, oidn::Format::Float3, width, height);
		filter.set("hdr", true); // image is HDR
		filter.commit();

		// Filter the image
		filter.execute();

		// Check for errors
		const char* errorMessage;
		if (device.getError(errorMessage) != oidn::Error::None)
			std::cout << "Error: " << errorMessage << std::endl;

		std::string outputPath = RESOURCES_DIR + std::string("imgs/test.exr");
		SaveEXR(outdata, width, height, channels , 0, outputPath.c_str(), &errorMessage);

	}

	TestPlayground::~TestPlayground() {
	}
}
