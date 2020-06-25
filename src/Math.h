#pragma once

#include "Ray.h"
#include <limits>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <random>
#define EPSILON 0.0000000001
#define PI 3.14159265358979323846264338327950288
#define TWO_PI 6.283185307179586476925286766559

inline glm::ivec3 seed(0, 0, 0);

inline float fraction(float f) {
	int v = f;
	return f - float(v);
}

inline glm::ivec3 intmod(glm::ivec3 v, int m) {
	return glm::ivec3(v.x % m, v.y % m, v.z % m);
};

/*
	Returns a random number between [0, 1)
*/
/*inline float random() {
	seed = seed + 1;
	seed = intmod(seed, 2003578967);

	return fraction(sin(glm::dot(glm::vec3(12.9898, 4.1414, 0), glm::vec3(seed))) * 43758.5453) * 0.5f + 0.5f;
}*/
/*
inline float random() {
	return rand() / (RAND_MAX + 1.0);
}
*/
inline std::random_device rd;
inline std::mt19937 mt(rd());
inline std::uniform_real_distribution<float> dist(0.0, 1.0);

inline float random() {
	return dist(mt);
}

/*
Converts 3D point to 1D index in ZYX order.
*/
inline int to1D(int width, int height, int x, int y, int z) {
	return height * width * z + width * y + x;
}

inline int to1D(int width, int height, glm::vec3 vec) {
	return to1D(width, height, vec.x, vec.y, vec.z);
}

/*
	Converts 1D index in ZYX order to 3D point.
*/
inline void to3D(int index, int width, int height, int &x, int &y, int &z) {
	z = index / (width * height);
	index -= (z * width * height);
	y = index / width;
	x = index % width;
}

inline void to3D(int index, int width, int height, glm::ivec3 &vec) {
	return to3D(index, width, height, vec.x, vec.y, vec.z);
}

inline float saturate(float x) {
	return glm::clamp(x, 0.0f, 1.0f);
}


/*
	Receives an integer with a max value of 10 bits (1024) and interleaves it by 3.
*/
inline int interleaveBits(int x) {
	if (x == (1 << 10)) --x;
	x = (x | (x << 16)) & 0b00000011000000000000000011111111;
	x = (x | (x << 8)) & 0b00000011000000001111000000001111;
	x = (x | (x << 4)) & 0b00000011000011000011000011000011;
	x = (x | (x << 2)) & 0b00001001001001001001001001001001;

	return x;
}

/*
	Encodes 3D point to morton code using 10bits for each axis in ZYX order.
*/
inline int encodeMorton3D(int x, int y, int z) {
	return (interleaveBits(z) << 2) | (interleaveBits(y) << 1) | interleaveBits(x);
}

inline int encodeMorton3D(glm::ivec3 v) {
	return (interleaveBits(v.z) << 2) | (interleaveBits(v.y) << 1) | interleaveBits(v.x);
}

/*
	Decodes morton code to 3D point. Assumes ZYX order.
*/
inline void decodeMorton3D(int value, int &x, int&y, int &z) {
	z = value >> 2;
	z = z & 0b00001001001001001001001001001001;
	z = (z | (z >> 2)) & 0b00000011000011000011000011000011;
	z = (z | (z >> 4)) & 0b00000011000000001111000000001111;
	z = (z | (z >> 8)) & 0b00000011000000000000000011111111;
	z = (z | (z >> 16)) & 0b00000000000000000000001111111111;

	y = value >> 1;
	y = y & 0b00001001001001001001001001001001;
	y = (y | (y >> 2)) & 0b00000011000011000011000011000011;
	y = (y | (y >> 4)) & 0b00000011000000001111000000001111;
	y = (y | (y >> 8)) & 0b00000011000000000000000011111111;
	y = (y | (y >> 16)) & 0b00000000000000000000001111111111;

	x = value;
	x = x & 0b00001001001001001001001001001001;
	x = (x | (x >> 2)) & 0b00000011000011000011000011000011;
	x = (x | (x >> 4)) & 0b00000011000000001111000000001111;
	x = (x | (x >> 8)) & 0b00000011000000000000000011111111;
	x = (x | (x >> 16)) & 0b00000000000000000000001111111111;
}

