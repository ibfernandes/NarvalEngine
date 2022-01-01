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
#include <glog/logging.h>

namespace narvalengine {
	/**
	 * Implementation from the paper "A Bucket LBVH Construction and Traversal Algorithm for Volumetric Sparse Data" from Igor Batista Fernandes et al.
	 */
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

		//Total of levels (NOT counted as indices, but as quantity).
		int levels;
		int numberOfNodes;

		//Node
		int *mortonCodes;
		//Node contains min and max on a layout of [min, max, min, max ...].
		int *node;
		int nodesSize;
		int *offsets;
		int amountOfBuckets;

		int intersectionsCount = 0;
		const int offsetMinMax = 2;

		int *sumOfBaseNTable;
		int numberOfEmptyNodes = 0;

		float maxDensity = 0;

		enum TREEDIMENSION { BINARYTREE, OCTREE };
		const TREEDIMENSION treeMode = OCTREE;

		/**
		 * Converts from Object Coordinate System (OCS) to Grid Coordinate System (GCS).
		 * Note that for easy of implementation the OCS is assumed to be an AABB centered at origin with width = 1.
		 * Hence, the OCS must be in between [-0.5, -0.5, -0.5] and [0.5, 0.5, 0.5].
		 * 
		 * @param x in Object Coordinate System (OCS). 
		 * @param y in Object Coordinate System (OCS).
		 * @param z in Object Coordinate System (OCS).
		 * @return the coordinates in GCS, varying from [0, 0, 0] and [width, height, depth].
		 */
		glm::vec3 fromOCStoGCS(float x, float y, float z) {
			return (glm::vec3(x, y, z) + glm::vec3(0.5)) * gridSize;
		}

		/**
		 * Converts from Grid Coordinate System (GCS) to Object Coordinate System (OCS).
		 * Note that for easy of implementation the OCS is assumed to be an AABB centered at origin with width = 1.
		 * The GCS must be in between [0, 0, 0] and [width, height, depth].
		 * 
		 * @param x in Grid Coordinate System (GCS). 
		 * @param y in Grid Coordinate System (GCS). 
		 * @param z in Grid Coordinate System (GCS). 
		 * @return the coordinates in OCS, varying from [-0.5, -0.5, -0.5] and [0.5, 0.5, 0.5].
		 */
		glm::vec4 fromGCStoOCS(float x, float y, float z) {
			return glm::vec4(x * invGridSize.x - 0.5f, y * invGridSize.y - 0.5f, z * invGridSize.z - 0.5f, 0);
		}

		/**
		 * Converts from Grid Coordinate System (GCS) to Object Coordinate System (OCS).
		 * Note that for easy of implementation the OCS is assumed to be an AABB centered at origin with width = 1.
		 * The GCS must be in between [0, 0, 0] and [width, height, depth].
		 * 
		 * @param v in Grid Coordinate System (GCS). 
		 * @return the coordinates in OCS, varying from [-0.5, -0.5, -0.5] and [0.5, 0.5, 0.5].
		 */
		glm::vec4 fromGCStoOCS(glm::vec3 v) {
			return fromGCStoOCS(v.x, v.y, v.z);
		}

		/**
		 * Decodes the Morton code {@code mortoncode} and then converts from Grid Coordinate System (GCS) to Object Coordinate System (OCS).
		 * Note that for easy of implementation the OCS is assumed to be an AABB centered at origin with width = 1.
		 * The GCS must be in between [0, 0, 0] and [width, height, depth].
		 * 
		 * @param mortonCode to be decoded and converted to OCS.
		 * @return the coordinates in OCS, varying from [-0.5, -0.5, -0.5] and [0.5, 0.5, 0.5].
		 */
		glm::vec4 fromGCStoOCS(int mortonCode) {
			int x, y, z;
			decodeSimple3D(mortonCode, x, y, z);
			return fromGCStoOCS(x, y, z);
		}

