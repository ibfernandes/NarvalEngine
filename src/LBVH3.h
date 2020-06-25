#pragma once
#include "Math.h"
#include <bitset>
#include <limits>
#include <vector>
#include <stack>
#include <iostream>
#include <glm/glm.hpp>
#include <sstream>
#define DEBUG_LBVH3 false
#define BRICK_DIMENSION_SIZE 8
#define BRICK_SIZE 512

struct AABB {
	glm::vec3 min, max;
};

struct Brick {
	//brick of size 8^3 = 512
	//ZYX order
	int mortonCode;
	float *grid;
	bool isEmpty = true;
};

struct Node {
	int id; // for debug purposes
	struct Node *leftChild = nullptr;
	struct Node *rightChild = nullptr;
	struct Node *parent = nullptr;
	AABB aabb;
	bool isLeaf = false;
	Brick *brick;
};


//Implementation based on "A Linear Time BVH Construction Algorithm for Sparse Volumes"
//Some interesting implementation in CUDA using Karras: https://github.com/aeoleader/CudaBVH
class LBVH3 {
public:
	glm::vec3 gridResolution;
	int gridSize = 0;
	float *grid;

	glm::vec3 bricksResolution;
	int bricksSize;
	Brick *bricks;

	int sortedBricksSize;
	Brick *sortedBricks;

	Node *nodes;
	int nodesSize = 0;
	int levels = 0;
	int intersectionsCount = 0;

	float calculateInnerBrick(Ray r, glm::vec3 mortonMin, Node *n, int &intersectionCount) {

		float acc = 0;
		for (int i = 0; i < BRICK_SIZE; i++) {
			glm::ivec3 vec;
			to3D(i, BRICK_DIMENSION_SIZE, BRICK_DIMENSION_SIZE, vec);
			vec += mortonMin;
			if (greaterEqualThan(vec, gridResolution))
				continue;

			glm::vec2 hit = intersectBox(r, vec, vec + 1);
			intersectionCount++;
			if (hit.x <= hit.y) {
				acc += n->brick->grid[i];
					//printVec3(vec);
					//std::cout <<"brick: " << n->brick->grid[i] << std::endl;
					//std::cout << "grid: " << grid[to1D(gridResolution.x, gridResolution.y, vec)] << std::endl;
			}
		}

		return acc;
	}

	/*
	Pseudo code from paper:

	procedure TRAVERSE(Ray,Root)
		t = 0 Initialize ray parameter
		STACK.PUSH(Root) Initialize stack

		while not STACK.EMPTY do TopOfLoop

			Node = STACK.POP
			while NODE.ISINNER do
				HITL = INTERSECT(Ray, Node.LeftChild)
				HITR = INTERSECT(Ray, Node.RightChild)

				if HITL and HITR then
					N= NEAR(t, Node.LeftChild, Node.RightChild)
					F = FAR(t, Node.LeftChild, Node.RightChild)
					Node = N
					STACK.PUSH(F)
				else if HITL then
					Node= Node.LeftChild
				else if HITR then
					Node= Node.RightChild
				else
					goto TopOfLoop Pop another node
				end if
			end while

			INTEGRATE(Node) This node is the closest leaf
			t = TFAR(Ray, Node)
		end while
	end procedure
	*/
	glm::vec3 traverse(Ray r) {
		std::stack<Node*> stack;
		float acc = 0;
		intersectionsCount = 0;

		//push root
		stack.push(&nodes[0]);
		Node *closestLeaf;

		while (!stack.empty()) {
			stack.top();
			Node *currentNode = stack.top();
			stack.pop();

			while (!currentNode->isLeaf) {
				Node *leftChild = currentNode->leftChild;
				Node *rightChild = currentNode->rightChild;

				glm::vec2 hitl = intersectBox(r, leftChild->aabb.min, leftChild->aabb.max);
				glm::vec2 hitr = intersectBox(r, rightChild->aabb.min, rightChild->aabb.max);
				intersectionsCount++;
				intersectionsCount++;

				bool didHitL = (hitl.x > hitl.y) ? false : true;
				bool didHitR = (hitr.x > hitr.y) ? false : true;

				if (didHitL && didHitR) {
					Node *near, *far;
					if (hitl.x > hitr.x) {
						near = rightChild;
						far = leftChild;
					}
					else {
						near = leftChild;
						far = rightChild;
					}
					currentNode = near;
					stack.push(far);
				}
				else if (didHitL) {
					currentNode = leftChild;
				}
				else if (didHitR) {
					currentNode = rightChild;
				}
				else {
					break;
				}
			}

			closestLeaf = currentNode;
			if(closestLeaf->isLeaf)
				acc += calculateInnerBrick(r, decodeMorton3D(closestLeaf->brick->mortonCode), closestLeaf, intersectionsCount);

			if (DEBUG_LBVH3 && closestLeaf->isLeaf) {
				std::cout << "HIT {" << std::endl;
				std::cout << "id: " << closestLeaf->id << std::endl;
				std::cout << "isLeaf: " << closestLeaf->isLeaf << std::endl;
				std::cout << "morton code: " << closestLeaf->brick->mortonCode << std::endl;
				printVec3(closestLeaf->aabb.min);
				printVec3(closestLeaf->aabb.max);
				std::cout << "}" << std::endl;
			}
		}

		//std::cout << "intersections done: " << intersectionsCount << std::endl;
		//std::cout << "acc: " << acc << std::endl;
		return glm::vec3(0, 0, acc);
	}

