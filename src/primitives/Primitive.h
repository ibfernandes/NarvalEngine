#pragma once
#include "primitives/Ray.h"
#include "core/RendererAPI.h"
#include <glm/glm.hpp>

namespace narvalengine {
	class Primitive;
	class Material;
	class InstancedModel;

	struct RayIntersection {
		float tNear, tFar;
		glm::vec3 hitPoint;
		glm::vec3 normal;
		Primitive *primitive;
		InstancedModel* instancedModel;
		glm::vec2 uv;
	};

	class Primitive {
	public:
		Material *material = nullptr;
		VertexLayout *vertexLayout = nullptr;

		/*
			Checks if Ray r intesercts this primitive. If true, stores its values on hit.
			Ray must be in OCS.
		*/
		virtual bool intersect(Ray r, RayIntersection &hit) = 0;
		/*
			Sample a random point on surface in WCS relative to the intersection
		*/
		virtual glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) = 0;
		/*
			Given a hit point on surface, convert it to UV normalized texture coordinates
		*/
		virtual glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface) = 0;
		virtual float pdf(RayIntersection interaction, glm::mat4 transformToWCS) = 0;
	};
}
