#pragma once
#define NOMINMAX
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
#include <assert.h>
#include "defines.h"
#define EPSILON3  0.001
#define EPSILON5  0.00001
#define EPSILON	  0.0000000001
#define EPSILON12 0.00000000001
#define PI 3.14159265358979323846264338327950288
#define INVPI 0.31830988618379067154
#define INV2PI 0.15915494309189533577
#define INV4PI float(0.07957747154594766788)
#define TWO_PI 6.283185307179586476925286766559
#define FOUR_PI 12.566370614359172953850573533118
#define PI_OVER_2 1.57079632679489661923
#define PI_OVER_4 0.78539816339744830961
#define INFINITY std::numeric_limits<float>::infinity()

namespace narvalengine {
	//from pbrt Managing Error chapter
	static float maxFloat = std::numeric_limits<float>::max();
	static float infinity = std::numeric_limits<float>::infinity();
	static float machineEpsilon = std::numeric_limits<float>::epsilon() * 0.5;

	inline constexpr float gamma(int n) {
		return (n * machineEpsilon) / (1 - n * machineEpsilon);
	}

	inline float fraction(float f) {
		int v = f;
		return f - float(v);
	}

	inline glm::ivec3 intmod(glm::ivec3 v, int m) {
		return glm::ivec3(v.x % m, v.y % m, v.z % m);
	};

	//Fixed seed in order to have a deterministic result.
	inline std::mt19937 mt(123);
	inline std::uniform_real_distribution<float> dist(0.0, 1.0);

	/**
	 * Samples an uniform random number generator.
	 * 
	 * @return an uniform random number between [0, 1).
	 */
	inline float random() {
		float r = dist(mt);
		//In some very rare instances, dist(mt) returns values >= 1.0f
		if (r >= 1.0f)
			r = 0.999999;

		return r;
	}

	/**
	 * Converts a 2D point to 1D index in YX order.
	 * 
	 * @param width
	 * @param height
	 * @param x
	 * @param y
	 * @return 1D coordinate
	 */
	inline uint32_t to1D(int width, int height, int x, int y) {
		return width * y + x;
	}

	/**
	 * Converts a 2D point to 1D index in YX order.
	 *
	 * @param width
	 * @param height
	 * @param x
	 * @param y
	 * @return 1D coordinate.
	 */
	inline uint32_t to1D(int width, int height, glm::vec2 vec) {
		return to1D(width, height, vec.x, vec.y);
	}

	/**
	 * Converts a 3D point to 1D index in ZYX order.
	 * 
	 * @param width
	 * @param height
	 * @param x coordinate to convert.
	 * @param y coordinate to convert.
	 * @param z coordinate to convert.
	 * @return 1D coordinate.
	 */
	inline uint32_t to1D(int width, int height, int x, int y, int z) {
		return height * width * z + width * y + x;
	}

	/**
	 * Converts a 3D point to 1D index in ZYX order.
	 * 
	 * @param width
	 * @param height
	 * @param vec  coordinates to convert.
	 * @return 1D coordinate.
	 */
	inline uint32_t to1D(int width, int height, glm::vec3 vec) {
		return to1D(width, height, vec.x, vec.y, vec.z);
	}

	/**
	 * Converts 1D index in ZYX order to 3D point.
	 * 
	 * @param index to be converted.
	 * @param width
	 * @param height
	 * @param x coordinate where the result will be stored.
	 * @param y coordinate where the result will be stored.
	 * @param z coordinate where the result will be stored.
	 */
	inline void to3D(int index, int width, int height, int &x, int &y, int &z) {
		z = index / (width * height);
		index -= (z * width * height);
		y = index / width;
		x = index % width;
	}

	/**
	 * Converts 1D index in ZYX order to 3D point.
	 * 
	 * @param index
	 * @param width
	 * @param height
	 * @param vec coordinates where the result will be stored.
	 */
	inline void to3D(int index, int width, int height, glm::ivec3 &vec) {
		return to3D(index, width, height, vec.x, vec.y, vec.z);
	}

	/**
	 * Saturate implementation from HLSL.
	 * 
	 * @param x
	 * @return the value x campled in the interval [0, 1].
	 */
	inline float saturate(float x) {
		return glm::clamp(x, 0.0f, 1.0f);
	}