		/**
		 * Returns the index of the left child of {@code node}.
		 * 
		 * Example:									  
		 *												o
		 * 
		 *							o									 o
		 *						 /     \						      /     \	
		 *    leftmost >o				node >o					o		rightmost >o
		 *			/      \			  /       \			 /     \			/     \	
		 *		   o		o	returns >o		   o		o		o		   o       o
		 * 
		 * @param node index in the range [1, 2^levels].
		 * @param leftmost index of the leftmost node at the same level of {@code node}.
		 * @param rightmost index of the rightmost node at the same level of {@code node}.
		 * @return index of the left child of {@code node}.
		 */
		int getLeftChild(int node, int leftmost, int rightmost) {
			if (treeMode == BINARYTREE)
				return node + (rightmost - node) + 2 * (node - leftmost) + 1;
			else if (treeMode == OCTREE)
				return node + (rightmost - node) + 8 * (node - leftmost) + 1;
		}
		
		/**
		 * Returns the index of the right child of {@code node}.
		 *
		 * Example:
		 *											   o
		 *							o									 o
		 *						 /     \						      /     \
		 *    leftmost >o				node >o					o		rightmost >o
		 *			/      \			  /        \			 /     \			/     \
		 *		   o		o	        o  returns >o		o		o		   o       o
		 *
		 * @param node index in the range [1, 2^levels].
		 * @param leftmost index of the leftmost node at the same level of {@code node}.
		 * @param rightmost index of the rightmost node at the same level of {@code node}.
		 * @return index of the right child of {@code node}.
		 */
		int getRightChild(int node, int leftmost, int rightmost) {
			if (treeMode == BINARYTREE)
				return getLeftChild(node, leftmost, rightmost) + 1;
			else if (treeMode == OCTREE)
				return getLeftChild(node, leftmost, rightmost) + 7;
		}

		/**
		 * Returns the index of the left child of {@code node}.
		 * 
		 * @param node index in the range [1, 2^({@code levels}-1)].
		 * @param level in the range [0, {@code levels}-1].
		 * @return index of the left child of {@code node}.
		 */
		int getLeftChild(int node, int level) {
			int leftmost;
			if (treeMode == BINARYTREE)
				leftmost = sumOfBaseNTable[level] - powBase2(level) + 1;
			else if (treeMode == OCTREE)
				leftmost = sumOfBaseNTable[level] - powBase8(level) + 1;

			int rightmost = sumOfBaseNTable[level];
			return getLeftChild(node, leftmost, rightmost);
		}

		/**
		 * Returns the index of the right child of {@code node}.
		 * 
		 * @param node index in the range [1, 2^({@code levels}-1)].
		 * @param level in the range [0, {@code levels}-1].
		 * @return index of the right child of {@code node}.
		 */
		int getRightChild(int node, int level) {
			if (treeMode == BINARYTREE)
				return getLeftChild(node, level) + 1;
			else if (treeMode == OCTREE)
				return getLeftChild(node, level) + 7;
		}

		int powBase8(int exponent) {
			return 1 << (exponent * 3);
		}

		int powBase2(int exponent) {
			return 1 << (exponent);
		}

		/**
		 * Sums all power of two values up to the exponent.
		 * 
		 * @param exponent in the range [0, n].
		 * @return \sum_{i=0}^{n} 2^i.
		 */
		int sumOfBase2(int exponent) {
			int sum = 1;
			for (int i = 1; i <= exponent; i++)
				sum += powBase2(i);
			return sum;
		}

		/**
		 * Calculates the parent index of {@code node}.
		 * 
		 * @param node index in the range [1, 2^({@code levels}-1)].
		 * @return the parent index of {@code node}.
		 */
		int getParent(int node) {
			float div;
			float res;

			if (treeMode == BINARYTREE) {
				div = node / 2.0f;
				res = std::floor(div);
			} else if (treeMode == OCTREE) {
				div = node / 8.0f;
				res = std::ceil(div - 0.125); // 1/8 = 0.125
			}
			return res;
		}

