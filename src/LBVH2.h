#pragma once
#include "Math.h"
#include <bitset>
#include <limits>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#define DEBUG_LBVH2 false

class LBVH2 {
public:
	glm::vec3 size;
	glm::vec3 gridSize;
	int vectorSize;

	int bucketSize = 128;
	const int bitsPerCoordinate = 10;
	float *grid;
	int mortonCodesSize;
	int treeDepth;
	glm::vec3 scale = glm::vec3(1.0f);
	glm::vec3 translate = glm::vec3( -scale.x/2, 0, 0.1f);

	//total of levels ranging from 0 to levels - 1
	int levels;
	int nodesSize;

	//node
	int *mortonCodes;
	//Node contains min and max on a layout of min, max, min, max...
	int *node;
	int dataSize;
	int *offsets;
	int amountOfBuckets;

	glm::vec3 getWCS(float x, float y, float z) {
		return ((glm::vec3(x, y, z) / gridSize)) * scale + translate;
	}

	glm::vec3 getWCS(glm::vec3 v) {
		return getWCS(v.x, v.y, v.z);
	}

	glm::vec3 getWCS(int mortonCode) {
		int x, y, z;
		decodeMorton3D(mortonCode, x, y, z);
		return getWCS(x, y, z);
	}

	glm::vec2 intersectOuterAABB(Ray r) {
		return intersectBox(r, getWCS(node[2]), getWCS(node[3]));
	}

	glm::vec3 traverseTreeUntil(Ray &r, float depth) {
		int currentNode = 1;
		int currentLevel = 0;

		glm::vec2 t = intersectBox(r, getWCS(node[currentNode * 2]), getWCS(node[currentNode * 2 + 1]));
		glm::vec2 finalt = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());
		if (t.x > t.y)
			return glm::vec3(finalt, 0);

		std::vector<int> intersectedMorton;

		float accumulated = 0;
		int firstNodeAtDeepestLevel = std::pow(2, levels - 1);

		int *marker = new int[levels];
		for (int i = 0; i < levels; i++)
			marker[i] = 0;


		while (currentNode != nodesSize) {
			bool miss = false;

			if (isEmpty(node[currentNode * 2]))
				miss = true;
			else {
				t = intersectBox(r, getWCS(node[currentNode * 2]), getWCS(node[currentNode * 2 + 1]));
				if (t.x > t.y)
					miss = true;
			}

			marker[currentLevel]++;

			if (DEBUG_LBVH2) {
				std::cout << "currentNode " << currentNode << std::endl;
				std::cout << "currentLevel " << currentLevel << std::endl;
				std::cout << "markers  [";
				for (int i = 0; i < levels; i++)
					std::cout << marker[i] << ", ";
				std::cout << "]" << std::endl << std::endl;
			}

			//if miss
			if (miss) {
				if (DEBUG_LBVH2) {
					std::cout << "Missed node: " << currentNode << std::endl;
					glm::vec3 min = decodeMorton3D(node[currentNode * 2]);
					glm::vec3 max = decodeMorton3D(node[currentNode * 2 + 1]);

					std::cout << "min: " << min.x << ", " << min.y << ", " << min.z << std::endl;
					std::cout << "max: " << max.x << ", " << max.y << ", " << max.z << std::endl << std::endl;
				}

				int tempNode = currentNode;

				//find rightmost leaf
				while (tempNode * 2 + 1 <= nodesSize)
					tempNode = 2 * tempNode + 1;

				//if rightmost leaf is the last one of the tree, which means we're on root's right child, then this miss means a break 
				if (tempNode == nodesSize)
					break;

				int newLevel;
				//Starts at parent level and checks the first one with an odd marker
				for (int i = currentLevel; i >= 0; i--)
					//if odd, means that we should go to the right child of the parent node of current node at this level
					if (marker[i] % 2 == 1) {
						newLevel = i;
						break;
					}

				int newNode = currentNode;
				for (int i = 0; i <= (currentLevel - newLevel); i++)
					newNode = newNode / 2;

				currentNode = 2 * newNode + 1;
				currentLevel = newLevel;
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

				if (DEBUG_LBVH2)
					std::cout << "elements on node bucket " << currentNode << ": " << elementsOnThisBucket << std::endl;


				//for each voxel on this bucket (leaf node), check which ones does in fact intersect this ray.
				for (int i = 0; i < elementsOnThisBucket; i++) {
					int morton = mortonCodes[startingIndex + i];
					glm::vec3 min, max;
					min = decodeMorton3D(morton);
					max = min + 1.0f;

					glm::vec2 t2 = intersectBox(r, getWCS(min), getWCS(max));

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

				//if this leaft node is the left child of its parent, then the next one to test is the right child of its parent.
				if (currentNode % 2 == 0)
					currentNode++;
				else {
					int newLevel;
					//Starts at parent level and checks the first one with an odd marker
					for (int i = currentLevel - 1; i >= 0; i--)
						//if odd, means that we should go to the right child of the parent node of current node at this level
						if (marker[i] % 2 == 1) {
							newLevel = i;
							break;
						}

					int newNode = currentNode;
					for (int i = 0; i <= (currentLevel - newLevel); i++)
						newNode = newNode / 2;

					currentNode = 2 * newNode + 1;
					currentLevel = newLevel;
				}
			}
			else {
				currentNode = 2 * currentNode;
				currentLevel++;

				if (currentNode > nodesSize) {
					currentNode = currentNode / 2;
					currentLevel--;
					currentNode++;
				}


			}

			if (currentNode == nodesSize)
				break;
		}

		///if (DEBUG_LBVH2)
			for (int i : intersectedMorton)
				std::cout << i << std::endl;

		return glm::vec3(finalt, accumulated);
	}

