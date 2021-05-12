#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include <bitset>
#include <limits>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <sstream>
#define DEBUG_LBTBVH false

namespace narvalengine {
	class BucketLBVH {
	public:
		glm::vec3 size;
		glm::vec3 gridSize;
		glm::vec3 invGridSize;
		int vectorSize;

		int bucketSize = 512;
		float *grid;
		int mortonCodesSize;
		glm::vec3 scale = glm::vec3(1.0f);
		glm::vec3 translate = glm::vec3(0.0f);

		//total of levels (NOT counted as indices, but as quantity)
		int levels;
		int numberOfNodes;

		//node
		int *mortonCodes;
		//Node contains min and max on a layout of min, max, min, max...
		int *node;
		int nodesSize;
		int *offsets;
		int amountOfBuckets;

		int intersectionsCount = 0;
		int const offsetMinMax = 2;

		int *sumOfBaseNTable;
		int numberOfEmptyNodes = 0;

		float maxDensity = 0;

		enum TREEDIMENSION { BINARYTREE, QUADTREE, OCTREE };
		TREEDIMENSION treeMode = BINARYTREE;

		glm::vec3 toGCS(float x, float y, float z) {
			return ((glm::vec3(x, y, z) - translate) / scale) * gridSize;
		}

		//from object coordinate space to grid coordinate space
		glm::vec3 fromOCStoGCS(float x, float y, float z) {
			return (glm::vec3(x, y, z) + glm::vec3(0.5)) * gridSize;
		}

		//Convert from local grid CS (0) to (gridSize) to OCS.
		//Centered AABB with width 1. Corners from (-0.5) to (0.5).
		glm::vec4 getOCS(float x, float y, float z) {
			//return (glm::vec4(x, y, z, 0.0f) / glm::vec4(gridSize, 1.0f)) - glm::vec4(0.5f, 0.5f, 0.5f, 0.0f);
			return glm::vec4(x * invGridSize.x - 0.5f, y * invGridSize.y - 0.5f, z * invGridSize.z - 0.5f, 0);
		}

		glm::vec4 getOCS(glm::vec3 v) {
			return getOCS(v.x, v.y, v.z);
		}

		glm::vec4 getOCS(int mortonCode) {
			int x, y, z;
			decodeSimple3D(mortonCode, x, y, z);
			return getOCS(x, y, z);
		}

		glm::vec3 getWCS(float x, float y, float z) {
			//return glm::vec3(x, y, z);
			return (glm::vec3(x, y, z) / gridSize) * scale + translate;
		}

		glm::vec3 getWCS(glm::vec3 v) {
			return getWCS(v.x, v.y, v.z);
		}

		glm::vec3 getWCS(int mortonCode) {
			int x, y, z;
			decodeSimple3D(mortonCode, x, y, z);
			return getWCS(x, y, z);
		}

		int getLeftmostChild(int node, int leftmost, int rightmost) {
			if (treeMode == BINARYTREE)
				return node + (rightmost - node) + 2 * (node - leftmost) + 1;
			else if (treeMode == OCTREE)
				return node + (rightmost - node) + 8 * (node - leftmost) + 1;
		}

		int getRightmostChild(int node, int leftmost, int rightmost) {
			if (treeMode == BINARYTREE)
				return getLeftmostChild(node, leftmost, rightmost) + 1;
			else if (treeMode == OCTREE)
				return getLeftmostChild(node, leftmost, rightmost) + 7;
		}

		int getLeftmosChild(int node, int lvl) {
			int leftmost;
			if (treeMode == BINARYTREE)
				leftmost = sumOfBaseNTable[lvl] - powBase2(lvl) + 1;
			else if (treeMode == OCTREE)
				leftmost = sumOfBaseNTable[lvl] - powBase8(lvl) + 1;

			int rightmost = sumOfBaseNTable[lvl];
			return getLeftmostChild(node, leftmost, rightmost);
		}

