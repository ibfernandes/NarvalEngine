#pragma once
#include <glm/glm.hpp>
#include <stdlib.h>
#include <time.h> 
#define PI 3.14159265358979323846264338327950288

struct Sample {
	glm::vec3 cartesianCoord;
	glm::vec2 sphericalCoord;
	float *SHcoefficients;
};

struct Sampler {
	Sample *samples;
	int numberOfSamples;
};

glm::vec3 convertSphericalCoordToCartesian(float theta, float phi);

glm::vec2 convertCartesianCoordToSpherical(float x, float y);

float random();

/*
	Evaluate an Associated Legendre Polynomial P(l,m,x) at x
*/
float P(int l, int m, float x);

int factorial(int n);
/*
	Renormalisation constant for SH function
*/
float K(int l, int m);

/*
	Y_{m}^{l} (\theta, \phi)
	return a point sample of a Spherical Harmonic basis function
	l is the band, range [0..N]
	m in the range [-l..l]
	theta in the range [0..Pi]
	phi in the range [0..2*Pi]
*/

float SH(int l, int m, float theta, float phi);

void generateSamples(Sampler *sampler, int n);

float lightL(int arrPos, float *visibility);

void phaseFunctionP();

void G();

void projectLightFunction(float *coeffs, Sampler *sampler, int bands, int gridSize, float *visibility, int lod);

void precomputeSHCoefficients(Sampler *sampler, int lBands);
