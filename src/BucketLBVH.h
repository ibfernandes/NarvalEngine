#pragma once
#include "Math.h"
#include <bitset>
#include <limits>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <sstream>
#define DEBUG_LBTBVH false

class BucketLBVH {
public:
	glm::vec3 size;
	glm::vec3 gridSize;
	int vectorSize;

	int bucketSize = 512;
	float *grid;
	int mortonCodesSize;
	glm::vec3 scale = glm::vec3(1.0f);
	glm::vec3 translate = glm::vec3(-scale.x / 2, 0, 0.1f);

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

	enum TREEDIMENSION{BINARYTREE, QUADTREE, OCTREE};
	TREEDIMENSION treeMode = BINARYTREE;

	glm::vec3 getWCS(float x, float y, float z) {
		return glm::vec3(x, y, z);
		//return ((glm::vec3(x, y, z) / gridSize)) * scale + translate;
	}

	glm::vec3 getWCS(glm::vec3 v) {
		return getWCS(v.x, v.y, v.z);
	}

	glm::vec3 getWCS(int mortonCode) {
		int x, y, z;
		decodeSimple3D(mortonCode, x, y, z);
		return glm::vec3(x, y, z);
		//return getWCS(x, y, z);
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
		if(treeMode == BINARYTREE)
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
		}else if(treeMode == OCTREE) {
			div = node / 8.0f;
			res = std::ceil(div - 0.125);
		}
		return res;
	}

	glm::vec2 intersectOuterAABB(Ray r) {
		return intersectBox(r, getWCS(node[1]), getWCS(node[2]));
	}

	glm::vec3 traverseTreeUntil(Ray &r, float depth) {
		int currentNode = 1;
		int currentLevel = 0;
		intersectionsCount = 1;

		glm::vec2 t = intersectBox(r, getWCS(node[currentNode * offsetMinMax - 1]), getWCS(node[currentNode * offsetMinMax]));
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

		while (currentNode != numberOfNodes) {
			bool miss = false;

			//If current node is empty, automatically misses
			if (isEmpty(node[currentNode * offsetMinMax]))
				miss = true;
			else {
				t = intersectBox(r, getWCS(node[currentNode * offsetMinMax - 1]), getWCS(node[currentNode * offsetMinMax]));
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
				}
				else if (getRightmostChild(currentNode, currentLevel) == currentNode) {
					currentNode = getParent(currentNode) + 1;
					currentLevel--;
				}
				else {
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
				}
				else {
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

					glm::vec2 t2 = intersectBox(r, getWCS(min), getWCS(max));
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
							return glm::vec3(finalt, accumulated);
					}
				}

				if (getRightmostChild(getParent(currentNode), currentLevel - 1) == currentNode) {
					currentNode = getParent(currentNode) + 1;
					currentLevel--;
				}
				else {
					currentNode = currentNode + 1;
				}
			}
			else {
				currentNode = getLeftmosChild(currentNode, currentLevel);
				currentLevel++;
			}

			if (currentNode == numberOfNodes)
				break;
		}

		///if (DEBUG_LBVH2)
		for (int i : intersectedMorton)
			std::cout << i << std::endl;
		return glm::vec3(finalt, accumulated);
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
		}else if (treeMode == OCTREE) {
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
			}
			else {
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
			}else if (treeMode == OCTREE) {
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
			if (grid[i] != 0) {
				glm::ivec3 v;
				to3D(i, size.x, size.y, v);
				mortonCodes[count++] = encodeMorton3D(v);
			}
		}

		mortonCodesSize = count;
		radixSort(mortonCodes, mortonCodesSize);

		int nonEmptyBuckets = findNonEmptyBuckets();

		if(treeMode == BINARYTREE)
			levels = std::ceil(std::log2(nonEmptyBuckets)) + 1;
		else if(treeMode == OCTREE)
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
		initGrid();
		genTree();
	}

	BucketLBVH(float *grid, glm::vec3 size, int bucketSize) {
		this->bucketSize = bucketSize;
		this->grid = grid;
		this->size = size;
		this->gridSize = size;
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