		int getRightmostChild(int node, int lvl) {
			if (treeMode == BINARYTREE)
				return getLeftmosChild(node, lvl) + 1;
			else if (treeMode == OCTREE)
				return getLeftmosChild(node, lvl) + 7;
		}

		int powBase8(int exponent) {
			return 1 << (exponent * 3);
		}

		int powBase2(int exponent) {
			return 1 << (exponent);
		}

		//TODO: maybe generate a table or there is a way to do this without using a for loop
		int sumOfBase2(int exponent) {
			int sum = 1;
			for (int i = 1; i <= exponent; i++)
				sum += powBase2(i);
			return sum;
		}

		int getParent(int node) {
			float div;
			float res;

			if (treeMode == BINARYTREE) {
				div = node / 2.0f;
				res = std::floor(div);
			} else if (treeMode == OCTREE) {
				div = node / 8.0f;
				res = std::ceil(div - 0.125);
			}
			return res;
		}

		glm::vec2 intersectOuterAABB(Ray r) {
			return intersectBox(r, getWCS(node[1]), getWCS(node[2]));
		}

		float density(glm::vec3 gridPoint) {
			if (gridPoint.x >= gridSize.x || gridPoint.y >= gridSize.y || gridPoint.z >= gridSize.z)
				return 0.0f;
			if (gridPoint.x < 0 || gridPoint.y < 0 || gridPoint.z < 0)
				return 0.0f;

			return grid[to1D(gridSize.x, gridSize.y, gridPoint.x, gridPoint.y, gridPoint.z)];
		}

		//Based on PBRT
		float interpolatedDensity(glm::vec3 gridPoint) {
			glm::vec3 pSamples(gridPoint.x - .5f, gridPoint.y - .5f, gridPoint.z - .5f);
			glm::ivec3 pi = glm::ivec3(glm::floor(pSamples.x), glm::floor(pSamples.y), glm::floor(pSamples.z));
			glm::vec3 d = pSamples - glm::vec3(pi);

			// Trilinearly interpolate density values to compute local density
			float d00 = glm::lerp(density(pi), density(pi + glm::ivec3(1, 0, 0)), d.x);
			float d10 = glm::lerp(density(pi + glm::ivec3(0, 1, 0)), density(pi + glm::ivec3(1, 1, 0)), d.x);
			float d01 = glm::lerp(density(pi + glm::ivec3(0, 0, 1)), density(pi + glm::ivec3(1, 0, 1)), d.x);
			float d11 = glm::lerp(density(pi + glm::ivec3(0, 1, 1)), density(pi + glm::ivec3(1, 1, 1)), d.x);
			float d0 = glm::lerp(d00, d10, d.y);
			float d1 = glm::lerp(d01, d11, d.y);
			return glm::lerp(d0, d1, d.z);
		}

		float sampleAt(Ray& r, float depth) {
			glm::vec3 gridPoint = r.getPointAt(depth);
			int currentNode = 1;
			int currentLevel = 0;
			intersectionsCount = 1;

			glm::vec3 bbMin = getOCS(node[currentNode * offsetMinMax - 1]);
			glm::vec3 bbMax = getOCS(node[currentNode * offsetMinMax]);


			//glm::vec3 bbMinWCS = toGCS(-0.73, 0.78, -1.22);
			//glm::vec3 bbMaxWCS = toGCS(0.75, 2.25, 0.25);

			glm::vec2 t = intersectBox(r, bbMin, bbMax);

			if (t.x > t.y)
				return 0.0f;

			gridPoint = fromOCStoGCS(gridPoint.x, gridPoint.y, gridPoint.z);

			return interpolatedDensity(gridPoint);
		}