		/**
		 * Samples the volume density at {@code gridPoint}.
		 * 
		 * @param gridPoint in Grid Coordinate System (GCS).
		 * @return sampled density.
		 */
		float density(glm::vec3 gridPoint) {

			//If out of bounds, the density is simply 0.
			if (gridPoint.x > gridSize.x || gridPoint.y > gridSize.y || gridPoint.z > gridSize.z)
				return 0;
			if (gridPoint.x < 0 || gridPoint.y < 0 || gridPoint.z < 0)
				LOG(FATAL) << "Grid point was negative. Value was: " << toString(gridPoint) << ".";

			return grid[to1D(gridSize.x, gridSize.y, gridPoint.x, gridPoint.y, gridPoint.z)];
		}

		/**
		 *  Samples the volume density at {@code gridPoint} using trilinear interpolation.
		 * 
		 * @param gridPoint in Grid Coordinate System (GCS).
		 * @return sampled density.
		 */
		float interpolatedDensity(glm::vec3 gridPoint) {
			glm::ivec3 gridPointInt = glm::ivec3(glm::floor(gridPoint.x), glm::floor(gridPoint.y), glm::floor(gridPoint.z));
			glm::vec3 d = gridPoint - glm::vec3(gridPointInt);

			// Trilinearly interpolate density values to compute local density
			float d00 = glm::lerp(density(gridPointInt), density(gridPointInt + glm::ivec3(1, 0, 0)), d.x);
			float d10 = glm::lerp(density(gridPointInt + glm::ivec3(0, 1, 0)), density(gridPointInt + glm::ivec3(1, 1, 0)), d.x);
			float d01 = glm::lerp(density(gridPointInt + glm::ivec3(0, 0, 1)), density(gridPointInt + glm::ivec3(1, 0, 1)), d.x);
			float d11 = glm::lerp(density(gridPointInt + glm::ivec3(0, 1, 1)), density(gridPointInt + glm::ivec3(1, 1, 1)), d.x);
			float d0 = glm::lerp(d00, d10, d.y);
			float d1 = glm::lerp(d01, d11, d.y);
			return glm::lerp(d0, d1, d.z);
		}

		float sampleAt(Ray& r, float depth) {
			glm::vec3 gridPoint = r.getPointAt(depth);
			int currentNode = 1;
			int currentLevel = 0;

			glm::vec3 bbMin = fromGCStoOCS(node[currentNode * offsetMinMax - 1]);
			glm::vec3 bbMax = fromGCStoOCS(node[currentNode * offsetMinMax]);

			glm::vec2 t = intersectBox(r, bbMin, bbMax);

			if (t.x > t.y)
				return 0.0f;

			return interpolatedDensity(fromOCStoGCS(gridPoint.x, gridPoint.y, gridPoint.z));
		}