	glm::vec3 traverseTree(Ray &r) {
		return traverseTreeUntil(r, 99999.0f);
	}

	void genTree() {
		//std::cout << "Generating LBVH2 tree..." << std::endl;
		int nodesAtDeepestLevel = vectorSize / bucketSize;
		int totalNodes = std::pow(2, levels) - 1;
		int firstNodeAtDeepestLevel = std::pow(2, levels - 1);
		amountOfBuckets = std::pow(2, levels) - 1 - firstNodeAtDeepestLevel;
		offsets = new int[amountOfBuckets];
		node = new int[totalNodes * 2];

		int bucketRange = bucketSize;
		int currentMortonIndex = 0;
		int sum = 0;
		//First, generate offsets alongside deepest levels mins and max
		//NOTE: morton codes must be in ascending order
		for (int i = 0; i < amountOfBuckets; i++) {
			glm::vec3 min = glm::vec3(99999, 99999, 99999);
			glm::vec3 max = glm::vec3(-99999, -99999, -99999);

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
				node[firstNodeAtDeepestLevel * 2] = setEmptyBit(0);
				node[firstNodeAtDeepestLevel * 2 + 1] = setEmptyBit(0);
			}
			else {
				node[firstNodeAtDeepestLevel * 2] = encodeMorton3D(min.x, min.y, min.z);
				node[firstNodeAtDeepestLevel * 2 + 1] = encodeMorton3D(max.x, max.y, max.z);
			}
			firstNodeAtDeepestLevel++;
		}

		firstNodeAtDeepestLevel = std::pow(2, levels - 1);
		totalNodes = firstNodeAtDeepestLevel - 1;
		//Bottom up construction of nodes
		//since the deepest level is already created, start one above it and walks the tree BFS backwards
		for (int l = levels - 2; l >= 0; l--) {
			for (int i = std::pow(2, l); i > 0; i--) {
				int leftChildNode = 2 * totalNodes;
				int rightChildNode = 2 * totalNodes + 1;

				if (isEmpty(node[leftChildNode * 2]) && isEmpty(node[rightChildNode * 2])) {
					node[totalNodes * 2] = setEmptyBit(0);
					node[totalNodes * 2 + 1] = setEmptyBit(0);
					totalNodes--;
					continue;
				}

				node[totalNodes * 2] = encodeMorton3D(glm::min(decodeMorton3D(node[leftChildNode * 2]), decodeMorton3D(node[rightChildNode * 2])));
				node[totalNodes * 2 + 1] = encodeMorton3D(glm::max(decodeMorton3D(node[leftChildNode * 2 + 1]), decodeMorton3D(node[rightChildNode * 2 + 1])));
				totalNodes--;
			}
		}

		totalNodes = std::pow(2, levels) - 1;
		///std::cout << "Finished LBVH2 tree..." << std::endl;

		if (DEBUG_LBVH2) {
			/*std::cout << "Levels " << levels << std::endl;

			std::cout << "Offset: " << std::endl;
			for (int i = 0; i < amountOfBuckets; i++)
				std::cout << offsets[i] << ", ";
			std::cout << std::endl;

			std::cout << "Buckets: " << std::endl;
			for (int i = 0; i < amountOfBuckets; i++) {
				std::cout << "Bucket: " << std::endl;
				std::cout << "Pos Array: " << firstNodeAtDeepestLevel <<  std::endl;
				std::cout << "Morton Range: " << i * bucketSize <<" - " << (i+1) * bucketSize <<  std::endl;
				glm::vec3 mi = decodeMorton3D(mins[firstNodeAtDeepestLevel]);
				glm::vec3 ma = decodeMorton3D(maxs[firstNodeAtDeepestLevel]);
				if (isEmpty(mins[firstNodeAtDeepestLevel]))
					std::cout << "Empty" << std::endl;
				else {
					std::cout << "min: " << mi.x << ", " << mi.y << "," << mi.z << std::endl;
					std::cout << "max: " << ma.x << ", " << ma.y << "," << ma.z << std::endl;
				}
				std::cout << "" << std::endl;
				firstNodeAtDeepestLevel++;
			}

			std::cout << "Tree: " << std::endl;
			for (int i = 1; i <= totalNodes; i++) {
				std::cout << "node["<< i <<"] {" << std::endl;
				glm::vec3 mi = decodeMorton3D(mins[i]);
				glm::vec3 ma = decodeMorton3D(maxs[i]);
				if (isEmpty(mins[i]))
					std::cout << "Empty" << std::endl;
				else {
					std::cout << "min: " << mi.x << ", " << mi.y << "," << mi.z << std::endl;
					std::cout << "max: " << ma.x << ", " << ma.y << "," << ma.z << std::endl;
				}
				std::cout << "}" << std::endl << std::endl;
			}*/

			std::cout << std::endl;
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
			size[i] = std::pow(2, std::ceil(std::log2(size[i])));
			highest = size[i] > highest ? size[i] : highest;
		}

		vectorSize = highest * highest * highest;
		levels = std::log2(vectorSize / bucketSize) + 1;
		nodesSize = std::pow(2, levels) - 1;
		dataSize = nodesSize * 2;
	}

	LBVH2(float *grid, glm::vec3 size) {
		this->grid = grid;
		this->size = size;
		this->gridSize = size;
		initGrid();
		genTree();
	}
};