		//TODO: density sum should be dist * dens + dist * dens...
		// Ray must be in OCS!!
		glm::vec3 traverseTreeUntil(Ray &r, float depth) {

			int currentNode = 1;
			int currentLevel = 0;
			intersectionsCount = 1;

			glm::vec3 invDir = 1.0f / r.direction;
			glm::vec4 ro4 = glm::vec4(r.o, 0);
			glm::vec4 rd4 = glm::vec4(invDir, 0);
			__m128 orSIMD;
			__m128 invDirSIMD;
			orSIMD = _mm_load_ps(&ro4[0]);
			invDirSIMD = _mm_load_ps(&rd4[0]);


			glm::vec3 bbMin = getOCS(node[currentNode * offsetMinMax - 1]);
			glm::vec3 bbMax = getOCS(node[currentNode * offsetMinMax]);

			glm::vec2 t = intersectBox(r, bbMin, bbMax);
			glm::vec2 finalt = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());

			if (t.x > t.y)
				return glm::vec3(finalt, 0);

			std::vector<int> intersectedMorton;

			float accumulated = 0;
			int firstNodeAtDeepestLevel;
			if (treeMode == BINARYTREE)
				firstNodeAtDeepestLevel = numberOfNodes - powBase2(levels - 1) + 1;
			else if (treeMode == OCTREE)
				firstNodeAtDeepestLevel = numberOfNodes - powBase8(levels - 1) + 1;

			while (currentNode != numberOfNodes + 1) {
				bool miss = false;

				//If current node is empty, automatically misses
				if (isEmpty(node[currentNode * offsetMinMax]))
					miss = true;
				else {
					//t = intersectBox(r, getOCS(node[currentNode * offsetMinMax - 1]), getOCS(node[currentNode * offsetMinMax]));
					t = intersectBoxSIMD(orSIMD, invDirSIMD, getOCS(node[currentNode * offsetMinMax - 1]), getOCS(node[currentNode * offsetMinMax]));
					intersectionsCount++;
					if (t.x > t.y)
						miss = true;
				}

				//if (DEBUG_LBTBVH) {
				//	std::cout << "currentNode " << currentNode << std::endl;
				//	std::cout << "currentLevel " << currentLevel << std::endl;
				//}

				//if miss
				if (miss) {

					if (DEBUG_LBTBVH) {
						std::cout << "Missed node: " << currentNode << std::endl;
						glm::vec3 min = decodeSimple3D(node[currentNode * offsetMinMax - 1]);
						glm::vec3 max = decodeSimple3D(node[currentNode * offsetMinMax]);

						std::cout << "min: " << min.x << ", " << min.y << ", " << min.z << std::endl;
						std::cout << "max: " << max.x << ", " << max.y << ", " << max.z << std::endl << std::endl;
					}

					//If it's the rightmost node current level, end
					if (currentNode == sumOfBaseNTable[currentLevel])
						break;


					//if this node is the rightmost child of its parent
					int parent = getParent(currentNode);
					int rightmostChild = getRightmostChild(parent, currentLevel - 1);
					if (rightmostChild == currentNode) {
						currentNode = getParent(currentNode) + 1;
						currentLevel--;
					} else if (getRightmostChild(currentNode, currentLevel) == currentNode) {
						currentNode = getParent(currentNode) + 1;
						currentLevel--;
					} else {
						currentNode = currentNode + 1;
					}

					continue;
				}

				//If we are checking a leaf node
				if (currentNode >= firstNodeAtDeepestLevel) {

					int offsetsPosition = currentNode - firstNodeAtDeepestLevel;
					int startingIndex;
					int elementsOnThisBucket = 0;

					if (offsetsPosition == 0) {
						startingIndex = 0;
						elementsOnThisBucket = offsets[offsetsPosition];
					} else {
						startingIndex = offsets[offsetsPosition - 1];
						elementsOnThisBucket = offsets[offsetsPosition] - offsets[offsetsPosition - 1];
					}

					if (DEBUG_LBTBVH)
						std::cout << "elements on node bucket " << currentNode << ": " << elementsOnThisBucket << std::endl << std::endl;

					//for each voxel on this bucket (leaf node), check which ones does in fact intersect this ray.
					// here we check only mortonCodes that represent non-empty voxels
					for (int i = 0; i < elementsOnThisBucket; i++) {
						int morton = mortonCodes[startingIndex + i];
						glm::vec3 min, max;
						min = decodeMorton3D(morton);
						max = min + 1.0f;

						//glm::vec2 t2 = intersectBox(r, getOCS(min), getOCS(max));
						glm::vec2 t2 = intersectBoxSIMD(orSIMD, invDirSIMD, getOCS(min), getOCS(max));
						//if t.x is negative, this voxel is behind the ray
						if (t2.x < 0 && t2.y < 0) //TODO: not that efficient, should this on all other intersections too
							continue;
						if (t2.x < 0 && t2.y > 0) //if we are inside this voxel, let the nearest be the current point.
							t2.x = 0;
						intersectionsCount++;

						//if intersects this voxel at current bucket, accumulate density and update intersection t's
						if (t2.x <= t2.y) {

							accumulated += grid[to1D(gridSize.x, gridSize.y, min.x, min.y, min.z)];
							if (t2.x < finalt.x)
								finalt.x = t2.x;
							if (t2.y >= finalt.y)
								finalt.y = t2.y;
							//intersectedMorton.push_back(morton);
							float distance = finalt.y - glm::max(0.0f, finalt.x);
							if (distance > depth)
								return glm::vec3(glm::vec2(glm::max(0.0f, finalt.x), finalt.y), accumulated);
						}
					}

					if (currentNode == numberOfNodes + 1)
						break;

					if (getRightmostChild(getParent(currentNode), currentLevel - 1) == currentNode) {
						currentNode = getParent(currentNode) + 1;
						currentLevel--;
					} else {
						currentNode = currentNode + 1;
					}
				} else {
					currentNode = getLeftmosChild(currentNode, currentLevel);
					currentLevel++;
				}
			}

