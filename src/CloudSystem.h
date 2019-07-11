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

	float changeRange(float x, float xmin, float xmax, float a, float b) {
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
		noise = changeRange(noise , -1.0, 1, 0, 1);

		return noise;
	}

	glm::vec3 permute(glm::vec3 x) {
		return glm::mod((34.0f * x + 1.0f) * x, 289.0f);
	}

	glm::vec3 dist(glm::vec3 x, glm::vec3 y, glm::vec3 z, bool manhattanDistance) {
		return manhattanDistance ? abs(x) + abs(y) + abs(z) : (x * x + y * y + z * z);
	}

	glm::vec2 worley(glm::vec3 P, float jitter, bool manhattanDistance) {
		float K = 0.142857142857; // 1/7
		float Ko = 0.428571428571; // 1/2-K/2
		float  K2 = 0.020408163265306; // 1/(7*7)
		float Kz = 0.166666666667; // 1/6
		float Kzo = 0.416666666667; // 1/2-1/6*2

		glm::vec3 Pi = glm::mod(floor(P), 289.0f);
		glm::vec3 Pf = glm::fract(P) - 0.5f;

		glm::vec3 Pfx = Pf.x + glm::vec3(1.0, 0.0, -1.0);
		glm::vec3 Pfy = Pf.y + glm::vec3(1.0, 0.0, -1.0);
		glm::vec3 Pfz = Pf.z + glm::vec3(1.0, 0.0, -1.0);

		glm::vec3 p = permute(Pi.x + glm::vec3(-1.0, 0.0, 1.0));
		glm::vec3 p1 = permute(p + Pi.y - 1.0f);
		glm::vec3 p2 = permute(p + Pi.y);
		glm::vec3 p3 = permute(p + Pi.y + 1.0f);

		glm::vec3 p11 = permute(p1 + Pi.z - 1.0f);
		glm::vec3 p12 = permute(p1 + Pi.z);
		glm::vec3 p13 = permute(p1 + Pi.z + 1.0f);

		glm::vec3 p21 = permute(p2 + Pi.z - 1.0f);
		glm::vec3 p22 = permute(p2 + Pi.z);
		glm::vec3 p23 = permute(p2 + Pi.z + 1.0f);

		glm::vec3 p31 = permute(p3 + Pi.z - 1.0f);
		glm::vec3 p32 = permute(p3 + Pi.z);
		glm::vec3 p33 = permute(p3 + Pi.z + 1.0f);

		glm::vec3 ox11 = fract(p11*K) - Ko;
		glm::vec3 oy11 = glm::mod(floor(p11*K), 7.0f)*K - Ko;
		glm::vec3 oz11 = floor(p11*K2)*Kz - Kzo; // p11 < 289 guaranteed

		glm::vec3 ox12 = fract(p12*K) - Ko;
		glm::vec3 oy12 = glm::mod(floor(p12*K), 7.0f)*K - Ko;
		glm::vec3 oz12 = floor(p12*K2)*Kz - Kzo;

		glm::vec3 ox13 = fract(p13*K) - Ko;
		glm::vec3 oy13 = glm::mod(floor(p13*K), 7.0f)*K - Ko;
		glm::vec3 oz13 = floor(p13*K2)*Kz - Kzo;

		glm::vec3 ox21 = fract(p21*K) - Ko;
		glm::vec3 oy21 = glm::mod(floor(p21*K), 7.0f)*K - Ko;
		glm::vec3 oz21 = floor(p21*K2)*Kz - Kzo;

		glm::vec3 ox22 = fract(p22*K) - Ko;
		glm::vec3 oy22 = glm::mod(floor(p22*K), 7.0f)*K - Ko;
		glm::vec3 oz22 = floor(p22*K2)*Kz - Kzo;

		glm::vec3 ox23 = fract(p23*K) - Ko;
		glm::vec3 oy23 = glm::mod(floor(p23*K), 7.0f)*K - Ko;
		glm::vec3 oz23 = floor(p23*K2)*Kz - Kzo;

		glm::vec3 ox31 = fract(p31*K) - Ko;
		glm::vec3 oy31 = glm::mod(floor(p31*K), 7.0f)*K - Ko;
		glm::vec3 oz31 = floor(p31*K2)*Kz - Kzo;

		glm::vec3 ox32 = fract(p32*K) - Ko;
		glm::vec3 oy32 = glm::mod(floor(p32*K), 7.0f)*K - Ko;
		glm::vec3 oz32 = floor(p32*K2)*Kz - Kzo;

		glm::vec3 ox33 = fract(p33*K) - Ko;
		glm::vec3 oy33 = glm::mod(floor(p33*K), 7.0f)*K - Ko;
		glm::vec3 oz33 = floor(p33*K2)*Kz - Kzo;

		glm::vec3 dx11 = Pfx + jitter * ox11;
		glm::vec3 dy11 = Pfy.x + jitter * oy11;
		glm::vec3 dz11 = Pfz.x + jitter * oz11;

		glm::vec3 dx12 = Pfx + jitter * ox12;
		glm::vec3 dy12 = Pfy.x + jitter * oy12;
		glm::vec3 dz12 = Pfz.y + jitter * oz12;

		glm::vec3 dx13 = Pfx + jitter * ox13;
		glm::vec3 dy13 = Pfy.x + jitter * oy13;
		glm::vec3 dz13 = Pfz.z + jitter * oz13;

		glm::vec3 dx21 = Pfx + jitter * ox21;
		glm::vec3 dy21 = Pfy.y + jitter * oy21;
		glm::vec3 dz21 = Pfz.x + jitter * oz21;

		glm::vec3 dx22 = Pfx + jitter * ox22;
		glm::vec3 dy22 = Pfy.y + jitter * oy22;
		glm::vec3 dz22 = Pfz.y + jitter * oz22;

		glm::vec3 dx23 = Pfx + jitter * ox23;
		glm::vec3 dy23 = Pfy.y + jitter * oy23;
		glm::vec3 dz23 = Pfz.z + jitter * oz23;

		glm::vec3 dx31 = Pfx + jitter * ox31;
		glm::vec3 dy31 = Pfy.z + jitter * oy31;
		glm::vec3 dz31 = Pfz.x + jitter * oz31;

		glm::vec3 dx32 = Pfx + jitter * ox32;
		glm::vec3 dy32 = Pfy.z + jitter * oy32;
		glm::vec3 dz32 = Pfz.y + jitter * oz32;

		glm::vec3 dx33 = Pfx + jitter * ox33;
		glm::vec3 dy33 = Pfy.z + jitter * oy33;
		glm::vec3 dz33 = Pfz.z + jitter * oz33;

		glm::vec3 d11 = dist(dx11, dy11, dz11, manhattanDistance);
		glm::vec3 d12 = dist(dx12, dy12, dz12, manhattanDistance);
		glm::vec3 d13 = dist(dx13, dy13, dz13, manhattanDistance);
		glm::vec3 d21 = dist(dx21, dy21, dz21, manhattanDistance);
		glm::vec3 d22 = dist(dx22, dy22, dz22, manhattanDistance);
		glm::vec3 d23 = dist(dx23, dy23, dz23, manhattanDistance);
		glm::vec3 d31 = dist(dx31, dy31, dz31, manhattanDistance);
		glm::vec3 d32 = dist(dx32, dy32, dz32, manhattanDistance);
		glm::vec3 d33 = dist(dx33, dy33, dz33, manhattanDistance);

		glm::vec3 d1a = min(d11, d12);
		d12 = max(d11, d12);
		d11 = min(d1a, d13); // Smallest now not in d12 or d13
		d13 = max(d1a, d13);
		d12 = min(d12, d13); // 2nd smallest now not in d13
		glm::vec3 d2a = min(d21, d22);
		d22 = max(d21, d22);
		d21 = min(d2a, d23); // Smallest now not in d22 or d23
		d23 = max(d2a, d23);
		d22 = min(d22, d23); // 2nd smallest now not in d23
		glm::vec3 d3a = min(d31, d32);
		d32 = max(d31, d32);
		d31 = min(d3a, d33); // Smallest now not in d32 or d33
		d33 = max(d3a, d33);
		d32 = min(d32, d33); // 2nd smallest now not in d33
		glm::vec3 da = min(d11, d21);
		d21 = max(d11, d21);
		d11 = min(da, d31); // Smallest now in d11
		d31 = max(da, d31); // 2nd smallest now not in d31
		if (d11.x < d11.y) {
			
		}else {
			float buffer = d11.x;
			d11.x = d11.y;
			d11.y = buffer;
		}
		if (d11.x < d11.z) {

		}
		else {
			float buffer = d11.x;
			d11.x = d11.z;
			d11.z = buffer;
		}
		d12 = min(d12, d21); // 2nd smallest now not in d21
		d12 = min(d12, d22); // nor in d22
		d12 = min(d12, d31); // nor in d31
		d12 = min(d12, d32); // nor in d32
		glm::vec2 m = glm::min( glm::vec2(d11.y, d11.z), glm::vec2(d12.x, d12.y)); // nor in d12.yz
		d11 = glm::vec3(d11.x, m.x, m.y);
		d11.y = glm::min(d11.y, d12.z); // Only two more to go
		d11.y = glm::min(d11.y, d11.z); // Done! (Phew!)
		return glm::vec2(sqrt(d11.x), sqrt(d11.y)); // F1, F2

	}

	float rescale(float x, glm::vec2 range)
	{
		float a = range.x, b = range.y;
		return (x - a) / (b - a);
	}

	float generateWorley(float frequency, float amplitude, int octaves, int x, int y, int z) {
		float f = 0;
		glm::vec3 st(x, y, z);
		st *= frequency;
		float ampl = amplitude;

		for (int i = 0; i < octaves; i++) {
			glm::vec2 w = worley(st, 1.0, false);

			f += ampl * rescale(glm::length(w) / (w.y + w.x) - w.x, glm::vec2(0.0, 1.0));

			st *= 2;
			ampl *= 0.5;
		}

		return f;
	}

	float generateWorley(FastNoise *fn, float frequency, float amplitude, int octaves, int x, int y, int z) {
		float wnoise = 0;
		glm::vec3 st(x, y, z);

		fn->SetFrequency(frequency);
		fn->SetCellularDistanceFunction(fn->Euclidean);
		fn->SetCellularReturnType(fn->Distance2Add);

		// fractal Brownian motion (FBM) for worley(cellular) noise
		for (int i = 0; i < octaves; i++) {
			wnoise += amplitude * (fn->GetCellular(st.x, st.y, st.z));
			st *= 2;
			amplitude *= .55;
		}

		//wnoise = glm::clamp(1 - wnoise, 0.0f, 1.0f);
		//wnoise = changeRange(-1.26376, 0.966978, wnoise, 0, 1);

		return wnoise;
	}

	float generatePerlinWorley(FastNoise *fn, int x, int y, int z) {
		float fnoise;

		int octaves = 7;
		float pnoise = 0;
		float amplitude = 1;
		glm::vec3 st(x, y, z);
		fn->SetFrequency(0.03);

		// fractal Brownian motion (FBM) for perlin noise
		for (int i = 0; i < octaves; i++) {
			pnoise += amplitude * (float)fn->GetPerlin(st.x, st.y, st.z);
			st *= 2;
			amplitude *= .5;
		}

		//pnoise = abs(pnoise);
		//pnoise = glm::clamp(pnoise, 0.0f, 1.0f);
		pnoise = changeRange(pnoise, -perlinRange*0.8, perlinRange*0.4, 0, 1);

		//float wnoise = generateWorley(fn, 0.025, 1, 1, x, y, z);

		//fnoise = changeRange(pnoise, 0, 1, wnoise, 1);

		return pnoise;
	}

	void generateCloudNoiseTextures() {
		FastNoise *noise = new FastNoise();
		noise->SetCellularDistanceFunction(noise->Euclidean);
		noise->SetCellularReturnType(noise->Distance2Add);

		//Generates a 3D RGBA texture with R: perlin-worley GBA: worley at increasing frequencies 
		int resolution = 64;
		float centerPos = resolution/2.0f;
		int channels = 4;
		float min[4] = { 0 };
		float max[4] = { 0 };

		float *data = new float[resolution * resolution * resolution * channels];
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);
					float dist = pow((x - centerPos), 2) + pow((y - centerPos), 2) + pow((z - centerPos), 2);
					dist = sqrt(dist)/55.9f;

					data[pos] = generatePerlinWorley(noise, x, y, z);
					data[pos + 1] = generateWorley(noise, 0.025, 1, 3, x, y, z);
					data[pos + 2] = generateWorley(noise, 0.09, 1, 2, x, y, z);
					data[pos + 3] = generateWorley(noise, 0.15, 1, 2, x, y, z);

					if (data[pos + 1] < min[0]) {
						min[0] = data[pos + 1];
					}
					if (data[pos + 1] > max[0]) {
						max[0] = data[pos + 1];
					}

					if (data[pos + 2] < min[1]) {
						min[1] = data[pos + 2];
					}
					if (data[pos + 2] > max[1]) {
						max[1] = data[pos + 2];
					}

					if (data[pos + 3] < min[2]) {
						min[2] = data[pos + 3];
					}
					if (data[pos + 3] > max[2]) {
						max[2] = data[pos + 3];
					}

					if (data[pos] < min[3]) {
						min[3] = data[pos];
					}
					if (data[pos] > max[3]) {
						max[3] = data[pos];
					}
				}
			}
		}
		
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);

					data[pos + 1] = 1 - changeRange(data[pos + 1], min[0], max[0], 0 , 1);
					data[pos + 2] = 1 - changeRange(data[pos + 2], min[1], max[1], 0, 1);
					data[pos + 3] = 1 - changeRange(data[pos + 3], min[2], max[2], 0, 1);
					//data[pos] = generateWorley(1, 1, 1, x, y, z);
					//data[pos] = abs(changeRange(data[pos], min[3], max[3], -1, 1));
					data[pos] = changeRange(data[pos], data[pos + 1], 1, 0, 1);
				}
			}
		}
		perlinWorley3.generateWithData(resolution, resolution, resolution, 4, data);



		//Generates a 3D RGB texture with RGB: worley at increasing frequencies
		min[0] = 0;
		min[1] = 0;
		min[2] = 0;
		min[3] = 0;
		max[0] = 0;
		max[1] = 0;
		max[2] = 0;
		max[3] = 0;

		resolution = 64;
		channels = 3;
		data = new float[resolution * resolution * resolution * channels];
		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);

					data[pos] =     generateWorley(noise, 0.01, 1, 1, x, y, z);
					data[pos + 1] = generateWorley(noise, 0.06, 1, 1, x, y, z);
					data[pos + 2] = generateWorley(noise, 0.09, 1, 1, x, y, z);

					if (data[pos] < min[0]) {
						min[0] = data[pos];
					}
					if (data[pos] > max[0]) {
						max[0] = data[pos];
					}

					if (data[pos + 1] < min[0]) {
						min[0] = data[pos + 1];
					}
					if (data[pos + 1] > max[0]) {
						max[0] = data[pos + 1];
					}

					if (data[pos + 2] < min[1]) {
						min[1] = data[pos + 2];
					}
					if (data[pos + 2] > max[1]) {
						max[1] = data[pos + 2];
					}
				}
			}
		}

		for (int z = 0; z < resolution; z++) {
			for (int x = 0; x < resolution; x++) {
				for (int y = 0; y < resolution; y++) {
					int pos = (x + y * resolution)*channels + (z * resolution * resolution* channels);

					data[pos]     = 1 - changeRange(data[pos], min[0], max[0], 0, 1);
					data[pos + 1] = 1 - changeRange(data[pos + 1], min[1], max[1], 0, 1);
					data[pos + 2] = 1 - changeRange(data[pos + 2], min[2], max[2], 0, 1);
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

		curl3.generateWithData(resolution, resolution, 3, data);
	}
};

