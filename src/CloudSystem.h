#pragma once
#include "FastNoise.h"
#include <math.h>
#include <glm/glm.hpp>
#include "Texture3D.h"
#include "Texture2D.h"
#include "PerlinNoise.h"
#include "WorleyNoise.h"
#include <iostream>

class CloudSystem
{
public:
	CloudSystem();
	~CloudSystem();

	Texture3D perlinWorley3;
	Texture3D worley3;
	Texture3D bunny;
	Texture2D curl3;
	const float perlinRange = sqrt(3.0 / 4.0);

	float changeRange(float x, float xmin, float xmax, float a, float b) {
		return (b - a) * (x - xmin) / (xmax - xmin) + a;
	}

	float rescale(float x, glm::vec2 range){
		float a = range.x, b = range.y;
		return (x - a) / (b - a);
	}

	float generatePerlin(int x, int y, int z, int octaves, float frequency, float amplitude) {
		const siv::PerlinNoise perlin(3444);

		float f = 0;
		glm::vec3 st(x, y, z);
		st *= frequency;
		float ampl = amplitude;

		for (int i = 0; i < octaves; i++) {
			float sample = perlin.octaveNoise0_1(st.x, st.y, st.z, 1);

			f += ampl * sample;

			st *= 2;
			ampl *= 0.5;
		}

		return f;
	}

	float generateWorley(int x, int y, int z, int octaves, float frequency, float amplitude){
		float f = 0;
		glm::vec3 st(x, y, z);
		st *= frequency;
		float ampl = amplitude;

		for (int i = 0; i < octaves; i++) {
			wnoise::WorleyNoise wnoise;
			glm::vec2 w = wnoise.worley(1,st);

			f += ampl * rescale(glm::length(w) / (w.y + w.x) - w.x, glm::vec2(0.0, 1.4));

			st *= 2;
			ampl *= 0.5;
		}

		return f;
	}

	void generateCloudNoiseTextures() {
		/*FastNoise *noise = new FastNoise();
		noise->SetCellularDistanceFunction(noise->Euclidean);
		noise->SetCellularReturnType(noise->Distance2Add);

		//Generates a 3D RGBA texture with R: perlin-worley GBA: worley at increasing frequencies 
		int resolution = 64;
		int resolutionZ = 64;
		float centerPos = resolution/2.0f;
		int channels = 4;

		float *data = new float[resolution * resolution * resolution * channels];
		for (int z = 0; z < resolutionZ; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolutionZ* channels);

					data[pos] = generatePerlin(x, y, z, 7, 0.04, 0.9);
					data[pos + 1] = generateWorley(x,y,z, 3, 0.065, 1);
					data[pos] = changeRange(data[pos], data[pos+1], 1.3f, 0, 1.0f);
					data[pos + 2] = generateWorley(x, y, z, 3, 0.10, 1);
					data[pos + 3] = generateWorley(x, y, z, 3, 0.12, 1);
				}
			}
		}
		perlinWorley3.generateWithData(resolution, resolution, resolutionZ, 4, data);

		//Generates a 3D RGB texture with RGB: worley at increasing frequencies
		resolution = 64;
		channels = 3;
		data = new float[resolution * resolution * resolution * channels];

		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);

					//data[pos] =     generateWorley(noise, 0.01, 1, 1, x, y, z);
					//data[pos + 1] = generateWorley(noise, 0.06, 1, 1, x, y, z);
				}
			}
		}
		worley3.generateWithData(resolution, resolution, resolution, 3, data);

		//Generates a 2D RGB texture with RGB: curl at increasing frequencies
		resolution = 128;
		channels = 3;
		data = new float[resolution * resolution * channels];

		for (int x = 0; x < resolution; x++) {
			for (int y = 0; y < resolution; y++) {
				int pos = (x + y * resolution) * channels;

				//data[pos] = generateCurl(noise, 0.01, 1, 1, x, y);
				//data[pos + 1] = generateCurl(noise, 0.08, 1, 1, x, y);
				//data[pos + 2] = generateCurl(noise, 0.1, 1, 1, x, y);
			}
		}

		curl3.generateWithData(resolution, resolution, 3, data);*/
	}
};

