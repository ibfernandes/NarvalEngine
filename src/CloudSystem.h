#pragma once
#include "FastNoise.h"
#include <math.h>
#include <glm/glm.hpp>
#include "Texture3D.h"
#include "Texture2D.h"
#include <iostream>

class CloudSystem
{
public:
	CloudSystem();
	~CloudSystem();

	Texture3D perlinWorley3;
	Texture3D worley3;
	Texture2D curl3;
	const float perlinRange = sqrt(3.0 / 4.0);

	float changeRange(float xmin, float xmax, float x, float a, float b) {
		return (b - a) * (x - xmin) / (xmax - xmin) + a;
	}

	glm::vec3 snoiseVec3(glm::vec3 v, FastNoise *fn) {
		float s = fn->GetSimplex(v.x, v.y, v.z);
		float s1 = fn->GetSimplex(v.y - 19.1, v.z + 33.4, v.x + 47.2);
		float s2 = fn->GetSimplex(v.z + 74.2, v.x - 124.5, v.y + 99.4);
		glm::vec3 c = glm::vec3(s, s1, s2);
		
		return c;
	}

	//TODO: not so close to what i expected...
	//Adapted from: https://github.com/cabbibo/glsl-curl-noise/blob/master/curl.glsl
	glm::vec3 curlNoise(glm::vec3 p, FastNoise *fn) {
		const float e = .1;
		glm::vec3 dx = glm::vec3(e, 0.0, 0.0);
		glm::vec3 dy = glm::vec3(0.0, e, 0.0);
		glm::vec3 dz = glm::vec3(0.0, 0.0, e);

		 glm::vec3 p_x0 = snoiseVec3(p - dx, fn);
		 glm::vec3 p_x1 = snoiseVec3(p + dx, fn);
		 glm::vec3 p_y0 = snoiseVec3(p - dy, fn);
		 glm::vec3 p_y1 = snoiseVec3(p + dy, fn);
		 glm::vec3 p_z0 = snoiseVec3(p - dz, fn);
		 glm::vec3 p_z1 = snoiseVec3(p + dz, fn);

		float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
		float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
		float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

		const float divisor = 1.0 / (2.0 * e);
		
		return normalize(glm::vec3(x, y, z) * divisor);
	}

	float generateCurl(FastNoise *fn, float frequency, float amplitude, int octaves, int x, int y) {
		fn->SetFrequency(frequency);

		float noise = curlNoise(glm::vec3(x, y, 1), fn).x;
		noise = changeRange(-1.0, 1, noise, 0, 1);

		return noise;
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
		FastNoise *noise = new FastNoise();
		noise->SetCellularDistanceFunction(noise->Euclidean);
		noise->SetCellularReturnType(noise->Distance2Add);

		//Generates a 3D RGBA texture with R: perlin-worley GBA: worley at increasing frequencies 
		int resolution = 64;
		int channels = 4;
		float *data = new float[resolution * resolution * resolution * channels];
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);

					data[pos] = generatePerlinWorley(noise, x, y, z);
					data[pos + 1] = generateWorley(noise, 0.1, 1, 1, x, y, z);
					data[pos + 2] = generateWorley(noise, 0.13, 1, 1, x, y, z);
					data[pos + 3] = generateWorley(noise, 0.16, 1, 1, x, y, z);
				}
			}
		}
		perlinWorley3.generateWithData(resolution, resolution, resolution, 4, data);

		//Generates a 3D RGB texture with RGB: worley at increasing frequencies
		resolution = 32;
		channels = 3;
		data = new float[resolution * resolution * resolution * channels];
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);

					data[pos] = generateWorley(noise, 0.01, 1, 1, x, y, z);
					data[pos + 1] = generateWorley(noise, 0.06, 1, 1, x, y, z);
					data[pos + 2] = generateWorley(noise, 0.09, 1, 1, x, y, z);
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

				data[pos] = generateCurl(noise, 0.14, 1, 1, x, y);
				data[pos + 1] = generateCurl(noise, 0.17, 1, 1, x, y);
				data[pos + 2] = generateCurl(noise, 0.19, 1, 1, x, y);
			}
		}

		curl3.generateWithData(resolution, resolution, 3, data);
	}
};

