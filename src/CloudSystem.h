#pragma once
#include "FastNoise.h"
#include <math.h>
#include <glm/glm.hpp>
#include "Texture3D.h"
class CloudSystem
{
public:
	CloudSystem();
	~CloudSystem();

	Texture3D perlinWorley3;
	Texture3D worley3;
	const float perlinRange = sqrt(3.0 / 4.0);

	float changeRange(float xmin, float xmax, float x, float a, float b) {
		return (b - a) * (x - xmin) / (xmax - xmin) + a;
	}

	float generateWorley(FastNoise *fn, float frequency, float amplitude, int octaves, int x, int y, int z) {
		float wnoise = 0;
		glm::vec3 st(x, y, z);

		fn->SetFrequency(frequency);
		fn->SetCellularDistanceFunction(fn->Euclidean);
		fn->SetCellularReturnType(fn->Distance2Add);

		// fractal Brownian motion (FBM) for worley(cellular) noise
		for (int i = 0; i < octaves; i++) {
			wnoise += amplitude * (1 - fn->GetCellular(st.x * 2, st.y * 2, st.z * 2));
			st *= 1.12;
			amplitude *= .45;
		}

		wnoise = changeRange(-1.26376, 0.966978, wnoise, 0, 1);
		return wnoise;
	}

	float generatePerlinWorley(FastNoise *fn, int x, int y, int z) {
		float fnoise;

		int octaves = 7;
		float pnoise = 0;
		float amplitude = .9;
		glm::vec3 st(x, y, z);

		// fractal Brownian motion (FBM) for perlin noise
		for (int i = 0; i < octaves; i++) {
			pnoise += amplitude * fn->GetPerlin(st.x, st.y, st.z);
			st *= 2.;
			amplitude *= .5;
		}

		pnoise = abs(pnoise);
		pnoise = changeRange(0, perlinRange, pnoise, 0, 1);

		float wnoise = generateWorley(fn, 0.02, 0.5, 3, x, y, z);

		fnoise = changeRange(0, 1, pnoise, wnoise, 1);

		return fnoise;
	}

	void generateCloudNoiseTextures() {
		int resolution = 64;
		int const CHANNELS = 4;
		float *data = new float[resolution * resolution * resolution * CHANNELS];
		FastNoise *noise = new FastNoise();
		noise->SetCellularDistanceFunction(noise->Euclidean);
		noise->SetCellularReturnType(noise->Distance2Add);

		//Generates a 3D RGBA texture with R: perlin-worley GBA: worley at increasing frequencies 
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*CHANNELS + (z * resolution * resolution* CHANNELS);

					data[pos] = generatePerlinWorley(noise, x, y, z);
					data[pos + 1] = generateWorley(noise, 0.1, 1, 1, x, y, z);
					data[pos + 2] = generateWorley(noise, 0.13, 1, 1, x, y, z);
					data[pos + 3] = generateWorley(noise, 0.16, 1, 1, x, y, z);
				}
			}
		}
		perlinWorley3.generateWithData(resolution, resolution, resolution, data);


		resolution = 32;
		data = new float[resolution * resolution * resolution * CHANNELS];

		//Generates a 3D RGBA texture with RGB: worley at increasing frequencies A: nothing
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*CHANNELS + (z * resolution * resolution* CHANNELS);

					data[pos] = generateWorley(noise, 0.01, 1, 1, x, y, z);
					data[pos + 1] = generateWorley(noise, 0.06, 1, 1, x, y, z);
					data[pos + 2] = generateWorley(noise, 0.09, 1, 1, x, y, z);
					data[pos + 3] = 0;
				}
			}
		}
		worley3.generateWithData(resolution, resolution, resolution, data);
	}
};

