#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "primitives/Ray.h"
#include <limits>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <random>
#include <sstream>
#define EPSILON3  0.001
#define EPSILON5  0.00001
#define EPSILON	  0.0000000001
#define EPSILON12 0.00000000001
#define PI 3.14159265358979323846264338327950288
#define INV4PI float(0.07957747154594766788)
#define TWO_PI 6.283185307179586476925286766559
#define INFINITY 9999999.0

namespace narvalengine {
	inline float fraction(float f) {
		int v = f;
		return f - float(v);
	}

	inline glm::ivec3 intmod(glm::ivec3 v, int m) {
		return glm::ivec3(v.x % m, v.y % m, v.z % m);
	};

	inline std::random_device rd;
	inline std::mt19937 mt(rd());
	inline std::uniform_real_distribution<float> dist(0.0, 1.0);

	inline float random() {
		//On some very rare instances the returned random is equal the upper bound due to floating point issues
		//TODO possible better solution for it? https://stackoverflow.com/questions/25668600/is-1-0-a-valid-output-from-stdgenerate-canonical
		float r = dist(mt);
		if (r >= 1.0f)
			r = 0.999999;

		return r;
	}

	/*
	Converts 2D point to 1D index in YX order.
	*/
	inline int to1D(int width, int height, int x, int y) {
		return width * x + y;
	}