	void printTree(){
		for (int i = 0; i < nodesSize; i++) {
			std::cout << "{" << std::endl;
			std::cout << "id: " << nodes[i].id << std::endl;
			if (!nodes[i].isLeaf) {
				std::cout << "childA: " << nodes[i].leftChild->id << std::endl;
				std::cout << "childB: " << nodes[i].rightChild->id << std::endl;
			}
			else {
				std::cout << "parent: " << nodes[i].parent->id << std::endl;
			}
			printVec3(nodes[i].aabb.min);
			printVec3(nodes[i].aabb.max);
			std::cout << "}" << std::endl;
		}
	}

	int delta(int x, int y, int numObjects){
		if (x >= 0 && x <= numObjects - 1 && y >= 0 && y <= numObjects - 1)
			return clz(sortedBricks[x].mortonCode ^ sortedBricks[y].mortonCode);
		
		return -1;
	}

	glm::ivec2 determineRange(int numObjects, int idx){
		int d = sign(delta(idx, idx + 1, numObjects) - delta(idx, idx - 1, numObjects));
		int dmin = delta(idx, idx - d, numObjects);
		int lmax = 2;

		while (delta(idx, idx + lmax * d, numObjects) > dmin)
			lmax = lmax * 2;

		int l = 0;
		for (int t = lmax / 2; t >= 1; t /= 2)
			if (delta(idx, idx + (l + t)*d, numObjects) > dmin)
				l += t;
		
		int j = idx + l * d;
		glm::ivec2 range;
		range.x = glm::min(idx, j);
		range.y = glm::max(idx, j);
		
		//if (idx == 38043 || idx == 38044 || idx == 38045 || idx == 38046 || idx == 38047 || idx == 38048)
		//	printf("idx %d range :%d - %d j: %d morton: %d\n", idx, range.x, range.y, j, sortedBricks[idx].mortonCode);
		return range;
	}


	/* Paper: use Karras’ algorithm [6] to efficiently find split positions (Median split operation using Morton codes) in parallel.
	The algorithm output consists of one list of inner tree nodesand a second list with leaf nodes
	(the list of leaf nodes is actually known a priori) for which parent and child relationships are set up appropriately.
	relationships are set upappropriately.*/
	//Karras related source: https://devblogs.nvidia.com/thinking-parallel-part-iii-tree-construction-gpu/
	int generateSplitPositions(int first, int last) {
		// Identical Morton codes => split the range in the middle.

		int firstCode = sortedBricks[first].mortonCode;
		int lastCode = sortedBricks[last].mortonCode;

		if (firstCode == lastCode)
			return (first + last) >> 1;

		// Calculate the number of highest bits that are the same
		// for all objects, using the count-leading-zeros intrinsic.

		//clz is an implementation equivalent to CUDA's __clz
		int commonPrefix = clz(firstCode ^ lastCode);

		// Use binary search to find where the next bit differs.
		// Specifically, we are looking for the highest object that
		// shares more than commonPrefix bits with the first one.

		int split = first; // initial guess
		int step = last - first;

		do {
			step = (step + 1) >> 1; // exponential decrease
			int newSplit = split + step; // proposed new position

			if (newSplit < last) {
				int splitCode = sortedBricks[newSplit].mortonCode;
				int splitPrefix = clz(firstCode ^ splitCode);
				if (splitPrefix > commonPrefix)
					split = newSplit; // accept proposal
			}
		} while (step > 1);

		return split;
	}

	AABB encapsuleAABB(AABB a1, AABB a2) {
		glm::vec3 min = glm::min(a1.min, a2.min);
		glm::vec3 max = glm::max(a1.max, a2.max);

		return AABB{min, max};
	}