	/**
	 * Receives an integer with a max value of 10 bits (1024) and interleaves it by 3 bits.
	 * 
	 * @param x
	 * @return interleaved x.
	 */
	inline int interleaveBits(int x) {
		if (x == (1 << 10)) --x;
		x = (x | (x << 16)) & 0b00000011000000000000000011111111;
		x = (x | (x << 8)) & 0b00000011000000001111000000001111;
		x = (x | (x << 4)) & 0b00000011000011000011000011000011;
		x = (x | (x << 2)) & 0b00001001001001001001001001001001;

		return x;
	}


	/**
	 * Encodes a 3D point to Morton code using 10bits for each axis in ZYX order.
	 * 
	 * @param x coordinate to be encoded.
	 * @param y coordinate to be encoded.
	 * @param z coordinate to be encoded.
	 * @return encoded Morton.
	 */
	inline int encodeMorton3D(int x, int y, int z) {
		return (interleaveBits(z) << 2) | (interleaveBits(y) << 1) | interleaveBits(x);
	}

	/**
	 * Encodes a 3D point to Morton code using 10bits for each axis in ZYX order.
	 * 
	 * @param v coordinates to be encoded.
	 * @return encoded Morton.
	 */
	inline int encodeMorton3D(glm::ivec3 v) {
		return (interleaveBits(v.z) << 2) | (interleaveBits(v.y) << 1) | interleaveBits(v.x);
	}

	/**
	 * Decodes morton code in {@code value} to a 3D point. Assumes ZYX order.
	 * 
	 * @param value to be decoded.
	 * @param x coordinate where the result will be stored.
	 * @param y coordinate where the result will be stored.
	 * @param z coordinate where the result will be stored.
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

	/**
	 * Decodes morton code in {@code value} to a 3D point. Assumes ZYX order.
	 * 
	 * @param value to be decoded.
	 * @return vec3 with decoded value.
	 */
	inline glm::ivec3 decodeMorton3D(int value) {
		glm::ivec3 res;
		decodeMorton3D(value, res.x, res.y, res.z);
		return res;
	}

	/**
	 * Simple encoder where the bits are shifted by 10 bits in each axis. Encodes in ZYX order.
	 * 
	 * @param x coordinate to be encoded.
	 * @param y coordinate to be encoded.
	 * @param z coordinate to be encoded.
	 * @return encoded value.
	 */
	inline int encodeSimple3D(int x, int y, int z) {
		int res = 0;
		res = z << 20;
		res = res | y << 10;
		res = res | x;
		return res;
	}

	/**
	 * Simple encoder where the bits are shifted by 10 bits in each axis. Encodes in ZYX order.
	 * 
	 * @param v coordinates to be encoded.
	 * @return encoded value.
	 */
	inline int encodeSimple3D(glm::ivec3 v) {
		return encodeSimple3D(v.x, v.y, v.z);
	}

	/**
	 * Decodes the simple encoder. Assumes ZYX order.
	 * 
	 * @param value to be decoded.
	 * @return decoded value.
	 */
	inline glm::ivec3 decodeSimple3D(int value) {
		glm::ivec3 res;
		//If we do not use unsigned type the right shift was filling with 1's
		value = value & 0b00111111111111111111111111111111;
		unsigned int v = value;
		res.z = v >> 20;
		res.y = (v << 12) >> 22;
		res.x = (v << 22) >> 22;
		return res;
	}

	/**
	 * Decodes the simple encoder. Assumes ZYX order.
	 * 
	 * @param value to be decoded.
	 * @param x coordinate where the result will be stored.
	 * @param y coordinate where the result will be stored.
	 * @param z coordinate where the result will be stored.
	 */
	inline void decodeSimple3D(int value, int &x, int &y, int &z) {
		//If we do not use unsigned type the right shift was filling with 1's
		value = value & 0b00111111111111111111111111111111;
		unsigned int v = value;
		z = v >> 20;
		y = (v << 12) >> 22;
		x = (v << 22) >> 22;
	}