		/**
		 * Traverses the tree until reaching the distance {@code depth}.
		 * 
		 * @param r in Object Coordinate System (OCS).
		 * @param depth maximum distance t in which to traverse the tree.
		 * @return accumulated density.
		 */
		glm::vec3 traverseTreeUntil(Ray &r, float depth) {

			int currentNode = 1;
			int currentLevel = 0;
			#ifdef NE_DEBUG_MODE
				intersectionsCount = 1;
			#endif

			glm::vec3 invDir = 1.0f / r.direction;
			glm::vec4 ro4 = glm::vec4(r.o, 0);
			glm::vec4 rd4 = glm::vec4(invDir, 0);
			__m128 orSIMD;
			__m128 invDirSIMD;
			orSIMD = _mm_load_ps(&ro4[0]);
			invDirSIMD = _mm_load_ps(&rd4[0]);


			glm::vec3 bbMin = fromGCStoOCS(node[currentNode * offsetMinMax - 1]);
			glm::vec3 bbMax = fromGCStoOCS(node[currentNode * offsetMinMax]);

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

				//If current node is empty, automatically misses.
				if (isEmpty(node[currentNode * offsetMinMax]))
					miss = true;
				else {
					t = intersectBoxSIMD(orSIMD, invDirSIMD, fromGCStoOCS(node[currentNode * offsetMinMax - 1]), fromGCStoOCS(node[currentNode * offsetMinMax]));
					#ifdef NE_DEBUG_MODE
						intersectionsCount++;
					#endif
					if (t.x > t.y)
						miss = true;
				}

				if (miss) {

					//If it's the rightmost node current level, finish.
					if (currentNode == sumOfBaseNTable[currentLevel])
						break;

					//If this node is the right child of its parent.
					int parent = getParent(currentNode);
					int rightmostChild = getRightChild(parent, currentLevel - 1);
					if (rightmostChild == currentNode) {
						currentNode = getParent(currentNode) + 1;
						currentLevel--;
					} else if (getRightChild(currentNode, currentLevel) == currentNode) {
						currentNode = getParent(currentNode) + 1;
						currentLevel--;
					} else {
						currentNode = currentNode + 1;
					}

					continue;
				}

				//If we are checking a leaf node.
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

					//For each voxel on this bucket (leaf node), check which ones does in fact intersect this ray.
					//Here we check only Morton codes that represent non-empty voxels.
					for (int i = 0; i < elementsOnThisBucket; i++) {
						int morton = mortonCodes[startingIndex + i];
						glm::vec3 min, max;
						min = decodeMorton3D(morton);
						max = min + 1.0f;

						glm::vec2 t2 = intersectBoxSIMD(orSIMD, invDirSIMD, fromGCStoOCS(min), fromGCStoOCS(max));
						
						//If t.x is negative, this voxel is behind the ray.
						if (t2.x < 0 && t2.y < 0)
							continue;

						//If we are inside this voxel, let the nearest be the current point.
						if (t2.x < 0 && t2.y > 0)
							t2.x = 0;

						#ifdef NE_DEBUG_MODE
							intersectionsCount++;
						#endif

						//If intersects this voxel at current bucket, accumulate density and update intersection t's.
						if (t2.x <= t2.y) {
							accumulated += grid[to1D(gridSize.x, gridSize.y, min.x, min.y, min.z)];
							if (t2.x < finalt.x)
								finalt.x = t2.x;
							if (t2.y >= finalt.y)
								finalt.y = t2.y;
							float distance = finalt.y - glm::max(0.0f, finalt.x);
							if (distance > depth)
								return glm::vec3(glm::vec2(glm::max(0.0f, finalt.x), finalt.y), accumulated);
						}
					}

					if (currentNode == numberOfNodes + 1)
						break;

					if (getRightChild(getParent(currentNode), currentLevel - 1) == currentNode) {
						currentNode = getParent(currentNode) + 1;
						currentLevel--;
					} else {
						currentNode = currentNode + 1;
					}
				} else {
					currentNode = getLeftChild(currentNode, currentLevel);
					currentLevel++;
				}
			}

			return glm::vec3(glm::vec2(glm::max(0.0f, finalt.x), finalt.y), accumulated);
		}

		glm::vec3 traverse(Ray &r) {
			return traverseTreeUntil(r, INFINITY);
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
					children[0] = getLeftChild(currentNode, leftmost, rightmost);
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

		std::string toStringStatus() {
			std::stringstream ss;
			ss << "Size: [" << size.x << ", " << size.y << ", " << size.z << "]" << std::endl;
			ss << "Depth: " << levels << std::endl;
			ss << "Number of nodes: " << numberOfNodes << std::endl;
			ss << "Number of empty nodes: " << numberOfEmptyNodes << std::endl;
			ss << "% of non-empty nodes: " << (1.0f - (numberOfEmptyNodes / (float)numberOfNodes)) * 100 << "%" << std::endl;
			ss << "Bucket size: " << bucketSize << std::endl;
			return ss.str();
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