	void propagateAABB(int numOfLeafs) {
		int totalNodes = numOfLeafs + numOfLeafs - 1;
		levels = 1;
		int currLevels = 0;

		for (int idx = numOfLeafs - 1; idx < totalNodes; idx++) { // in parallel
			Node *node = &nodes[idx];
			currLevels = 0;

			while (node != nullptr) {
				if (node->parent == nullptr) // if we reached root
					break;
				node->parent->aabb = encapsuleAABB(node->parent->aabb, node->aabb);

				node = node->parent;
				currLevels++;
			}

			if (currLevels > levels)
				levels = currLevels;
		}
	}

	AABB generateLeafAABB(Brick brick) {
		glm::ivec3 min = decodeMorton3D(brick.mortonCode);
		glm::ivec3 max = min + BRICK_DIMENSION_SIZE;

		return AABB{min, max};
	}

	//numOfLeafs = number of non-empty sorted bricks
	void generateHierarchy(int numOfLeafs) {
		int totalNodes = numOfLeafs + numOfLeafs - 1;
		nodesSize = totalNodes;
		//LeafNode *leafNodes = new LeafNode[numOfLeafs];
		//InternalNode *internalNodes = new InternalNode[numOfLeafs - 1];
		nodes = new Node[totalNodes];

		// Construct leaf nodes.
		// Note: This step can be avoided by storing
		// the tree in a slightly different way.

		int c = 0;
		for (int idx = numOfLeafs - 1; idx < totalNodes; idx++) { // in parallel
			nodes[idx].id = idx;
			nodes[idx].brick = &sortedBricks[c++];
			nodes[idx].isLeaf = true;
			nodes[idx].aabb = generateLeafAABB(*nodes[idx].brick);
		}

		// Construct internal nodes.
		for (int idx = 0; idx < numOfLeafs - 1; idx++){ // in parallel
			// Find out which range of objects the node corresponds to.
			// (This is where the magic happens!)

			glm::ivec2 range = determineRange(numOfLeafs, idx);
			int first = range.x;
			int last = range.y;

			// Determine where to split the range.

			int split = generateSplitPositions(first, last);

			// Select childA.

			Node* childA;
			if (split == first) { //split + numOfLeafs 
				childA = &nodes[(numOfLeafs-1) + split];
				childA->isLeaf = true;
			}else
				childA = &nodes[split];

			// Select childB.

			Node* childB;
			if (split + 1 == last) { 
				childB = &nodes[(numOfLeafs-1) + split + 1];
				childB->isLeaf = true;
			}else
				childB = &nodes[split + 1];

			// Record parent-child relationships.

			childA->parent = &nodes[idx];
			childB->parent = &nodes[idx];
			nodes[idx].id = idx;
			nodes[idx].leftChild = childA;
			nodes[idx].rightChild = childB;
			nodes[idx].aabb = AABB{glm::vec3(9999999), glm::vec3(-9999999)};
			//childA->parent = &internalNodes[idx];
			//childB->parent = &internalNodes[idx];
		}
		
		propagateAABB(numOfLeafs);
		//printTree();
	}

	void radixSort() {
		//int arrSize = sizeof(sortedBricks) / sizeof(sortedBricks[0]); 
		std::vector<Brick> gridVec(sortedBricks, sortedBricks + sortedBricksSize);
		std::vector<Brick> tempArr(sortedBricksSize);
		const int bitsPerPass = 6;
		int nBits = 30;
		int nPasses = nBits / bitsPerPass;

		for (int i = 0; i < nPasses; i++) {
			int lowBit = i * bitsPerPass;
			std::vector<Brick> &toBeSorted = (i & 1) ? tempArr : gridVec;
			std::vector<Brick> &sortedValues = (i & 1) ? gridVec : tempArr;

			const int nBuckets = 1 << bitsPerPass;
			int bucketCount[nBuckets] = { 0 };
			int bitMask = (1 << bitsPerPass) - 1;
			for (Brick mc : toBeSorted) {
				int bucket = (mc.mortonCode >> lowBit) & bitMask;
				++bucketCount[bucket];
			}

			int outIndex[nBuckets];
			outIndex[0] = 0;
			for (int k = 1; k < nBuckets; ++k)
				outIndex[k] = outIndex[k - 1] + bucketCount[k - 1];

			for (Brick mc : toBeSorted) {
				int bucket = (mc.mortonCode >> lowBit) & bitMask;
				sortedValues[outIndex[bucket]++] = mc;
			}
		}

		if (nPasses & 1)
			std::swap(gridVec, tempArr);

		std::copy(gridVec.begin(), gridVec.end(), sortedBricks);
	}