	/**
	 * Decodes the simple encoder. Assumes ZYX order.
	 * 
	 * @param value to be decoded.
	 * @param vec coordinates where the result will be stored.
	 */
	inline void decodeSimple3D(int value, glm::ivec3 &vec) {
		//If not unsigned right shift was filling with 1's
		value = value & 0b00111111111111111111111111111111;
		unsigned int v = value;
		vec.z = v >> 20;
		vec.y = (v << 12) >> 22;
		vec.x = (v << 22) >> 22;
	}

	/**
	 * Tests if a box centered at origin with point {@code bbmin} as min and {@code bbmax} as max
	 * is intersected by a ray with {@code origin} and {@code invDir} using SIMD (Single Instruction, Multiple Data).
	 * 
	 * @param origin ray origin.
	 * @param invDir ray inversed direction, i.e. 1/direction.
	 * @param bbmin min point of this AABB.
	 * @param bbmax max point of this AABB.
	 * @return vector containing (tmin, tmax).
	 */
	inline glm::vec2 intersectBoxSIMD(__m128 origin, __m128 invDir, __m128 bbmin, __m128 bbmax) {
		__m128 t1 = _mm_mul_ps(_mm_sub_ps(bbmin, origin), invDir);
		__m128 t2 = _mm_mul_ps(_mm_sub_ps(bbmax, origin), invDir);
		__m128 vmax4 = _mm_max_ps(t1, t2);
		__m128 vmin4 = _mm_min_ps(t1, t2);
		float* vmax = (float*)&vmax4;
		float* vmin = (float*)&vmin4;
		float tmax = glm::min(vmax[0], glm::min(vmax[1], vmax[2]));
		float tmin = glm::max(vmin[0], glm::max(vmin[1], vmin[2]));

		return glm::vec2(tmin, tmax);
	}

	/**
	 * Tests if a box centered at origin with point {@code bmin} as min and {@code bmax} as max
	 * is intersected by a ray with {@code origin} and {@code invDir} using SIMD (Single Instruction, Multiple Data).
	 * 
	 * @param origin ray origin.
	 * @param invDir ray inversed direction, i.e. 1/direction.
	 * @param bmin min point of this AABB.
	 * @param bmax max point of this AABB.
	 * @return vector containing (tmin, tmax).
	 */
	inline glm::vec2 intersectBoxSIMD(__m128 origin, __m128 invDir, glm::vec4 bmin, glm::vec4 bmax) {
		__m128 bminSIMD;
		__m128 bmaxSIMD;
		bminSIMD = _mm_load_ps(&bmin[0]);
		bmaxSIMD = _mm_load_ps(&bmax[0]);
		return intersectBoxSIMD(origin, invDir, bminSIMD, bmaxSIMD);
	}