inline glm::ivec3 decodeMorton3D(int value) {
	glm::ivec3 res;
	decodeMorton3D(value, res.x, res.y, res.z);
	return res;
}

inline int encodeSimple3D(int x, int y, int z) {
	int res = 0;
	res = z << 20;
	res = res | y << 10;
	res = res | x;
	return res;
}

inline int encodeSimple3D(glm::ivec3 v) {
	return encodeSimple3D(v.x, v.y, v.z);
}

inline glm::ivec3 decodeSimple3D(int value) {
	glm::ivec3 res;
	//If not unsigned right shift was filling with 1's
	//TODO:remove first two bits
	value = value & 0b00111111111111111111111111111111;
	unsigned int v = value;
	res.z = v >> 20;
	res.y = (v << 12) >> 22;
	res.x = (v << 22) >> 22;
	return res;
}

inline void decodeSimple3D(int value, int &x, int &y, int &z) {
	//If not unsigned right shift was filling with 1's
	value = value & 0b00111111111111111111111111111111;
	unsigned int v = value;
	z = v >> 20;
	y = (v << 12) >> 22;
	x = (v << 22) >> 22;
}

inline void decodeSimple3D(int value, glm::ivec3 &vec) {
	//If not unsigned right shift was filling with 1's
	value = value & 0b00111111111111111111111111111111;
	unsigned int v = value;
	vec.z = v >> 20;
	vec.y = (v << 12) >> 22;
	vec.x = (v << 22) >> 22;
}

inline glm::vec2 intersectBox(glm::vec3 orig, glm::vec3 dir, glm::vec3 bmin, glm::vec3 bmax) {
	//Line's/Ray's equation
	// o + t*d = y
	// t = (y - o)/d
	//when t is negative, the box is behind the ray origin
	glm::vec3 tMinTemp = (bmin - orig) / dir;
	glm::vec3 tmaxTemp = (bmax - orig) / dir;

	glm::vec3 tMin = glm::min(tMinTemp, tmaxTemp);
	glm::vec3 tMax = glm::max(tMinTemp, tmaxTemp);

	float t0 = glm::max(tMin.x, glm::max(tMin.y, tMin.z));
	float t1 = glm::min(tMax.x, glm::min(tMax.y, tMax.z));

	//if t0 > t1: miss
	return glm::vec2(t0, t1);
}


inline glm::vec2 intersectBox(Ray r, glm::vec3 bmin, glm::vec3 bmax) {
	return intersectBox(r.origin, r.direction, bmin, bmax);
}

inline int setEmptyBit(int mortonCode) {
	return (mortonCode | 0b10000000000000000000000000000000) & 0b10000000000000000000000000000000;
}

inline bool isEmpty(int mortonCode) {
	return mortonCode & 0b10000000000000000000000000000000;
}

inline void radixSort(int *grid, int size) {
	std::vector<int> gridVec(grid, grid + size);
	std::vector<int> tempArr(size);
	const int bitsPerPass = 6;
	int nBits = 30;
	int nPasses = nBits / bitsPerPass;

	for (int i = 0; i < nPasses; i++) {
		int lowBit = i * bitsPerPass;
		std::vector<int> &toBeSorted = (i & 1) ? tempArr : gridVec;
		std::vector<int> &sortedValues = (i & 1) ? gridVec : tempArr;

		const int nBuckets = 1 << bitsPerPass;
		int bucketCount[nBuckets] = { 0 };
		int bitMask = (1 << bitsPerPass) - 1;
		for (int mc : toBeSorted) {
			int bucket = (mc >> lowBit) & bitMask;
			++bucketCount[bucket];
		}

		int outIndex[nBuckets];
		outIndex[0] = 0;
		for (int k = 1; k < nBuckets; ++k)
			outIndex[k] = outIndex[k - 1] + bucketCount[k - 1];

		for (int mc : toBeSorted) {
			int bucket = (mc >> lowBit) & bitMask;
			sortedValues[outIndex[bucket]++] = mc;
		}
	}

	if (nPasses & 1)
		std::swap(gridVec, tempArr);

	std::copy(gridVec.begin(), gridVec.end(), grid);
}