	/* 1 - compaction operation to partition all the emptybricks
		   in the list to the end so we no longer have to consider them  (TODO: not done yet)
	//2 -  assign 30-bit 3-d Morton codes to the non-empty bricksand sort them using a parallelO(n)GPU algorithm */
	void compactionAndMortonCodeAssignment() {
		int i = 0;

		for (int b = 0; b < bricksSize; b++) {
			glm::ivec3 coord;
			to3D(b, bricksResolution.x, bricksResolution.y, coord);
			coord *= BRICK_DIMENSION_SIZE;

			if (!bricks[b].isEmpty) {
				bricks[b].mortonCode = encodeMorton3D(coord);
				sortedBricks[i++] = bricks[b];
			}
		}

		//sort by Morton code
		radixSort();
	}

	void findAndMarkEmptyBricks() {
		int nonEmptyBricks = 0;
		for (int b = 0; b < bricksSize; b++) {
			//Slightly different from what is suggested from the paper
			//There each thread visits a voxel an then vote atomically to change it as empty or not.
			for (int i = 0; i < BRICK_SIZE; i++) {
				if (bricks[b].grid[i] != 0.0f) {
					nonEmptyBricks++;
					bricks[b].isEmpty = false;
					break;
				}
			}
		}

		sortedBricksSize = nonEmptyBricks;
		sortedBricks = new Brick[nonEmptyBricks];
	}

	//when the volume is loaded for the first time, subdivide it into bricks of size 8^3
	void subdivideIntoBricks() {
		bricksResolution = glm::vec3(std::ceil(gridResolution.x/ BRICK_DIMENSION_SIZE), std::ceil(gridResolution.y / BRICK_DIMENSION_SIZE), std::ceil(gridResolution.z / BRICK_DIMENSION_SIZE));
		bricksSize = bricksResolution.x * bricksResolution.y * bricksResolution.z;
		bricks = new Brick[bricksSize];
		
		for (int b = 0; b < bricksSize; b++) {
			bricks[b].grid = new float[BRICK_SIZE];

			glm::ivec3 gridPos;
			to3D(b, bricksResolution.x, bricksResolution.y, gridPos);
			gridPos = gridPos * BRICK_DIMENSION_SIZE;

			//x, y and z are in grid coordinates
			for (int x = gridPos.x; x < gridPos.x + BRICK_DIMENSION_SIZE; x++)
				for (int y = gridPos.y; y < gridPos.y + BRICK_DIMENSION_SIZE; y++)
					for (int z = gridPos.z; z < gridPos.z + BRICK_DIMENSION_SIZE; z++) {

						int gridIndex = to1D(gridResolution.x, gridResolution.y, x, y, z);
						int localBrickIndex = to1D(BRICK_DIMENSION_SIZE, BRICK_DIMENSION_SIZE, intmod(glm::ivec3(x,y,z), BRICK_DIMENSION_SIZE));
						if (gridIndex > gridSize) {
							bricks[b].grid[localBrickIndex] = 0.0f;
							continue;
						}
						bricks[b].grid[localBrickIndex] = grid[gridIndex];
					}
		}
	}

	void testing() {

		Node *node = &nodes[0];
		while (!node->isLeaf)
			node = node->rightChild;


		traverse(Ray(glm::vec3(0.5f, 0.5f, -3), glm::vec3(0, 0, 1)));
	}

	void generateTestGrid() {
		this->gridResolution = glm::vec3(16, 16, 16);
		this->gridSize = gridResolution.x * gridResolution.y * gridResolution.z;
		this->grid = new float[gridSize];

		for (int i = 0; i < gridSize; i++)
			this->grid[i] = 1.0f; 
	}

	void printStatus() {
		std::cout << "-------------------------------" << std::endl;
		std::cout << "Paper:" << std::endl;
		std::cout << "Depth: " << levels << std::endl;
		std::cout << "Number of nodes: " << formatWithCommas(nodesSize) << std::endl;
		std::cout << "Brick size: [8x8x8] = " << 512 << std::endl;
		std::cout << "-------------------------------" << std::endl;
	}

	std::string getStatus() {
		std::stringstream ss;
		ss << "Depth: " << levels << std::endl;
		ss << "Number of nodes: " << formatWithCommas(nodesSize) << std::endl;
		ss << "Brick size: [8x8x8] = " << 512 << std::endl;
		return ss.str();
	}

	LBVH3(float *grid, glm::vec3 gridResolution) {
		this->grid = grid;
		this->gridResolution = gridResolution;
		this->gridSize = gridResolution.x * gridResolution.y * gridResolution.z;
		//generateTestGrid();

		subdivideIntoBricks();
		findAndMarkEmptyBricks();
		compactionAndMortonCodeAssignment();
		generateHierarchy(this->sortedBricksSize);

		//testing();
	}

	LBVH3();

	~LBVH3() {
		delete[] bricks;
		delete[] sortedBricks;
		delete[] nodes;
	}
};