			///if (DEBUG_LBVH2)
			for (int i : intersectedMorton)
				std::cout << i << std::endl;
			return glm::vec3(glm::vec2(glm::max(0.0f, finalt.x), finalt.y), accumulated);
		}

		glm::vec3 traverse(Ray &r) {
			return traverseTreeUntil(r, 99999.0f);
		}

		//returns true if fitted succesfully
		bool fitMortonCodesInThisBucket(glm::vec3 &min, glm::vec3 &max, int &currentMortonIndex, int &bucketRange, int &sum) {
			int count = 0;

			while (currentMortonIndex < mortonCodesSize && mortonCodes[currentMortonIndex] < bucketRange) {
				glm::vec3 coord = decodeMorton3D(mortonCodes[currentMortonIndex]);
				min = glm::min(min, coord);
				max = glm::max(max, coord + 1.0f);

				count++;
				sum++;
				currentMortonIndex++;
			}

			if (count == 0 && bucketRange <= mortonCodes[mortonCodesSize - 1]) {
				bucketRange += bucketSize;
				return true;
			}

			return false;
		}

		void genTree() {
			int firstNodeAtDeepestLevel;

			if (treeMode == BINARYTREE) {
				firstNodeAtDeepestLevel = numberOfNodes - powBase2(levels - 1) + 1;
				amountOfBuckets = powBase2(levels - 1);
			} else if (treeMode == OCTREE) {
				firstNodeAtDeepestLevel = numberOfNodes - powBase8(levels - 1) + 1;
				amountOfBuckets = powBase8(levels - 1);
			}

			offsets = new int[amountOfBuckets];
			//Plus one (+1) since we ignore the index 0 to preserve mathematical properties.
			nodesSize = numberOfNodes * offsetMinMax + 1;
			node = new int[nodesSize];

			int bucketRange = bucketSize;
			int currentMortonIndex = 0;
			int sum = 0;
			//First, generate offsets alongside deepest levels mins and max
			//NOTE: morton codes must be in ascending order
			for (int i = 0; i < amountOfBuckets; i++) {
				glm::vec3 min = glm::vec3(99999, 99999, 99999);
				glm::vec3 max = glm::vec3(-99999, -99999, -99999);

				while (fitMortonCodesInThisBucket(min, max, currentMortonIndex, bucketRange, sum))
					continue;

				offsets[i] = sum;

				//If there's no actual data in this bucket, set empty flag
				if (min.x == 99999) {
					node[firstNodeAtDeepestLevel * offsetMinMax - 1] = setEmptyBit(0);
					node[firstNodeAtDeepestLevel * offsetMinMax] = setEmptyBit(0);
					numberOfEmptyNodes++;
				} else {
					node[firstNodeAtDeepestLevel * offsetMinMax - 1] = encodeSimple3D(min.x, min.y, min.z);
					node[firstNodeAtDeepestLevel * offsetMinMax] = encodeSimple3D(max.x, max.y, max.z);
				}

				firstNodeAtDeepestLevel++;
			}

			if (treeMode == BINARYTREE)
				firstNodeAtDeepestLevel = numberOfNodes - powBase2(levels - 1) + 1;
			else if (treeMode == OCTREE)
				firstNodeAtDeepestLevel = numberOfNodes - powBase8(levels - 1) + 1;

			//currentNode starts at the rightmost node of its level
			int currentNode = firstNodeAtDeepestLevel - 1;
			int numberOfChildren;
			if (treeMode == BINARYTREE)
				numberOfChildren = powBase2(1);
			else
				numberOfChildren = powBase8(1);
			//Bottom up construction of nodes
			//since the deepest level is already created, start one above it and walks the tree BFS backwards
			for (int l = levels - 2; l >= 0; l--) {
				int leftmost;
				int rightmost;
				int pbase;
				if (treeMode == BINARYTREE) {
					leftmost = currentNode - powBase2(l) + 1;
					rightmost = currentNode;
					pbase = powBase2(l);
				} else if (treeMode == OCTREE) {
					leftmost = currentNode - powBase8(l) + 1;
					rightmost = currentNode;
					pbase = powBase8(l);
				}

				for (int i = pbase; i > 0; i--) {

					bool emptyCheck = true;
					int children[8];
					children[0] = getLeftmostChild(currentNode, leftmost, rightmost);
					emptyCheck = emptyCheck && isEmpty(node[children[0] * offsetMinMax]);

					for (int c = 1; c < numberOfChildren; c++) {
						children[c] = children[c - 1] + 1;
						emptyCheck = emptyCheck && isEmpty(node[children[c] * offsetMinMax]);
					}

					if (emptyCheck) {
						node[currentNode * offsetMinMax - 1] = setEmptyBit(0);
						node[currentNode * offsetMinMax] = setEmptyBit(0);
						numberOfEmptyNodes++;
						currentNode--;
						continue;
					}

					//if all children maxs are empty
					/*if (isEmpty(node[children[0] * offsetMinMax]) && isEmpty(node[children[1] * offsetMinMax])) {
						node[currentNode * offsetMinMax - 1] = setEmptyBit(0);
						node[currentNode * offsetMinMax] = setEmptyBit(0);
						numberOfEmptyNodes++;
						currentNode--;
						continue;
					}*/

					glm::ivec3 min = glm::vec3(99999, 99999, 99999);
					glm::ivec3 max = glm::vec3(-99999, -99999, -99999);
					for (int c = 0; c < numberOfChildren; c++) {
						if (isEmpty(node[children[c] * offsetMinMax]))
							continue;

						min = glm::min(min, decodeSimple3D(node[children[c] * offsetMinMax - 1]));
						max = glm::max(max, decodeSimple3D(node[children[c] * offsetMinMax]));
					}

					node[currentNode * offsetMinMax - 1] = encodeSimple3D(min);
					node[currentNode * offsetMinMax] = encodeSimple3D(max);
					currentNode--;
				}
			}
		}