	inline glm::vec2 intersectBox(glm::vec3 orig, glm::vec3 dir, glm::vec3 bmin, glm::vec3 bmax) {
		//Line's/Ray's equation
		// o + t*d = y
		// t = (y - o)/d
		//when t is negative, the box is behind the ray origin
		glm::vec3 tMinTemp = (bmin - orig) / dir; //TODO: div by 0
		glm::vec3 tmaxTemp = (bmax - orig) / dir;

		tmaxTemp.x *= 1 + 2 * gamma(3);
		tmaxTemp.y *= 1 + 2 * gamma(3);
		tmaxTemp.z *= 1 + 2 * gamma(3);

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

	/**
	*	Samples a direction vector in the unit sphere using Spherical Coordinates.
	*				 z
	*				 |
	*				 |
	*	phi [0, PI] (|_____ x
	*				  \) theta [0, 2PI]
	*				   \ 
	*					\ y
	*	@param	e1			multiplies the theta/azimuthal angle. Must be in the interval [0,1).
	*	@param	e2			mulitplies the phi/polar angle. Must be in the interval [0,1).
	*	@return	glm::vec3	in Cartesian Coordinates.
	*/
	inline glm::vec3 sampleUnitSphere(float e1, float e2) {
		#ifdef NE_DEBUG_MODE
			assert(e1 >= 0.0f && e1 <=1);
			assert(e2 >= 0.0f && e2 <=1);
		#endif
		
		//Theta varies from		0 rad <= x <= 2PI rad
		//						0 deg <= x <= 360 deg
		float theta = TWO_PI * e1;

		//Phi varies from	     -1 <= x <= 1 
		//						180 deg <= x <= 0 deg
		float phi = acos(1.0f - 2.0f * e2);

		return glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	}

	inline glm::vec3 sampleUnitSphere() {
		return sampleUnitSphere(random(), random());
	}

	/**
	*	Samples a direction vector in the unit sphere using Spherical Coordinates.
	*				 z
	*				 |
	*				 |
	*	phi [0, PI] (|_____ x
	*				  \) theta [0, 2PI]
	*				   \
	*					\ y
	*	@param	cosTheta	
	*	@param	sinTheta	
	*	@param	phi			in radians. Must be in the interval [-1, 1].
	*	@return	glm::vec3	in Cartesian Coordinates.
	*/
	inline glm::vec3 sampleUnitSphere(float cosTheta, float sinTheta, float phi) {
		return glm::vec3(sin(phi) * cosTheta, sin(phi) * sinTheta, cos(phi));
	}

	/**
	 * Generates a random direction vector on the cartesian unit hemisphere using spherical coordinates
	 * where theta varies from 0 to 90 degrees and phi from 0 to 360 degrees.
	 * 
	 * @param e1 in the interval [0, 1].
	 * @param e2 in the interval [0, 1].
	 * @return direction sampled in this hemisphere in Spherical Coordinate System (SCS).
	 */
	inline glm::vec3 sampleUnitHemisphere(float e1, float e2) {

		//Varies from	0 <= x <= 1
		//since sin(0 deg) = 0 and sin(90 deg) = 1, we can simply generate a random number r on [0, 1] without need
		//to caculate the actual sin()
		float sinTheta = std::sqrt(glm::max(0.0f, 1.0f - e1 * e1));

		//Varies from     0 <= x <= 2PI
		//			  0 deg <= x <= 360 deg
		float phi = TWO_PI * e2;
		return glm::vec3(sinTheta * cos(phi), sinTheta * sin(phi), e1);
	}

	inline glm::vec3 randomInUnitDisk() {
		float theta = 2 * PI * random();
		float r = sqrt(random());

		return r * glm::vec3(cos(theta), sin(theta), 0);
	}

	inline glm::vec2 sampleConcentricDisk() {
		glm::vec2 u = glm::vec2(random(), random());
		glm::vec2 uOffset = 2.0f * u - glm::vec2(1.0f, 1.0f);

		// Handle degeneracy at the origin
		if (uOffset.x == 0 && uOffset.y == 0) 
			return glm::vec2(0, 0);

		// Apply concentric mapping to point
		float theta;
		float r;
		if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
			r = uOffset.x;
			theta = PI_OVER_4 * (uOffset.y / uOffset.x);
		}else {
			r = uOffset.y;
			theta = PI_OVER_2 - PI_OVER_4 * (uOffset.x / uOffset.y);
		}
		return r * glm::vec2(std::cos(theta), std::sin(theta));
	}

	/**
	 * Returns the signal multiplier of x.
	 * 
	 * @param x
	 * @return 1 if positive, -1 otherwise.
	 */
	inline int sign(float x) {
		return x >= 0 ? 1 : -1;
	}

	/**
	 * Counts the leading zeroes in an unsigned integer.
	 * 
	 * @param x
	 * @return 
	 */
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

	/**
	 * Counts the leading zeroes in an unsigned integer.
	 * 
	 * @param x
	 * @return 
	 */
	inline int ctz(unsigned int x) {
		// Keep shifting x by one until leftmost bit does not become 1. 
		int totalBits = sizeof(x) * 8;
		int res = 0;
		while (!(x & (1 << (totalBits - 1)))) {
			x = (x << 1);
			res++;
		}

		return 32 - res - 1;
	}
	
