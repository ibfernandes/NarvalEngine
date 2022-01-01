#pragma once
#include "defines.h"
#include <glad/glad.h>
#include "primitives/Mesh.h"
#include "primitives/Ray.h"
#include "primitives/AABB.h"
#include "primitives/Triangle.h"
#include "primitives/Primitive.h"
#include "materials/Texture.h"
#include "lights/Light.h"
#include "materials/Material.h"
#include "core/ResourceManager.h"
#include "utils/Math.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <assimp/pbrmaterial.h>
#include "materials/GridMedia.h"
#include "core/GlossyBSDF.h"
#include "core/Microfacet.h"

namespace narvalengine {
	class Model {
	private:
		/**
		 *	Pointer for vertex data.
		 */
		float* vertexData = nullptr;
		uint32_t vertexDataLength = 0;

		/**
		 *	Pointer for index data.
		 */
		uint32_t* indexData = nullptr;
		uint32_t indexDataLength = 0;//Number of faces * 3 ( IF TRIANGLES )

		void processNode(aiNode* node, const aiScene* scene, std::string path, Material* m, std::string materialName);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene, unsigned int vertexStartIndex, unsigned int indicesStartIndex, Material* m, std::string materialName);
		std::vector<TextureInfo> loadMaterialTextures(Material* m, std::string materialName);
		std::vector<TextureInfo> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, TextureName texName, const aiScene* scene);
		/**
		* Process a scene loaded from Assimp. Must be triangulated first.
		*
		* @param scene
		* @param path
		* @param materialName
		*/
		void processScene(const aiScene* scene, std::string path, std::string materialName);
	public:
		std::string relativePath;

		//vertex, normal, tan, UV
		int strideLayout[4] = { 3, 3, 3, 2 };
		int layoutSize = 4; //Data per vertex 
		VertexLayout vertexLayout; //TODO sync with mesh and primitive

		//for processing only
		int stride = 11;
		int startIndexVertex = 0;
		int startIndexIndices = 0;

		std::vector<Mesh> meshes;

		//Contains only non-emissive primitives.
		MemoryBuffer memoryBufferPrimitives{};
		std::vector<Primitive*> primitives;

		//Contains only emissive primitives.
		MemoryBuffer memoryBufferLights{};
		std::vector<Primitive*> lights;

		//Contains all materials associated with this model's meshes
		//Materials are allocated through the resource manager.
		std::vector<Material*> materials;

		glm::vec3 aabbMin = glm::vec3(+INFINITY);
		glm::vec3 aabbMax = glm::vec3(-INFINITY);
		AABB boundingBox;

		/**
		 * All memory is assumed to be allocated externally and this model has now the ownership of all associated memory.
		 * 
		 * @param vertexData
		 * @param indexData
		 * @param memoryBufferPrimitives
		 * @param primitives
		 * @param memoryBufferLights
		 * @param lights
		 * @param materials
		 * @param vertexLayout
		 */
		Model(MemoryBuffer vertexData, MemoryBuffer indexData,
			MemoryBuffer memoryBufferPrimitives,
			std::vector<Primitive*> primitives,
			MemoryBuffer memoryBufferLights,
			std::vector<Primitive*> lights,
			std::vector<Material*> materials,
			std::vector<Mesh> meshes,
			VertexLayout vertexLayout);

		/**
		 * {@code vertexData} is assumed to be vertices composed by triangles.
		 * 
		 * @param vertexData
		 * @param indexData
		 * @param vertexLayout
		 */
		Model(MemoryBuffer vertexData, MemoryBuffer indexData, VertexLayout vertexLayout);

		Model(const aiScene* scene, std::string path, std::string materialName);

		~Model();

		/**
		*	Tests if {@code ray} intersects any of this model's primitives. 
		* 
		* @param ray in Object Coordinate System (OCS).
		* @param hit in which to store the result.
		* @param tMin minimum distance in which to consider an intersection.
		* @param tMax maximum distance in which to consider an intersection.
		* @return true if intersects any primitive composing this model. False otherwise.
		*/
		bool intersect(Ray ray, RayIntersection& hit, float& tMin, float& tMax);

		AABB* getAABB();
		void centralize();
		Primitive* getRandomLightPrimitive();
	};
}