		bool fitMortonCodesInThisBucket(int &currentMortonIndex, int &bucketRange) {
			int count = 0;

			while (currentMortonIndex < mortonCodesSize && mortonCodes[currentMortonIndex] < bucketRange) {
				count++;
				currentMortonIndex++;
			}

			if (count == 0 && bucketRange <= mortonCodes[mortonCodesSize - 1]) {
				bucketRange += bucketSize;
				return true;
			}

			return false;
		}

		int findNonEmptyBuckets() {
			int bucketRange = bucketSize;
			int currentMortonIndex = 0;
			int nonEmptyBuckets = 0;

			while (currentMortonIndex < mortonCodesSize) {
				while (fitMortonCodesInThisBucket(currentMortonIndex, bucketRange))
					continue;
				nonEmptyBuckets++;
			}

			return nonEmptyBuckets;
		}

		void initGrid() {
			vectorSize = size.x * size.y * size.z;
			mortonCodes = new int[vectorSize];

			int count = 0;
			for (int i = 0; i < vectorSize; i++) {
				if (grid[i] > maxDensity)
					maxDensity = grid[i];

				if (grid[i] != 0) {
					glm::ivec3 v;
					to3D(i, size.x, size.y, v);
					mortonCodes[count++] = encodeMorton3D(v);
				}
			}

			mortonCodesSize = count;
			radixSort(mortonCodes, mortonCodesSize);

			int nonEmptyBuckets = findNonEmptyBuckets();

			if (treeMode == BINARYTREE)
				levels = std::ceil(std::log2(nonEmptyBuckets)) + 1;
			else if (treeMode == OCTREE)
				levels = std::ceil(log8(nonEmptyBuckets)) + 1;

			sumOfBaseNTable = new int[levels];
			sumOfBaseNTable[0] = 1;
			numberOfNodes = 1;

			for (int l = 1; l < levels; l++) {
				if (treeMode == BINARYTREE)
					numberOfNodes += powBase2(l);
				else if (treeMode == OCTREE)
					numberOfNodes += powBase8(l);

				sumOfBaseNTable[l] = numberOfNodes;
			}

		}