	/**
	 *	Generates an orthonormal coordinate system w.r.t. the given normal.
	 * 
	 * 	normal
	 *	|
	 *	| 90°
	 *	|)____ v
	 *	 \) 90°
	 *	  \
	 *	   \ u
	 * 
	 * @param normal
	 * @param v
	 * @param u
	 */
	inline void generateOrthonormalCS(glm::vec3 normal, glm::vec3 &v, glm::vec3 &u) {
		//ns, ss, ts
		if (std::abs(normal.x) > std::abs(normal.y))
			v = glm::vec3(-normal.z, 0, normal.x) / std::sqrt(normal.x * normal.x + normal.z * normal.z);
		else
			v = glm::vec3(0, normal.z, -normal.y) / std::sqrt(normal.y * normal.y + normal.z * normal.z);

		u = glm::normalize(glm::cross(normal, v));
	}

	/**
	 *	Transforms from Local Coordinate System (LCS) where the normal vector v is "up" to World Coordinate System (WCS), where z is up.
	 *	from pbrt:
	 *	http://www.pbr-book.org/3ed-2018/Materials/BSDFs.html#BSDF::ss
	 * 
	 * @param v
	 * @param ns
	 * @param ss
	 * @param ts
	 * @return 
	 */
	inline glm::vec3 toWorld(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
		return glm::vec3(
			ss.x * v.x + ts.x * v.y + ns.x * v.z,
			ss.y * v.x + ts.y * v.y + ns.y * v.z,
			ss.z * v.x + ts.z * v.y + ns.z * v.z);
	}

	/**
	 * Transforms from World Coordinate System (WCS) to Local Coordinate System (LCS) where the normal vector v is "up".
	 *	ns, ss and ts define the LCS axis system
	 * 
	 * @param v
	 * @param ns
	 * @param ss
	 * @param ts
	 * @return 
	 */
	inline glm::vec3 toLCS(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
		return glm::vec3(glm::dot(v, ss), glm::dot(v, ts), glm::dot(v, ns));
	}

	inline glm::vec3 sphericalToCartesianPre(float sinTheta, float cosTheta, float phi) {
		return glm::vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
	}

	inline glm::vec3 sphericalToCartesian(float r, float theta, float phi) {
		return r * glm::vec3(std::cos(phi) * std::sin(theta), std::sin(phi) * std::sin(theta), std::cos(theta));
	}

	inline bool areAllZero(glm::vec3 v) {
		return (v.x == 0 && v.y == 0 && v.z == 0) ? true : false;
	}

	inline bool isBlack(glm::vec3 v) {
		return areAllZero(v);
	}

