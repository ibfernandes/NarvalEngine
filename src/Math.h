#pragma once
#include <limits>
#include <vector>
#include <glm/glm.hpp>

struct Ray {
	glm::vec3 origin, direction;
};

/*
Converts 3D point to 1D index in ZYX order.
*/
inline int to1D(int width, int height, int x, int y, int z) {
	return height * width * z + width * y + x;
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

inline int encodeSimple3D(glm::ivec3 value) {
	int res = 0;
	res = value.z << 20;
	res = res | value.y << 10;
	res = res | value.x;
	return res;
}

inline glm::ivec3 decodeSimple3D(int value) {
	glm::ivec3 res;
	//If not unsigned right shift was filling with 1's
	unsigned int v = value;
	res.z = v >> 20;
	res.y = (v << 12) >> 22;
	res.x = (v << 22) >> 22;
	return res;
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