	inline int to1D(int width, int height, glm::vec2 vec) {
		return to1D(width, height, vec.x, vec.y);
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
		glm::vec3 tMinTemp = (bmin - orig) / dir; //TODO: div by 0
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

	inline glm::vec3 sampleUnitSphere(float e1, float e2) {
		//Varies from		0 rad <= x <= 2PI rad
		//					0 deg <= x <= 360 deg
		float theta = TWO_PI * e1;

		//Varies from	     -1 <= x <= 1 
		//				180 deg <= x <= 0 deg
		float phi = acos(1.0f - 2.0f * e2);

		return glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	}

	inline glm::vec3 sampleUnitSphere() {
		return sampleUnitSphere(random(), random());
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
		float sinTheta = std::sqrt(glm::max(0.0f, 1.0f - e1 * e1));

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

	inline void generateOrthonormalCS(glm::vec3 normal, glm::vec3 &v, glm::vec3 &u) {
		//ns, ss, ts
		if (std::abs(normal.x) > std::abs(normal.y))
			v = glm::vec3(-normal.z, 0, normal.x) / std::sqrt(normal.x * normal.x + normal.z * normal.z);
		else
			v = glm::vec3(0, normal.z, -normal.y) / std::sqrt(normal.y * normal.y + normal.z * normal.z);

		u = glm::normalize(glm::cross(normal, v));
	}

	/*
		Transforms from Local Coordinate System(LCS) where the normal vector v is "up" to World Coordinate System (WCS)
		from PBR:
		http://www.pbr-book.org/3ed-2018/Materials/BSDFs.html#BSDF::ss
	*/
	inline glm::vec3 toWorld(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
		return glm::vec3(
			ss.x * v.x + ts.x * v.y + ns.x * v.z,
			ss.y * v.x + ts.y * v.y + ns.y * v.z,
			ss.z * v.x + ts.z * v.y + ns.z * v.z);
	}

	/*
		Transforms from World Coordinate System (WCS) to Local Coordinate System(LCS) where the normal vector v is "up"
	*/
	inline glm::vec3 toLCS(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
		return glm::vec3(glm::dot(v, ss), glm::dot(v, ts), glm::dot(v, ns));
	}

	inline glm::vec3 sphericalToCartesianPre(float cosTheta, float sinTheta, float phi) {
		return glm::vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
	}

	inline glm::vec3 sphericalToCartesian(float r, float theta, float phi) {
		return r * glm::vec3(std::cos(phi) * std::sin(theta), std::sin(phi) * std::sin(theta), std::cos(theta));
	}

	inline bool isAllZero(glm::vec3 v) {
		return (v.x == 0 && v.y == 0 && v.z == 0) ? true : false;
	}

	inline bool isBlack(glm::vec3 v) {
		return isAllZero(v);
	}

	inline bool isAllOne(glm::vec3 v) {
		return (v.x == 1 && v.y == 1 && v.z == 1) ? true : false;
	}

	inline bool allEqualTo(glm::vec3 v, float b) {
		return (v.x == b && v.y == b && v.z == b) ? true : false;
	}

	inline float absDot(glm::vec3 v1, glm::vec3 v2) {
		return glm::abs(glm::dot(v1, v2));
	}

	//beckman
	inline float roughnessToAlpha(float roughness) {
		roughness = glm::max(roughness, 0.001f);
		float x = std::log(roughness);
		return 1.62142f + 0.819955f * x + 0.1734f * x * x +
			0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
	}

	//TODO can be simplified https://github.com/mmp/pbrt-v3/blob/d3d6cc54291f1467ad4ad3c70b89afb1a87379f6/src/core/reflection.h
	inline bool sameHemisphere(glm::vec3 v1, glm::vec3 v2) {
		return glm::dot(v1, v2) > 0 ? true : false;
	}

	inline void printVec2(glm::vec2 v, std::string label = "") {
		std::cout << label << "[" << v.x << ", " << v.y << "]" << std::endl;
	}

	inline void printVec3(glm::vec3 v, std::string label = "") {
		std::cout << label << "[" << v.x << ", " << v.y << ", " << v.z << "]" << std::endl;
	}

	inline std::string toString(glm::vec3 v, std::string label = "") {
		std::stringstream ss;
		ss.precision(3);
		ss << std::fixed << label << "[" << v.x << ", " << v.y << ", " << v.z << "]";
		return ss.str();
	}

	inline std::string toString(glm::vec2 v, std::string label = "") {
		std::stringstream ss;
		ss.precision(3);
		ss << std::fixed << label << "[" << v.x << ", " << v.y << "]";
		return ss.str();
	}

	inline void printMat4(glm::mat4 m, int precision = 1) {
		std::stringstream ss;
		ss.precision(precision);
		ss << std::fixed << "[" << m[0][0] << " \t" << m[1][0] << " \t" << m[2][0] << " \t" << m[3][0] << "]" << std::endl;
		ss << std::fixed << "[" << m[0][1] << " \t" << m[1][1] << " \t" << m[2][1] << " \t" << m[3][1] << "]" << std::endl;
		ss << std::fixed << "[" << m[0][2] << " \t" << m[1][2] << " \t" << m[2][2] << " \t" << m[3][2] << "]" << std::endl;
		ss << std::fixed << "[" << m[0][3] << " \t" << m[1][3] << " \t" << m[2][3] << " \t" << m[3][3] << "]" << std::endl;
		std::cout << ss.str();
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

	inline std::string formatWithCommas(int value) {
		std::string numWithCommas = std::to_string(value);
		int insertPosition = numWithCommas.length() - 3;
		while (insertPosition > 0) {
			numWithCommas.insert(insertPosition, ".");
			insertPosition -= 3;
		}
		return numWithCommas;
	}

	inline float powerHeuristic(float pdf0, float pdf1) {
		return (pdf0*pdf0) / (pdf0*pdf0 + pdf1 * pdf1);
	}

	/*
		Trigonometric functions based on pbrt
		Each function is calculated having the z axis (0, 0, 1) as the up vector (Analog to the spherical coordinates system). Vector w must be in this coordinate space
	*/
	inline float cosTheta(const glm::vec3 w) {
		return w.z;
	}
	inline float cos2Theta(const glm::vec3 w) {
		return w.z * w.z;
	}
	inline float absCosTheta(const glm::vec3 w) {
		return std::abs(w.z);
	}
	inline float sin2Theta(const glm::vec3 w) {
		return glm::max((float)0, (float)1 - cos2Theta(w));
	}
	inline float sinTheta(const glm::vec3 w) {
		return std::sqrt(sin2Theta(w));
	}
	inline float tanTheta(const glm::vec3 w) {
		return sinTheta(w) / cosTheta(w);
	}
	inline float tan2Theta(const glm::vec3 w) {
		return sin2Theta(w) / cos2Theta(w);
	}
	inline float cosPhi(const glm::vec3 w) {
		float sinThetaVar = sinTheta(w);
		return (sinThetaVar == 0) ? 1 : glm::clamp(w.x / sinThetaVar, -1.0f, 1.0f);
	}
	inline float sinPhi(const glm::vec3 w) {
		float sinThetaVar = sinTheta(w);
		return (sinThetaVar == 0) ? 0 : glm::clamp(w.y / sinThetaVar, -1.0f, 1.0f);
	}
	inline float cos2Phi(const glm::vec3 &w) {
		return cosPhi(w) * cosPhi(w);
	}
	inline float sin2Phi(const glm::vec3 &w) {
		return sinPhi(w) * sinPhi(w);
	}

	//not tested
	inline glm::mat4 fromColToRowMajor(glm::mat4 src) {
		glm::mat4 dest;
		float* destPointer = &dest[0][0];
		float* srcPointer = &src[0][0];

		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++)
				destPointer[row * 4 + col] = srcPointer[col * 4 + row];
		
		return dest;
	}

	//not tested
	inline glm::mat4 fromRowToColMajor(glm::mat4 src) {
		glm::mat4 dest;
		float* destPointer = &dest[0][0];
		float* srcPointer = &src[0][0];
		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++)
				destPointer[col * 4 + row] = srcPointer[row * 4 + col];

		return dest;
	}

	//Angle in degrees
	//v axis to be rotated
	inline glm::mat4 rotateAxis(glm::vec3 v, float angle) {
		float angleRad = glm::radians(angle);

		float s = sin(angleRad);
		float c = cos(angleRad);
		float ic = 1.0 - c;

		return glm::mat4(v.x*v.x*ic + c, v.y*v.x*ic - s * v.z, v.z*v.x*ic + s * v.y, 0.0,
			v.x*v.y*ic + s * v.z, v.y*v.y*ic + c, v.z*v.y*ic - s * v.x, 0.0,
			v.x*v.z*ic - s * v.y, v.y*v.z*ic + s * v.x, v.z*v.z*ic + c, 0.0,
			0.0, 0.0, 0.0, 1.0);
	}

	inline glm::mat4 rotate(glm::vec3 angles) {
		glm::vec3 output;
		return glm::eulerAngleXYZ(glm::radians(angles.x), glm::radians(angles.y), glm::radians(angles.z));
		//return rotateAxis(glm::vec3(1, 0, 0), angles.x) * rotateAxis(glm::vec3(0, 1, 0), angles.y) * rotateAxis(glm::vec3(0, 0, 1), angles.z);
	}

	inline glm::mat4 getTransform(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale) {
		glm::mat4 model(1.0f);
		model = glm::translate(model, translation);
		//model = model * glm::eulerAngleXYZ(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z));
		model = model * rotate(rotation);
		model = glm::scale(model, scale);

		float* modelPointer = &model[0][0];
		for (int i = 0; i < 4 * 4; i++) {
			modelPointer[i] = ((int)(modelPointer[i] / EPSILON3) * EPSILON3); //to avoid rounding errors, ignore decimals after 3 places
			modelPointer[i] = (modelPointer[i] == 0) ? 0 : modelPointer[i]; //to avoid negative zeroes that affect atan2 rotation angles
		}

		return model;
	};

	inline glm::vec3 getRotation(glm::mat4 t) {
		//Pitch, Yaw, Roll
		glm::vec3 rotation;
		
		/*float T1 = atan2(t[2][1], t[2][2]);
		float C2 = glm::sqrt(t[0][0] * t[0][0] + t[1][0] * t[1][0]);
		float T2 = atan2(-t[2][0], C2);
		float S1 = glm::sin(T1);
		float C1 = glm::cos(T1);
		float T3 = atan2(S1 * t[0][2] - C1 * t[0][1], C1 * t[1][1] - S1 * t[1][2]);

		rotation.x = -T1;
		rotation.y = -T2;
		rotation.z = -T3;*/

		glm::extractEulerAngleXYZ(t, rotation.x, rotation.y, rotation.z);

		rotation = glm::degrees(rotation);
		return rotation;
	}

	inline glm::vec3 getScale(glm::mat4 t) {
		glm::vec3 scale;
		glm::mat4 localMatrix(t);

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				localMatrix[i][j] /= localMatrix[3][3];

		glm::mat3 row;
		glm::vec3 skew;
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				row[i][j] = localMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = glm::length(row[0]);// v3Length(Row[0]);

		row[0] = glm::detail::scale(row[0], static_cast<float>(1));

		// Compute XY shear factor and make 2nd row orthogonal to 1st.
		skew.z = glm::dot(row[0], row[1]);
		row[1] = glm::detail::combine(row[1], row[0], static_cast<float>(1), -skew.z);

		// Now, compute Y scale and normalize 2nd row.
		scale.y = glm::length(row[1]);
		row[1] = glm::detail::scale(row[1], static_cast<float>(1));
		skew.z /= scale.y;

		// Compute XZ and YZ shears, orthogonalize 3rd row.
		skew.y = glm::dot(row[0], row[2]);
		row[2] = glm::detail::combine(row[2], row[0], static_cast<float>(1), -skew.y);
		skew.x = glm::dot(row[1], row[2]);
		row[2] = glm::detail::combine(row[2], row[1], static_cast<float>(1), -skew.x);

		// Next, get Z scale and normalize 3rd row.
		scale.z = glm::length(row[2]);

		return scale;
	}

	inline glm::vec3 getTranslation(glm::mat4 t) {
		glm::vec3 translation;

		glm::mat4 localMatrix(t);

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				localMatrix[i][j] /= localMatrix[3][3];

		translation.x = localMatrix[3][0];
		translation.y = localMatrix[3][1];
		translation.z = localMatrix[3][2];

		return translation;
	}

	inline float avg(glm::vec3 v) {
		return (v.x + v.y + v.z) / 3.0f;
	}

	/*
		Converts ray from World Coordinate System(WCS) to Object Coordinate System(OCS)
	*/
	inline Ray transformRay(Ray ray, glm::mat4 invTransformToWCS) {
		Ray r;
		r.o = invTransformToWCS * glm::vec4(ray.o, 1.0f);
		r.d = invTransformToWCS * glm::vec4(ray.d, 0); //TODO: How to transform direction? should remove scale too should normalize
		//for now it is not normalized to preserve the real WCS distance of tNear and tFar

		return r;
	}
}