#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"

namespace narvalengine {
	class Triangle : public Primitive {
	public:
		//Pointers to Model OCS vertex data
		//The layout for each vertex can vary, as per Model layout options.
		//Ex: Strides of 5 floats (VERT_COORD_3F_TEX_COORD_2F), strides of 3 floats (VERT_COORD_3F) etc
		float *vertexData[3];
		float *normal;

		Triangle();
		~Triangle();
		Triangle(float *index1, float *index2, float *index3, Material *material, float *normal) {
			this->material = material;
			vertexData[0] = index1;
			vertexData[1] = index2;
			vertexData[2] = index3;
			this->normal = normal;
		}

		Triangle(float *index1, float *index2, float *index3, Material *material) {
			this->material = material;
			vertexData[0] = index1;
			vertexData[1] = index2;
			vertexData[2] = index3;
		}

		Triangle(float *index1, float *index2, float *index3) {
			vertexData[0] = index1;
			vertexData[1] = index2;
			vertexData[2] = index3;
		}

		glm::vec3 getVertex(int n);
		/*
			Checks if Ray r intesercts this primitive. If true, stores its values on hit.
			Ray must be in OCS.
			Source: https://www.iquilezles.org/www/articles/intersectors/intersectors.htm
		*/
		bool intersect(Ray r, RayIntersection &hit);
		glm::vec3 samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec2 samplePointOnTexture(glm::vec3 pointOnSurface);
		float pdf(RayIntersection interaction, glm::mat4 transformToWCS);
		glm::vec3 barycentricCoordinates(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c);
	};
}