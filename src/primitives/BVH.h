#pragma once
#include <vector>
#include <stack>
#include "primitives/Primitive.h"
#include "primitives/AABB.h"
#include "primitives/Triangle.h"

namespace narvalengine {
	class Model;

	struct BVHNode {
		glm::vec3 aabbMin{};
		glm::vec3 aabbMax{};
		AABB aabb;
		uint32_t start;
		uint32_t primitiveCount;
		// Node + 1 > Node's left child
		// Node + rightOffset > Node's right child
		uint32_t rightOffset;

		inline bool isLeaf() const noexcept{ return rightOffset == 0; };
	};

	struct BVHStackItem {
		//Parent index in the nodes array.
		uint32_t parent;
		//Start index in the primitives array.
		uint32_t start;
		//End index in the primitives array.
		uint32_t end;
	};

	struct BVHTraversalNode {
		uint32_t nodeIndex;
		float tNear;
	};

	/**
	 * Implementation adapted from: https://github.com/brandonpelfrey/Fast-BVH.
	 */
	class BVH {
	public:
		BVHNode *nodes;
		Model* model;
		uint32_t nodeCount = 0;

		void expandAABB(glm::vec3 &min, glm::vec3 &max, glm::vec3 nMin, glm::vec3 nMax) {
			min = glm::min(min, nMin);
			max = glm::max(max, nMax);
		}

		void expandAABBToInclude(glm::vec3& min, glm::vec3& max, Primitive *primitive) {
			glm::vec3 pMin, pMax;
			primitive->calculateAABB(pMin, pMax);
			expandAABB(min, max, pMin, pMax);
		}

		void expandAABBToInclude(glm::vec3& min, glm::vec3& max, const glm::vec3 &point) {
			expandAABB(min, max, point, point);
		}

		int longestDimensionIndex(glm::vec3 v) {
			int res = 0;
			if (v[1] > v[res])
				res = 1;
			if (v[2] > v[res])
				res = 2;
			return res;
		}

		void init(Model* model);
		bool intersect(Ray ray, RayIntersection& rayIntersection);
	};
}