		std::string getStatus() {
			std::stringstream ss;
			ss << "Size: [" << size.x << ", " << size.y << ", " << size.z << "]" << std::endl;
			ss << "Depth: " << levels << std::endl;
			ss << "Number of nodes: " << numberOfNodes << std::endl;
			ss << "Number of empty nodes: " << numberOfEmptyNodes << std::endl;
			ss << "% of non-empty nodes: " << (1.0f - (numberOfEmptyNodes / (float)numberOfNodes)) * 100 << "%" << std::endl;
			ss << "Bucket size: " << bucketSize << std::endl;
			return ss.str();
		}

		void testGrid() {
			std::cout << "STARTING DIAGNOSIS TEST" << std::endl;
			std::cout << "grid Res: " << std::endl;
			printVec3(gridSize);
			for (int x = 0; x < gridSize.x; x++) {
				float sum = 0;
				for (int y = 0; y < gridSize.y; y++) {
					glm::vec3 normCoord = (glm::vec3(x, y, 0)/gridSize)* scale + translate;
					glm::vec3 hit = traverseTreeUntil(Ray(normCoord, glm::vec3(0, 0, 1)), 99999);

					if(hit.x <= hit.y)
						sum += hit.z;
				}

				std::cout << "x " << x << ": " << sum << std::endl;
			}
		}

		BucketLBVH(float *grid, glm::vec3 size) {
			this->grid = grid;
			this->size = size;
			this->gridSize = size;
			this->invGridSize = 1.0f/size;
			this->bucketSize = 16;
			initGrid();
			genTree();
		}

		BucketLBVH(float *grid, glm::vec3 size, int bucketSize) {
			this->bucketSize = bucketSize;
			this->grid = grid;
			this->size = size;
			this->gridSize = size;
			this->invGridSize = 1.0f / size;
			initGrid();
			genTree();
		}

		~BucketLBVH() {
			delete[] node;
			delete[] offsets;
			delete[] mortonCodes;
			delete[] sumOfBaseNTable;
		}
	};
}