inline glm::vec3 sampleUnitSphere() {
	//Varies from		0 rad <= x <= 2PI rad
	//					0 deg <= x <= 360 deg
	float theta = TWO_PI * random();
	
	//Varies from	     -1 <= x <= 1 
	//				180 deg <= x <= 0 deg
	float phi = acos(1.0f - 2.0f * random());

	return glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
}

inline glm::vec3 sampleUnitSphere(float cosTheta, float sintTheta, float phi) {
	return glm::vec3(sin(phi) * cosTheta, sin(phi) * sintTheta, cos(phi));
}

/*
	Generates a random direction vector on the cartesian unit hemisphere
	using spherical coordinates
	where theta varies from 0 to 90 degrees
	and phi from 0 to 360 degrees
*/
inline glm::vec3 sampleUnitHemisphere() {
	float e1 = random();

	//Varies from	0 <= x <= 1
	//since sin(0 deg) = 0 and sin(90 deg) = 1, we can simply generate a random number r on [0, 1] without need
	//to caculate the actual sin()
	float sinTheta = std::sqrt(1.0f - e1 * e1);

	//Varies from     0 <= x <= 2PI
	//			  0 deg <= x <= 360 deg
	float phi = TWO_PI * random();
	return glm::vec3(sinTheta * cos(phi), sinTheta * sin(phi), e1);
}

inline glm::vec3 randomInUnitDisk() {
	float theta = 2 * PI * random();
	float r = sqrt(random());

	return r * glm::vec3(cos(theta), sin(theta), 0);
}

inline int sign(float x) {
	return x >= 0 ? 1 : -1;
}

inline int clz(unsigned int x) {
	// Keep shifting x by one until leftmost bit does not become 1. 
	int totalBits = sizeof(x) * 8;
	int res = 0;
	while (!(x & (1 << (totalBits - 1)))) {
		x = (x << 1);
		res++;
	}

	return res;
}

inline glm::mat3 generateOrthonormalCS(glm::vec3 normal, glm::vec3 &v, glm::vec3 &u) {
	if (std::abs(normal.x) > std::abs(normal.y))
		v = glm::vec3(-normal.z, 0, normal.x) / std::sqrt(normal.x * normal.x + normal.z * normal.z);
	else
		v = glm::vec3(0, normal.z, -normal.y) / std::sqrt(normal.y * normal.y + normal.z * normal.z);

	u = glm::normalize(glm::cross(normal, v));

	glm::mat3 m;
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;

	m[0][1] = normal.x;
	m[1][1] = normal.y;
	m[2][1] = normal.z;

	m[0][2] = u.x;
	m[1][2] = u.y;
	m[2][2] = u.z;
	
	return m;
}

inline glm::vec3 toWorld(glm::mat3 orthonormalCS, glm::vec3 dirSample) {
	return orthonormalCS * dirSample;
}

//from PBR
//http://www.pbr-book.org/3ed-2018/Materials/BSDFs.html#BSDF::ss
inline glm::vec3 toWorld(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
	return glm::vec3(ss.x * v.x + ts.x * v.y + ns.x * v.z,
                    ss.y * v.x + ts.y * v.y + ns.y * v.z,
                    ss.z * v.x + ts.z * v.y + ns.z * v.z);
}

inline glm::vec3 sphericalToCartesian(float r, float theta, float phi) {
	return r * glm::vec3(std::cos(phi) * std::sin(theta), std::sin(phi) * std::sin(theta), std::cos(theta));
}