	inline bool areAllOne(glm::vec3 v) {
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

	inline std::string toString(glm::vec4 v, std::string label = "", int precision = 3) {
		std::stringstream ss;
		ss.precision(precision);
		ss << std::fixed << label << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
		return ss.str();
	}

	inline std::string toString(glm::vec3 v, std::string label = "", int precision = 3) {
		std::stringstream ss;
		ss.precision(precision);
		ss << std::fixed << label << "[" << v.x << ", " << v.y << ", " << v.z << "]";
		return ss.str();
	}

	inline std::string toString(glm::vec2 v, std::string label = "", int precision = 3) {
		std::stringstream ss;
		ss.precision(precision);
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

	/**
	 * Power Heuristic commonly utilized when importance sampling.
	 * 
	 * @param pdf0
	 * @param pdf1
	 * @return 
	 */
	inline float powerHeuristic(float pdf0, float pdf1) {
		return (pdf0*pdf0) / (pdf0*pdf0 + pdf1 * pdf1);
	}

	/**
	*	Trigonometric functions based on pbrt.
	*	Each function is calculated having the z axis (0, 0, 1) as the up vector, analog to the Spherical Coordinate System (SCS). The vector w must be in SCS.
	*/

	/**
	 * Calculates the cos(theta) of {@code w} and (0, 0, 1).
	 * 
	 * @param w in Spherical Coordinate System (SCS).
	 * @return cos(theta).
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

	/**
	 * Creates the rotation matrix given the Euler {@code angles}.
	 * 
	 * @param angles in degrees.
	 * @return euler matrix for rotation in XYZ order.
	 */
	inline glm::mat4 rotate(glm::vec3 angles) {
		return glm::eulerAngleXYZ(glm::radians(angles.x), glm::radians(angles.y), glm::radians(angles.z));
	}

	inline glm::mat4 getTransform(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale) {
		glm::mat4 model(1.0f);
		model = glm::translate(model, translation);
		model = model * rotate(rotation);
		model = glm::scale(model, scale);

		return model;
	};

	/**
	 * Extracts the Euler angles in XYZ order from the matrix {@code t}.
	 * 
	 * @param t matrix to extract Euler angles from.
	 * @return euler angles in degrees.
	 */
	inline glm::vec3 getRotation(glm::mat4 t) {
		glm::vec3 rotation;
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

	/**
	 * Converts {@code ray} from World Coordinate System (WCS) to Object Coordinate System (OCS).
	 * The returned ray direction is not normalized.
	 * 
	 * @param ray in WCS.
	 * @param invTransformToWCS matrix from the object to which convert to.
	 * @return ray in this object OCS.
	 */
	inline Ray transformRay(Ray ray, glm::mat4 invTransformToWCS) {
		Ray r;
		r.o = invTransformToWCS * glm::vec4(ray.o, 1.0f);
		//Direction is not normalized in order to preserve the WCS distances of tNear and tFar. 
		r.d = invTransformToWCS * glm::vec4(ray.d, 0);
		return r;
	}

	inline uint32_t floatToBits(float f) {
		uint32_t ui;
		memcpy(&ui, &f, sizeof(float));
		return ui;
	}

	inline float bitsToFloat(uint32_t ui) {
		float f;
		memcpy(&f, &ui, sizeof(uint32_t));
		return f;
	}

	inline float nextFloatUp(float v) {
		if (std::isinf(v) && v > 0.0)
			return v;

		if (v == -0.0f)
			v = 0.0f;

		uint32_t ui = floatToBits(v);
		if (v >= 0)
			ui++;
		else
			ui--;

		return bitsToFloat(ui);
	}

	inline float nextFloatDown(float v) {
		if (std::isinf(v) && v < 0.0) 
			return v;

		if (v == 0.0f)
			v = -0.0f;

		uint32_t ui = floatToBits(v);
		if (v > 0)
			--ui;
		else
			++ui;

		return bitsToFloat(ui);
	}

	struct Efloat {
		float v;
		float low, high;
		
		Efloat() { }

		Efloat(float v, float err = 0.0f) {
			this->v = v;

			if (err == 0.0f) {
				this->low = v;
				this->high = v;
			}else {
				low = nextFloatDown(v - err);
				high = nextFloatUp(v + err);
			}
		}

		Efloat operator+(Efloat f) const {
			Efloat result;
			result.v = v + f.v;
			
			// Interval arithemetic addition, with the result rounded away from
			// the value r.v in order to be conservative.
			result.low = nextFloatDown(lowerBound() + f.lowerBound());
			result.high = nextFloatUp(upperBound() + f.upperBound());
			return result;
		}

		float upperBound() const { return high; }
		float lowerBound() const { return low; }

		float getAbsoluteError() const {
			return nextFloatUp(std::max(std::abs(high - v), std::abs(v - low)));
		}

		explicit operator float() const { return v; }
	};

	inline void linearToRGB(float *linear, float *rgb) {
		rgb[0] = glm::clamp(linear[0] * 255.0f, 0.0f, 255.0f);
		rgb[1] = glm::clamp(linear[1] * 255.0f, 0.0f, 255.0f);
		rgb[2] = glm::clamp(linear[2] * 255.0f, 0.0f, 255.0f);
	}

	inline void rgbToCIE(float* inputrgb, float* lab) {
		float num = 0;
		float rgb[3] = { 0,0,0 };

		for (int i = 0; i < 3; i++) {
			inputrgb[i] = inputrgb[i] / 255.0f;


			if (inputrgb[i] > 0.04045f)
				inputrgb[i] = std::pow(((inputrgb[i] + 0.055f) / 1.055f), 2.4f);
			else
				inputrgb[i] = inputrgb[i] / 12.92f;


			rgb[i] = inputrgb[i] * 100.0f;
		}

		float xyz[3] = { 0, 0, 0 };


		xyz[0] = rgb[0] * 0.4124f + rgb[1] * 0.3576f + rgb[2] * 0.1805f;
		xyz[1] = rgb[0] * 0.2126f + rgb[1] * 0.7152f + rgb[2] * 0.0722f;
		xyz[2] = rgb[0] * 0.0193f + rgb[1] * 0.1192f + rgb[2] * 0.9505f;


		// Observer= 2°, Illuminant= D65
		xyz[0] = xyz[0] / 95.047f;         // ref_X =  95.047
		xyz[1] = xyz[1] / 100.0f;          // ref_Y = 100.000
		xyz[2] = xyz[2] / 108.883f;        // ref_Z = 108.883


		for (int i = 0; i < 3; i++) {
			if (xyz[i] > 0.008856f)
				xyz[i] = std::pow(xyz[i], 0.3333333333333333);
			else
				xyz[i] = (7.787f * xyz[i]) + (16.0f / 116.0f);
		}


		lab[0] = (116.0f * xyz[1]) - 16.0f;
		lab[1] = 500 * (xyz[0] - xyz[1]);
		lab[2] = 200 * (xyz[1] - xyz[2]);
	}


	inline float deltaE(float* labA, float* labB) {
		float deltaL = labA[0] - labB[0];
		float deltaA = labA[1] - labB[1];
		float deltaB = labA[2] - labB[2];
		float c1 = std::sqrt(labA[1] * labA[1] + labA[2] * labA[2]);
		float c2 = std::sqrt(labB[1] * labB[1] + labB[2] * labB[2]);
		float deltaC = c1 - c2;
		float deltaH = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC;
		deltaH = deltaH < 0 ? 0 : std::sqrt(deltaH);
		float sc = 1.0 + 0.045 * c1;
		float sh = 1.0 + 0.015 * c1;
		float deltaLKlsl = deltaL / (1.0);
		float deltaCkcsc = deltaC / (sc);
		float deltaHkhsh = deltaH / (sh);
		float i = deltaLKlsl * deltaLKlsl + deltaCkcsc * deltaCkcsc + deltaHkhsh * deltaHkhsh;
		return i < 0 ? 0 : std::sqrt(i);
	}

	inline glm::vec3 getRectangleSize(glm::vec3 v0, glm::vec3 v1) {
		for (int i = 0; i < 3; i++)
			if (v0[i] == v1[i])
				v0[i] = v1[i] = 0;

		return glm::abs(v0) + glm::abs(v1);
	}

	// Convert from area measure, as returned by the pdf() call
	// above, to solid angle measure.
	inline float convertAreaToSolidAngle(float pdfArea, glm::vec3 normal, glm::vec3 p1, glm::vec3 p2) {
		glm::vec3 wi = p1 - p2;

		if (glm::length2(wi) == 0)
			return 0;
		else {
			wi = glm::normalize(wi);
			// Convert from area measure, as returned by the Sample() call
			// above, to solid angle measure.
			pdfArea *= glm::distance2(p1, p2) / absDot(normal, -wi);
			if (std::isinf(pdfArea)) return 0;
		}

		return pdfArea;
	}

	//Adapted from pbrt FindInterval
	template <typename Predicate>
	inline int binarySearch(int size, const Predicate& pred) {
		int first = 0;
		int len = size;

		while (len > 0) {
			int half = len >> 1;
			int middle = first + half;

			if (pred(middle)) {
				first = middle + 1;
				len -= half + 1;
			}else
				len = half;
		}

		return glm::clamp(first - 1, 0, size - 2);
	}

	inline float getSphericalPhi(glm::vec3 v) {
		float p = std::atan2(v.y, v.x);
		return (p < 0) ? (p + TWO_PI) : p;
	}

	inline float getSphericalTheta(glm::vec3 v) {
		return std::acos(glm::clamp(v.z, -1.0f, 1.0f));
	}

	/**
	 * Computes the Root Mean Square Error (RMSE) between two vectors.
	 * 
	 * @param c1
	 * @param c2
	 * @return the RMSE of c1 and c2.
	 */
	inline glm::vec3 computeRMSE(glm::vec3 c1, glm::vec3 c2) {
		float r = c1.x - c2.x;
		r *= r;
		float g = c1.y - c2.y;
		g *= g;
		float b = c1.z - c2.z;
		b *= b;

		float dist = sqrt((r + g + b) / 3.0f);

		return glm::vec3(dist, dist, dist);
	}
}