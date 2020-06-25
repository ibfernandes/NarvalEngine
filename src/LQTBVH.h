#pragma once
#include "Math.h"
#include <bitset>
#include <limits>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <sstream>
#define DEBUG_LQTBVH false

class LQTBVH
{
public:
	int *mortonCodes;
	int bucketSize = 2048;
	int mortonCodesSize;
	int numberOfNodes;
	int vectorSize;
	int levels;
	glm::vec3 size, gridSize;
	float *grid;
	//node layout of min, max, min, max...
	int *node;
	int offsetMinMax = 2;
	int *offsets;
	int intersectionsCount = 0;
	int numberOfEmptyNodes = 0;
	int *sumOfBase4Table;

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
		return node + (rightmost - node) + 4 * (node - leftmost) + 1;
	}

	int getRightmostChild(int node, int leftmost, int rightmost) {
		return getLeftmostChild(node, leftmost, rightmost) + 3;
	}

	int getLeftmosChild(int node, int lvl) {
		int leftmost = sumOfBase4Table[lvl] - powBase4(lvl) + 1;
		int rightmost = sumOfBase4Table[lvl];
		return getLeftmostChild(node, leftmost, rightmost);
	}

	int getRightmostChild(int node, int lvl) {
		return getLeftmosChild(node, lvl) + 3;
	}

	int powBase4(int exponent) {
		return 1 << (exponent * 2);
	}

	//TODO: maybe generate a table or there is a way to do this without using a for loop
	int sumOfBase4(int exponent) {
		int sum = 1;
		for (int i = 1; i <= exponent; i++)
			sum += powBase4(i);
		return sum;
	}

	int getParent(int node) {
		float div = node / 4.0f;
		float res = std::ceil(div);
		//if rightmost leaf the divison has a fraction of 0.25f which must be left out 
		if (res - div == 0.75f)
			return res - 1.0f;
		else
			return res;
	}

	glm::vec3 traverseTreeUntil(Ray &r, float depth) {
		int currentNode = 1;
		int currentLevel = 0;
		intersectionsCount = 1;

		glm::vec2 t = intersectBox(r, getWCS(node[currentNode * 2 - 1]), getWCS(node[currentNode * 2]));
		glm::vec2 finalt = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());

		if (t.x > t.y)
			return glm::vec3(finalt, 0);

		std::vector<int> intersectedMorton;

		float accumulated = 0;
		int firstNodeAtDeepestLevel = numberOfNodes - powBase4(levels - 1) + 1;

		int *marker = new int[levels];
		for (int i = 0; i < levels; i++)
			marker[i] = 0;

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

			marker[currentLevel]++;

			if (DEBUG_LQTBVH) {
				std::cout << "currentNode " << currentNode << std::endl;
				std::cout << "currentLevel " << currentLevel << std::endl;
				std::cout << "markers  [";
				for (int i = 0; i < levels; i++)
					std::cout << marker[i] << ", ";
				std::cout << "]" << std::endl << std::endl;
			}

			//if miss
			if (miss) {

				if (DEBUG_LQTBVH) {
					std::cout << "Missed node: " << currentNode << std::endl;
					glm::vec3 min = decodeSimple3D(node[currentNode * offsetMinMax - 1]);
					glm::vec3 max = decodeSimple3D(node[currentNode * offsetMinMax]);

					std::cout << "min: " << min.x << ", " << min.y << ", " << min.z << std::endl;
					std::cout << "max: " << max.x << ", " << max.y << ", " << max.z << std::endl << std::endl;
				}

				//If it's the rightmost node current level, end
				if (currentNode == sumOfBase4Table[currentLevel]) {
					break;
				}

					//if this node is the rightmost child of its parent
				if (getRightmostChild(getParent(currentNode), currentLevel - 1) == currentNode) {
					currentNode = getParent(currentNode) + 1;
					currentLevel--;
				}else if (getRightmostChild(currentNode, currentLevel) == currentNode) {
					currentNode = getParent(currentNode) + 1;
					currentLevel--;
				}else {
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

				if (DEBUG_LQTBVH)
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
		//std::cout << "intersections done: " << intersectionsCount << std::endl;
		return glm::vec3(finalt, accumulated);
	}

	glm::vec3 traverseTree(Ray &r) {
		return traverseTreeUntil(r, 99999.0f);
	}

	void genTree() {
		int firstNodeAtDeepestLevel, amountOfBuckets;

		firstNodeAtDeepestLevel = numberOfNodes - powBase4(levels - 1) + 1;
		amountOfBuckets = powBase4(levels - 1);
		
		offsets = new int[amountOfBuckets];
		//Plus one (+1) since we ignore the index 0 to preserve mathematical properties.
		node = new int[numberOfNodes * offsetMinMax + 1];

		int bucketRange = bucketSize;
		int currentMortonIndex = 0;
		int sum = 0;
		//First, generate offsets alongside deepest levels mins and max
		//NOTE: morton codes must be in ascending order
		for (int i = 0; i < amountOfBuckets; i++) {
			glm::vec3 min = glm::vec3(99999, 99999, 99999);
			glm::vec3 max = glm::vec3(-99999, -99999, -99999);

			//for all morton codes within this bucket range, calculate the whole bucket bbox for non empty voxels
			while (currentMortonIndex < mortonCodesSize && mortonCodes[currentMortonIndex] < bucketRange) {
				glm::vec3 coord = decodeMorton3D(mortonCodes[currentMortonIndex]);
				min = glm::min(min, coord);
				max = glm::max(max, coord + 1.0f);

				sum++;
				currentMortonIndex++;
			}

			offsets[i] = sum;

			bucketRange += bucketSize;

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

		firstNodeAtDeepestLevel = numberOfNodes - powBase4(levels - 1) + 1;
		int currentNode = firstNodeAtDeepestLevel - 1;
		//Bottom up construction of nodes
		//since the deepest level is already created, start one above it and walks the tree BFS backwards
		for (int l = levels - 2; l >= 0; l--) {
			int leftmost  = currentNode - powBase4(l) + 1;
			int rightmost = currentNode;

			for (int i = powBase4(l); i > 0; i--) {

				int children[4];
				children[0] = getLeftmostChild(currentNode, leftmost, rightmost);
				children[1] = children[0] + 1;
				children[2] = children[0] + 2;
				children[3] = children[0] + 3;

				//if all children mins are empty
				if (isEmpty(node[children[0] * offsetMinMax]) && isEmpty(node[children[1] * offsetMinMax]) && isEmpty(node[children[2] * offsetMinMax]) && isEmpty(node[children[3] * offsetMinMax])) {
					node[currentNode * offsetMinMax - 1] = setEmptyBit(0);
					node[currentNode * offsetMinMax] = setEmptyBit(0);
					numberOfEmptyNodes++;
					currentNode--;
					continue;
				}

				glm::ivec3 min = glm::vec3(99999, 99999, 99999);
				glm::ivec3 max = glm::vec3(-99999, -99999, -99999);
				for (int c = 0; c < 4; c++) {
					if (isEmpty(node[children[c] * offsetMinMax]))
						continue;

					min = glm::min(min, decodeSimple3D(node[children[c] * offsetMinMax - 1]) );
					max = glm::max(max, decodeSimple3D(node[children[c] * offsetMinMax]) );
				}

				node[currentNode * offsetMinMax - 1] = encodeSimple3D(min);
				node[currentNode * offsetMinMax] = encodeSimple3D(max);
				currentNode--;
				
			}
		}

		///std::cout << "Finished LBVH2 tree..." << std::endl;

		if (DEBUG_LQTBVH) {
			/*std::cout << "Levels " << levels << std::endl;
			int offsetMinMax = 2;

			std::cout << "Offset: " << std::endl;
			for (int i = 0; i < amountOfBuckets; i++)
				std::cout << offsets[i] << ", ";
			std::cout << std::endl;

			std::cout << "Buckets: " << std::endl;
			for (int i = 0; i < amountOfBuckets; i++) {
				std::cout << "Bucket: " << std::endl;
				std::cout << "Pos Array: " << firstNodeAtDeepestLevel <<  std::endl;
				std::cout << "Morton Range: " << i * bucketSize <<" - " << (i+1) * bucketSize <<  std::endl;
				glm::vec3 mi = decodeSimple3D(node[firstNodeAtDeepestLevel * offsetMinMax]);
				glm::vec3 ma = decodeSimple3D(node[firstNodeAtDeepestLevel * offsetMinMax + 1]);
				if (isEmpty(node[firstNodeAtDeepestLevel * offsetMinMax]))
					std::cout << "Empty" << std::endl;
				else {
					std::cout << "min: ";
					printVec3(mi);
					std::cout << "max: ";
					printVec3(ma);
				}
				std::cout << "" << std::endl;
				firstNodeAtDeepestLevel++;
			}

			std::cout << "Tree: " << std::endl;
			for (int i = 1; i <= numberOfNodes; i++) {
				std::cout << "node["<< i <<"] {" << std::endl;
				glm::vec3 mi = decodeSimple3D(node[i * offsetMinMax]);
				glm::vec3 ma = decodeSimple3D(node[i * offsetMinMax + 1]);
				if (isEmpty(node[i * offsetMinMax]))
					std::cout << "Empty" << std::endl;
				else {
					std::cout << "min: ";
					printVec3(mi);
					std::cout << "max: ";
					printVec3(ma);
				}
				std::cout << "}" << std::endl << std::endl;
			}

			std::cout << std::endl;*/
		}
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

		int highest = 0;

		for (int i = 0; i < 3; i++) {
			size[i] = powBase4(std::ceil(log4(size[i])));
			highest = size[i] > highest ? size[i] : highest;
		}

		vectorSize = highest * highest * highest;

		levels = log4(vectorSize / bucketSize) + 1;
		//levels = std::ceil(log4(vectorSize / float(bucketSize))) + 1;

		sumOfBase4Table = new int[levels];
		sumOfBase4Table[0] = 1;
		numberOfNodes = 1;

		for (int l = 1; l < levels; l++) {
			numberOfNodes += powBase4(l);
			sumOfBase4Table[l] = numberOfNodes;
		}
		//dataSize = nodesSize * 2;
	}

	void printStatus() {
		std::cout << "-------------------------------" << std::endl;
		std::cout << "Bucket Quadtree:" << std::endl;
		std::cout << "Depth: " << levels << std::endl;
		std::cout << "Number of nodes: " << formatWithCommas(numberOfNodes) << std::endl;
		std::cout << "Bucket size: " << bucketSize << std::endl;
		std::cout << "-------------------------------" << std::endl;
	}

	std::string getStatus() {
		std::stringstream ss;
		ss << "Depth: " << levels << std::endl;
		ss << "Number of nodes: " << formatWithCommas(numberOfNodes) << std::endl;
		ss << "Number of empty nodes: " << formatWithCommas(numberOfEmptyNodes) << std::endl;
		ss << "% of non-empty nodes: " << (1.0f - (numberOfEmptyNodes/(float)numberOfNodes)) * 100 << "%" << std::endl;
		ss << "Bucket size: " << bucketSize << std::endl;
		return ss.str();
	}

	void genTestGrid() {
		glm::vec3 gridRes = glm::vec3(4,4,4);
		grid = new float[gridRes.x * gridRes.y * gridRes.z];
		for (int i = 0; i < gridRes.x * gridRes.y * gridRes.z; i++)
			grid[i] = 1.0f;

		this->size = gridRes;
		this->gridSize = gridRes;
	}

	LQTBVH(float *grid, glm::vec3 size) {
		this->grid = grid;
		this->size = size;
		this->gridSize = size;
		//genTestGrid();
		initGrid();
		genTree();

		//traverseTreeUntil( Ray(glm::vec3(0.5f, 0.5f, -1), glm::vec3(0, 0, 1)), 99999);
	}

	LQTBVH(float *grid, glm::vec3 size, int bucketSize) {
		this->bucketSize = bucketSize;
		this->grid = grid;
		this->size = size;
		this->gridSize = size;
		//genTestGrid();
		initGrid();
		genTree();
	}

	~LQTBVH() {
		delete[] node;
		delete[] offsets;
		delete[] mortonCodes;
		delete[] sumOfBase4Table;
	}
};