//vec4.xyz = direction from surfaceP to sphere sampled point
//vec4.w = pdf
inline glm::vec4 spherePDF(glm::vec3 surfaceP, glm::vec3 lightCenterP, float r) {
	glm::vec3 w = lightCenterP - surfaceP;
	float distanceToCenter = glm::length(w);
	w = glm::normalize(w);

	float q = std::sqrt(1.0 - (r / distanceToCenter) * (r / distanceToCenter));
	glm::vec3 u, v;
	generateOrthonormalCS(w, u, v);

	float r1 = random();
	float r2 = random();
	float theta = std::acos(1 - r1 + r1 * q);
	float phi = TWO_PI * r2;


	glm::vec3 local =  sphericalToCartesian(r, theta, phi);
	//direction to x'
	glm::vec3 nwp = toWorld(local, w, u, v);
	//nwp = (lightCenterP + r * local) - surfaceP;
	//nwp = glm::normalize(nwp);

	//std::cout << nwp.x << " " << nwp.y << " " << nwp.z << std::endl;

	
	//float dotNL = glm::clamp(glm::dot(nwp, surfaceNormal), 0.0f, 1.0f);

	//the dist^2 and dotNL from the pdf cancel out
	float pdf = 1.0f / (2.0f * PI * (1.0f - q));
	//float pdf = glm::max(EPSILON, 1 / (2 * PI * (1.0f - q)));
	return  glm::vec4(nwp, 1.0f / pdf);
	//pdf = dotNL * (1.0f / pdf);
	
	//return 1 / (4 * PI * r * r);
	//if (dotNL > 0.0f) 
		//return pdf;
	
	//else return 1;
}

inline bool isAllZero(glm::vec3 v) {
	return (v.x == 0 && v.y == 0 && v.z == 0) ? true : false;
}

inline float cosTheta(glm::vec3 w) { return w.z; }
inline float cos2Theta(glm::vec3 w) { return w.z * w.z; }
inline float absCosTheta(glm::vec3 w) { return glm::abs(w.z); }

inline float sin2Theta(glm::vec3 w) {
	return glm::max(0.0f, 1.0f - cos2Theta(w));
}

inline float sinTheta(glm::vec3 w) {
	return std::sqrt(sin2Theta(w));
}

inline float tanTheta(glm::vec3 w) {
	return sinTheta(w) / cosTheta(w);
}
inline float tan2Theta(glm::vec3 w) {
	return sin2Theta(w) / cos2Theta(w);
}

inline float cosPhi(glm::vec3 w) {
	float sin = sinTheta(w);
	return (sinTheta == 0) ? 1 : glm::clamp(w.x / sin, -1.0f, 1.0f);
}

inline float sinPhi(glm::vec3 w) {
	float sin = sinTheta(w);
	return (sinTheta == 0) ? 0 : glm::clamp(w.y / sin, -1.0f, 1.0f);
}

inline float cos2Phi(glm::vec3 w) {
	return cosPhi(w) * cosPhi(w);
}
inline float sin2Phi(glm::vec3 w) {
	return sinPhi(w) * sinPhi(w);
}

//beckman
static float roughnessToAlpha(float roughness) {
	roughness = glm::max(roughness, 0.001f);
	float x = std::log(roughness);
	return 1.62142f + 0.819955f * x + 0.1734f * x * x +
		0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
}

inline bool sameHemisphere(glm::vec3 &w, glm::vec3 &wp) {
	return w.z * wp.z > 0;
}

inline void printVec3(glm::vec3 v) {
	std::cout << "[" << v.x << ", " << v.y << ", " << v.z << "]" << std::endl;
}

inline bool greaterThan(glm::vec3 v1, glm::vec3 v2) {
	if (v1.x > v2.x || v1.y > v2.y || v1.z > v2.z)
		return true;
	return false;
}

inline bool greaterEqualThan(glm::vec3 v1, glm::vec3 v2) {
	if (v1.x >= v2.x || v1.y >= v2.y || v1.z >= v2.z)
		return true;
	return false;
}

inline float log4(float x) {
	return log(x) / log(4);
}

inline float log8(float x) {
	return log(x) / log(8);
}


inline std::string formatWithCommas(int value){
	std::string numWithCommas = std::to_string(value);
	int insertPosition = numWithCommas.length() - 3;
	while (insertPosition > 0) {
		numWithCommas.insert(insertPosition, ".");
		insertPosition -= 3;
	}
	return numWithCommas;
}