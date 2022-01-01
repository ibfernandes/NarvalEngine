#include "TestPlayground.h"
#include "tinyexr.h"

namespace narvalengine {
	void TestPlayground::compareEXR(std::string img1Path, std::string img2Path, std::string writeTo) {

		float* img1;
		float* img2;
		int width;
		int height;
		const char* err = NULL; // or nullptr in C++11

		LoadEXR(&img1, &width, &height, img1Path.c_str(), &err);
		LoadEXR(&img2, &width, &height, img2Path.c_str(), &err);

		int channels = 4;
		float* result = new float[width * height * channels];
		float highestDistance = 0;
		float avg = 0;
		int activePixels = 0;

		//May diverge for RGB, since it is bounded and does not cover full cielab resolution
		//Need to confirm these values:
		//https://stackoverflow.com/questions/19099063/what-are-the-ranges-of-coordinates-in-the-cielab-color-space
		//L in[0, 100]
		//A in[-86.185, 98.254]
		//B in[-107.863, 94.482]
		glm::vec3 cieLabMax = glm::vec3(100, 127, 127);
		glm::vec3 cieLabMin= glm::vec3(0, -128, -128);
		float maxDeltaE = deltaE(&cieLabMin[0], &cieLabMax[0]);
		float trespassingJND = 0;

		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				int index = (x + y * width) * channels;
				glm::vec3 c1 = glm::vec3(img1[index], img1[index + 1], img1[index + 2]);
				glm::vec3 c2 = glm::vec3(img2[index], img2[index + 1], img2[index + 2]);
				float rgb1[3];
				float rgb2[3];
				linearToRGB(&c1[0], &rgb1[0]);
				linearToRGB(&c2[0], &rgb2[0]);
				rgbToCIE(&rgb1[0], &c1[0]);
				rgbToCIE(&rgb2[0], &c2[0]);
				float d = deltaE(&c1[0], &c2[0]);

				highestDistance = (d > highestDistance) ? d : highestDistance;

				if (c1.x > 0 || c1.y > 0 || c1.z > 0 || c2.x > 0 || c2.y > 0 || c2.z > 0){
					activePixels++;
					avg += d;
				}

				if (d > 2.3f)
					trespassingJND++;

				glm::vec3 res = computeRMSE(c1, c2);
				//result[index] = res.x;
				//result[index + 1] = res.y;
				//result[index + 2] = res.z;
				result[index] = d;
				result[index+1] = d;
				result[index+2] = d;
				result[index + 3] = 1;
			}
		}

		//the black spots of background are bringing the avg down...
		avg = avg / (activePixels);

		//normalization
		/*for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				int index = (x + y * width) * channels;

				result[index] /= maxDeltaE;
				result[index + 1] /= maxDeltaE;
				result[index + 2] /= maxDeltaE;
				result[index + 3] = 1;
			}
		}*/

		//show pixels trespassing the JND as red dots
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				int index = (x + y * width) * channels;

				if(result[index] > 2.3f)
					result[index] = 1;
				else
					result[index] = 0;
				result[index + 1] = 0;
				result[index + 2] = 0;
				result[index + 3] = 1;
			}
		}

		std::cout << "deltaE avg of the whole image: " << avg << std::endl;
		std::cout << "highest deltaE: " << highestDistance << std::endl;
		std::cout << "max deltaE in CieLab Space: " << maxDeltaE << std::endl;
		std::cout << "% trespassing JND: " << (float(trespassingJND)/activePixels)*100.0f << std::endl;
		std::cout << "Active pixels: " << activePixels << std::endl;

		MemoryBuffer mem;
		mem.data = (uint8_t*)result;
		mem.size = sizeof(float) * width * height * channels;

		saveImage(mem, width, height, TextureLayout::RGBA32F, ImageFileFormat::EXR, writeTo.c_str());
		float k = 0;
	}

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
		/*int width, height, channels;
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
		SaveEXR(outdata, width, height, channels , 0, outputPath.c_str(), &errorMessage);*/

	}

	TestPlayground::~TestPlayground() {
	}
}
