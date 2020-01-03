#pragma once
#include "SphericalHarmonics.h"
#include <glm/glm.hpp>
#include <stdlib.h>
#include <time.h> 
#include <iostream>
#define PI 3.14159265358979323846264338327950288


glm::vec3 convertSphericalCoordToCartesian(float theta, float phi) {
	glm::vec3 cartCoord;
	cartCoord.x = glm::sin(theta) * glm::cos(phi);
	cartCoord.y = glm::sin(theta) * glm::sin(phi);
	cartCoord.z = glm::cos(theta);

	return cartCoord;
}

glm::vec2 convertCartesianCoordToSpherical(float x, float y) {
	glm::vec2 sphericalCoord;
	sphericalCoord.x = 2 * glm::acos(sqrt(1 - x));
	sphericalCoord.y = 2 * PI * y;

	return sphericalCoord;
}

float random() {
	return (float)(rand() % 1000) / 1000.0f;
}

/*
	Evaluate an Associated Legendre Polynomial P(l,m,x) at x
*/
float P(int l, int m, float x) {
	float pmm = 1.0f;
	if (m > 0) {
		float somx2 = sqrt((1.0f - x) * (1.0f + x));
		float fact = 1.0f;
		for (int i = 1; i <= m; i++) {
			pmm *= (-fact) * somx2;
			fact += 2.0f;
		}
	}
	if (l == m) return pmm;
	float pmmp1 = x * (2.0f * m + 1.0f) * pmm;
	if (l == m + 1) return pmmp1;
	float pll = 0.0f;
	for (int ll = m + 2; ll <= l; ++ll) {
		pll = ((2.0f * ll - 1.0f) * x * pmmp1 - (ll + m - 1.0f) * pmm) / (ll - m);
		pmm = pmmp1;
		pmmp1 = pll;
	}

	return pll;
}

int factorial(int n) {
	if (n == 0 || n == 1)
		return 1;
	return 	n * factorial(n - 1);
}

/*
	Renormalisation constant for SH function
*/
float K(int l, int m) {
	float temp = ((2.0f * l + 1.0f) * factorial(l - m)) / (4.0f * PI * factorial(l + m));
	return sqrt(temp);
}

/*
	Y_{m}^{l} (\theta, \phi)
	return a point sample of a Spherical Harmonic basis function
	l is the band, range [0..N]
	m in the range [-l..l]
	theta in the range [0..Pi]
	phi in the range [0..2*Pi]
*/

float SH(int l, int m, float theta, float phi) {
	const float sqrt2 = sqrt(2.0);
	if (m == 0) return K(l, 0) * P(l, m, cos(theta));
	else if (m > 0) return sqrt2 * K(l, m) * cos(m * phi) * P(l, m, cos(theta));
	else return sqrt2 * K(l, -m) * sin(-m * phi) * P(l, -m, cos(theta));
}

void generateSamples(Sampler *sampler, int n) {
	Sample *samples = new Sample[n * n];
	sampler->samples = samples;
	sampler->numberOfSamples = n * n;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			float a = ((float)j + random()) / (float)n;
			float b = ((float)i + random()) / (float)n;
			glm::vec2 spherical = convertCartesianCoordToSpherical(a, b);
			glm::vec3 cartesian = convertSphericalCoordToCartesian(spherical.x, spherical.y);
			int k = i * n + j;
			sampler->samples[k].sphericalCoord = spherical;
			sampler->samples[k].cartesianCoord = cartesian;
			sampler->samples[k].SHcoefficients = NULL;
		}
	}
}

void precomputeSHCoefficients(Sampler *sampler, int lBands) {
	for (int i = 0; i < sampler->numberOfSamples; i++) {
		float *SHcoefficients = new float[lBands * lBands];
		sampler->samples[i].SHcoefficients = SHcoefficients;
		glm::vec2 sphericalCoord = sampler->samples[i].sphericalCoord;
		for (int l = 0; l < lBands; l++) {
			for (int m = -l; m <= l; m++) {
				int j = l * (l + 1) + m;
				SHcoefficients[j] = SH(l, m, sphericalCoord.x, sphericalCoord.y);
			}
		}
	}
}

float lightL(int arrPos, float *visibility) {
	/*int x = pos.x * 128;
	int y = pos.y * 128;
	int z = pos.z * 128;
	int arrPos = (x + y * 128)*4 + (z * 128 * 128* 4);
	std::cout << x << " "<< y << " " << z << std::endl;*/

	return visibility[arrPos];
}

glm::vec3 convertTo3D(int idx, int lod) {
	int xMax = 128 * 3 * 3/lod;
	int yMax = 128/lod;
	int zMax = 128/lod;

	int z = idx / (xMax * yMax);
	idx -= (z * xMax * yMax);
	int y = idx / xMax;
	int x = idx % xMax;
	return glm::vec3( x/ xMax, y / yMax, z / zMax );
}

//monochromatic
float evaluateLight(int pos, glm::vec3 light, int lod) {
	glm::vec3 cubePos = convertTo3D(pos, lod);
	glm::vec3 L = light - cubePos;
	return 1.0f / (1.2f + glm::dot(L, L));
}

float lightProbe(glm::vec2 spherical) {
	return glm::max(0.0f, 5.0f * glm::cos(spherical.x) - 4.0f) + glm::max(0.0f, -4.0f * glm::sin(spherical.x - (float)PI) * glm::cos(spherical.y - 2.5f) - 3.0f);
}

void projectLightFunction(float *coeffs, Sampler *sampler, int bands, int gridSize, float *visibility, int lod) {

	int bandSquared = bands * bands;
	int gridSizeWithBands = gridSize * gridSize * gridSize * bandSquared/lod;
	float weight = 4.0f * PI;
	float scale = weight / sampler->numberOfSamples;
	//scale /= 1000;

	for (int x = 0; x < gridSizeWithBands; x++)
		coeffs[x] = 0;

	std::cout << "projectLightFunction starting..." << std::endl;

	for (int x = 0; x < gridSizeWithBands; x += bandSquared) {
		if (gridSizeWithBands / 4 == x)
			std::cout << "25%..." << std::endl;
		if (gridSizeWithBands / 2 == x)
			std::cout << "50%..." << std::endl;
		if (gridSizeWithBands * 0.75f == x)
			std::cout << "75%..." << std::endl;

		for (int i = 0; i < sampler->numberOfSamples; i++) {
			float light = evaluateLight(x, glm::vec3(0,11.0f,2.0f), lod);

			for (int j = 0; j < bands * bands; j++) { 
				float coeff = sampler->samples[i].SHcoefficients[j];
				coeffs[x + j] += light * coeff;
				//coeffs[x + j] += lightL(x/bandSquared, visibility) * light * coeff;
				//coeffs[x + j] += lightProbe(sampler->samples[i].sphericalCoord) * coeff;
			}
		}
	}

	std::cout << "projectLightFunction finished..." << std::endl;

	for (int x = 0; x < gridSizeWithBands; x++)
		coeffs[x] *= scale;
}

