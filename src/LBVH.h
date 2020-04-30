#pragma once
#include "Math.h"
#include <bitset>
#include <limits>
#include <glm/glm.hpp>
#include <iostream>
#define DEBUG_LBVH false

class LBVH {
public:
	glm::vec3 size;
	int vectorSize;

	const int bitsPerCoordinate = 10;
	int mortonCodesSize;
	int treeDepth;

	//Interleaved data as Node - Min - Max ...
	int *data;

	int getChild(int m, int currDepth, int index, bool rightChild) {
		int childMortonCode;

		childMortonCode = m ^ (0b1 << (treeDepth - currDepth - 1));

		if (rightChild)
			childMortonCode = childMortonCode | (0b1 << (treeDepth - currDepth));
		else
			childMortonCode = childMortonCode;

		return childMortonCode;
	}

	glm::vec3 getWCS(float x, float y, float z) {
		return glm::vec3(x, y, z) / size;
	}

	glm::vec3 getWCS(glm::vec3 v) {
		return getWCS(v.x, v.y, v.z);
	}

	glm::vec3 getMin(int pos) {
		int x, y, z;
		decodeMorton3D(data[pos],x,y,z);
		return glm::vec3(x, y, z);
	}

	glm::vec3 getMax(int pos) {
		int x, y, z;
		decodeMorton3D(data[pos], x, y, z);
		return glm::vec3(x, y, z);
	}

	glm::vec3 getWCS(int morton) {
		glm::vec3 r = decodeMorton3D(morton);
		return glm::vec3(r.x, r.y, r.z) / size;
	}

	glm::vec3 traverseTree(glm::vec3 origin, glm::vec3 direction) {
		int currentNode = 1;
		int side = 0;
		glm::vec2 t = intersectBox(origin, direction, getWCS(data[currentNode * 3 + 1]), getWCS(data[currentNode * 3 + 2]));
		glm::vec2 finalt = glm::vec2(999999, -999999);
		int currentMorton = 0;
		float accumulated = 0;
		int vectorSize = int(size.x * size.y * size.z);

		if (t.x > t.y) {
			return glm::vec3(9999.0f, -9999.0f, 0.0f);
		}

		//Three ways of ignoring a sub tree:
		// 1 - empty bit is set to 1, which means there's no density.
		// 2 - Already visited
		// 3 - Doesn't intersect
		while (currentMorton != (vectorSize - 2)) {
			currentMorton = data[currentNode * 3];
			t = intersectBox(origin, direction, getWCS(data[currentNode * 3 + 1]), getWCS(data[currentNode * 3 + 2]));
			
			//if miss
			if (t.x > t.y) {
				int tempNode = currentNode;

				//find leaf
				while (tempNode * 2 + 1 < vectorSize)
					tempNode = 2 * tempNode + 1;

				if (data[tempNode * 3] == (vectorSize - 2))
					break;
				else
					currentNode = tempNode;

				int nextParent = data[currentNode * 3] + 1;
				currentMorton = data[currentNode * 3];
				int n = currentNode;

				while (currentMorton != nextParent) {
					n = (n - 0) / 2;
					currentMorton = data[n * 3];
				}
				currentNode = 2 * n + 1;
				side = 0;

				continue;
			}

			glm::vec3 v = decodeMorton3D(data[currentNode * 3]);
			glm::vec2 t2 = intersectBox(origin, direction, getWCS(v.x, v.y, v.z), getWCS(v.x + 1, v.y + 1, v.z + 1));

			//bool test = false;
			if (t2.x <= t2.y) {
				accumulated += 1;
				//std::cout << data[currentNode * 3] << std::endl;
			}

			if (t.x < finalt.x)
				finalt.x = t.x;
			if (t.y >= finalt.y)
				finalt.y = t.y;

			if (currentMorton == (vectorSize - 2))
				break;

			//if not a leaf then keep going down
			if (currentNode * 2 + side < vectorSize)
				currentNode = 2 * currentNode + side;
			else {
				if (side == 1) {
					currentNode = (currentNode - 0) / 2;
					currentNode = 2 * currentNode + 1;
					int nextParent = data[currentNode * 3] + 1;
					currentMorton = data[currentNode * 3];
					int n = currentNode;
					while (currentMorton != nextParent) {
						n = (n - 0) / 2;
						currentMorton = data[n * 3];
					}
					currentNode = 2 * n + 1;
				}
				else {
					currentNode = (currentNode - 0) / 2;
					currentNode = 2 * currentNode + 1;
				}

				side = ++side % 2;
			}

		}

		return glm::vec3(finalt, accumulated);
	}

	void genTree() {
		std::cout << "Generating LBVH tree..." << std::endl;
		int gridArea = size.x * size.y * size.z;
		treeDepth = std::log2(gridArea);
		int root = (gridArea / 2) - 1;

		//Pos 0 is invalid in order to respect the arithmetric rules for right and left child.
		data[3] = root;
		data[4] = encodeMorton3D(0, 0, 0);
		data[5] = encodeMorton3D(size.x, size.y, size.z);
		int count = 2;

		for (int l = 1; l < treeDepth; l++) {
			for (int i = 0; i < std::pow(2, l); i++) {
				int parent = count / 2;
				int mortonChild = getChild(data[parent * 3], l, i, i % 2 != 0);
				int x, y, z;
				decodeMorton3D(mortonChild, x, y, z);

				data[count * 3] = mortonChild;
				data[count * 3 + 1] = mortonChild;
				data[count * 3 + 2] = encodeMorton3D(x + 1, y + 1, z + 1);
				count++;
			}
		}

		count = 1;
		for (int l = 0; l < treeDepth; l++) {
			for (int i = 0; i < std::pow(2, l); i++) {
				int x, y, z;

				//leftmost child
				int leftChild = count;
				while (leftChild * 2 < gridArea) {
					leftChild = 2 * leftChild;

					decodeMorton3D(data[leftChild * 3], x, y, z);
					glm::vec3 m = glm::min(getMin(count * 3 + 1), glm::vec3(x, y, z));
					data[count * 3 + 1] = encodeMorton3D(m.x, m.y, m.z);
				}

				//rightmost child
				int rightChild = count;
				while (rightChild * 2 + 1 < gridArea) {
					rightChild = 2 * rightChild + 1;

					decodeMorton3D(data[rightChild * 3], x, y, z);
					glm::vec3 m = glm::max(getMax(count * 3 + 2), glm::vec3(x + 1, y + 1, z + 1));
					data[count * 3 + 2] = encodeMorton3D(m.x, m.y, m.z);
				}


				if (l == treeDepth - 1) {
					decodeMorton3D(data[count * 3], x, y, z);
					data[count * 3 + 1] = encodeMorton3D(x, y, z);
					data[count * 3 + 2] = encodeMorton3D(x + 1, y + 1, z + 1);
				}
				else {
					decodeMorton3D(data[count * 3], x, y, z);
					glm::ivec3 parent = glm::ivec3(x, y, z);

					decodeMorton3D(data[leftChild * 3], x, y, z);
					decodeMorton3D(data[rightChild * 3], x, y, z);
				}

				count++;
			}
		}

		std::cout << std::endl;
		std::cout << "Finished LBVH tree..." << std::endl;
	}


	LBVH(glm::vec3 size) {
		this->size = size;
		vectorSize = size.x * size.y * size.z;
		data = new int[vectorSize * 3];
		
		genTree();

		//glm::vec3 res = traverseTree(getWCS(0.5f, 0.5, -0.5f), glm::vec3(0,0,1));
		//std::cout << ">>" << res.z << std::endl;
	}
};