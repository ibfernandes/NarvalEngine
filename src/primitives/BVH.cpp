#include "primitives/BVH.h"
#include "primitives/Model.h"

namespace narvalengine {

	void BVH::init(Model* model) {
		this->model = model;
		nodes = new BVHNode[model->primitives.size() * 2];
		std::stack<BVHStackItem> stack;

		const uint32_t primitivesPerLeaf = 4;
		const uint32_t notVisited = 0xffffffff;
		const uint32_t visitedTwice = 0xfffffffd;
		const uint32_t rootParent = 0xfffffffc;
		nodeCount = 0;

		BVHStackItem root = {
			rootParent,
			0,
			model->primitives.size()
		};
		stack.push(root);

		BVHNode node;
		while (stack.size() > 0) {
			BVHStackItem currentStackItem = stack.top();
			stack.pop();
			nodeCount++;

			node.start = currentStackItem.start;
			node.primitiveCount = currentStackItem.end - currentStackItem.start;
			node.rightOffset = notVisited;

			//Calculate the bounding box for this node.
			glm::vec3 bbMin = glm::vec3(INFINITY);
			glm::vec3 bbMax = glm::vec3(-INFINITY);
			expandAABBToInclude(bbMin, bbMax, model->primitives[node.start]);
			AABB aabb = AABB(&bbMin[0], &bbMax[0]);
			glm::vec3 bcMin = aabb.getCenter();
			glm::vec3 bcMax = aabb.getCenter();

			for (int i = currentStackItem.start + 1; i < currentStackItem.end; i++) {
				Primitive* p = model->primitives[i];
				glm::vec3 pMin;
				glm::vec3 pMax;
				p->calculateAABB(pMin, pMax);
				AABB pAABB = AABB(&pMin[0], &pMax[0]);
				expandAABB(bbMin, bbMax, pMin, pMax);

				expandAABB(bcMin, bcMax, pAABB.getCenter(), pAABB.getCenter());
			}

			node.aabbMin = bbMin;
			node.aabbMax = bbMax;

			//If the number of primitives in this iteration fits a leaf, mark this node as leaf.
			if (node.primitiveCount <= primitivesPerLeaf)
				node.rightOffset = 0;

			nodes[nodeCount - 1] = node;
			nodes[nodeCount - 1].aabb = AABB(&nodes[nodeCount - 1].aabbMin[0], &nodes[nodeCount - 1].aabbMax[0]);

			//If not the root.
			if (currentStackItem.parent != rootParent) {
				//Special case: if child touches parent.
				nodes[currentStackItem.parent].rightOffset--;

				if (nodes[currentStackItem.parent].rightOffset == visitedTwice)
					nodes[currentStackItem.parent].rightOffset = nodeCount - 1 - currentStackItem.parent;
			}

			//If this is a leaf, no need to subdivide.
			if (node.rightOffset == 0) continue;

			//Choose to dimension in which to split.
			uint32_t splitDimension = longestDimensionIndex(bcMax - bcMin);

			// Split on the center of the longest axis.
			float splitCoord = 0.5f * (bcMin[splitDimension] + bcMax[splitDimension]);

			// Partition the list of objects on this split.
			uint32_t mid = currentStackItem.start;
			for (uint32_t i = currentStackItem.start; i < currentStackItem.end; i++) {
				Primitive* p = model->primitives[i];
				glm::vec3 pMin;
				glm::vec3 pMax;
				p->calculateAABB(pMin, pMax);
				AABB pAABB = AABB(&pMin[0], &pMax[0]);
				if (pAABB.getCenter()[splitDimension] < splitCoord) {
					std::swap(model->primitives[i], model->primitives[mid]);
					mid++;
				}
			}

			// If a bad split is generated, choose the center.
			if (mid == currentStackItem.start || mid == currentStackItem.end) 
				mid = currentStackItem.start + (currentStackItem.end - currentStackItem.start) / 2;

			BVHStackItem rightChild{ nodeCount - 1, mid, currentStackItem.end };

			BVHStackItem leftChild{ nodeCount - 1, currentStackItem.start, mid };

			stack.push(rightChild);
			stack.push(leftChild);
		}
	}

	bool BVH::intersect(Ray ray, RayIntersection& rayIntersection) {
		uint32_t closer, other;

		// Working set
		// WARNING : The working set size is relatively small here, should be made dynamic or template-configurable
		BVHTraversalNode todo[64];
		int32_t stackptr = 0;

		// "Push" on the root node to the working set.
		todo[stackptr].nodeIndex = 0;
		todo[stackptr].tNear = INFINITY;

		RayIntersection closestIntersection;
		closestIntersection.tNear = INFINITY;
		closestIntersection.tFar = -INFINITY;

		bool anyHit = false;

		while (stackptr >= 0) {

			// Pop off the next node to work on.
			uint32_t ni = todo[stackptr].nodeIndex;
			float nearest = todo[stackptr].tNear;
			stackptr--;
			BVHNode node = nodes[ni];

			// If this node is further than the closest found intersection, continue
			if (nearest > closestIntersection.tNear) continue;

			// Is leaf -> Intersect
			if (node.isLeaf()) {
				for (uint32_t i = 0; i < node.primitiveCount; i++) {
					Primitive* p = model->primitives[node.start + i];

					RayIntersection temp;
					bool current = p->intersect(ray, temp);
					anyHit = anyHit || current;
					if (current && temp.tNear < closestIntersection.tNear) {
						closestIntersection = temp;
						//TODO: good optimization!
						// If we're only looking for occlusion, then any hit is good enough to return true
						//if (Flags & TraverserFlags::OnlyTestOcclusion) {
						//	return current;
						//}
					}
				}
			} else {  // Not a leaf
				RayIntersection leftNodeHit;
				RayIntersection rightNodeHit;

				bool hitc0 = nodes[ni + 1].aabb.intersect(ray, leftNodeHit);
				bool hitc1 = nodes[ni + node.rightOffset].aabb.intersect(ray, rightNodeHit);

				// Did we hit both nodes?
				if (hitc0 && hitc1) {
					// We assume that the left child is a closer hit...
					closer = ni + 1;
					other = ni + node.rightOffset;

					// ... If the right child was actually closer, swap the relavent values.
					if (rightNodeHit.tNear < leftNodeHit.tNear) {
						RayIntersection temp = leftNodeHit;
						leftNodeHit = rightNodeHit;
						rightNodeHit = temp;
						std::swap(closer, other);
					}

					// It's possible that the nearest object is still in the other side, but
					// we'll check the further-awar node later...

					// Push the farther first
					todo[++stackptr] = BVHTraversalNode{ other, rightNodeHit.tNear };
					// And now the closer (with overlap test)
					todo[++stackptr] = BVHTraversalNode{ closer, leftNodeHit.tNear };

				} else if (hitc0) {
					todo[++stackptr] = BVHTraversalNode{ ni + 1, leftNodeHit.tNear };
				} else if (hitc1) {
					todo[++stackptr] = BVHTraversalNode{ ni + node.rightOffset, rightNodeHit.tNear };
				}
			}
		}

		rayIntersection = closestIntersection;

		return anyHit;
	}
}
