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
//#include <boost/filesystem.hpp>
#include <assimp/pbrmaterial.h>
#include "materials/GridMedia.h"

namespace narvalengine {
	//Tells how each primitive should interpret its Vertex Data
	/*enum VertexPacket {
		COORD_3F, //TODO duplicate
		COORD_3F_NORMAL_3F,
		COORD_3F_NORMAL_3F_TANGENT_3F_TEX_2F
	};

	enum VertexData {
		COORD_3F,
		NORMAL_3F,
		TANGENT_3F,
		TEX0_2F,
		TEX1_2F
	};*/

	class Model {
	public:
		std::string relativePath;
		int stride = 11;

		float *vertexData;
		int *faceVertexIndices;
		//vertex, normal, tan, UV
		int strideLayout[4] = {3, 3, 3, 2};
		int layoutSize = 4; //Data per vertex 
		//VAOGL vao;
		VertexLayout vertexLayout; //TODO sync with mesh and primitive

		int vertexDataLength = 0;
		//Number of faces * 3 ( IF TRIANGLES )
		int faceVertexIndicesLength = 0;


		std::vector<Mesh> meshes;
		//Contains only non-emissive primitives
		std::vector<Primitive*> primitives;
		//Contains only emissive primitives
		std::vector<Primitive*> lights;
		std::vector<Material*> materials;

		glm::vec3 aabbMin = glm::vec3(INFINITY);
		glm::vec3 aabbMax = glm::vec3(-INFINITY);
		AABB *boundingBox = nullptr;

		Model();
		~Model();
		/*
			Tests if this ray intersects any of this model's primitives. Ray must be in this Model's OCS
		*/
		bool intersect(Ray ray, RayIntersection &hit, float &tMin, float &tMax) {
			bool ditIntersect = false;
			RayIntersection tempIntersec;

			if (boundingBox != nullptr)
				if (!boundingBox->intersect(ray, tempIntersec))
					return false;

			//TODO: Check AABB first
			//If no LBVH is set, iterate linearly over all primitives
			for (int i = 0; i < primitives.size(); i++) {

				//If it is a participating medium like grid, check if the ray intersects any voxel.
				if(primitives.at(i)->material != nullptr && primitives.at(i)->material->medium != nullptr)
					if (GridMedia* gridMedia = dynamic_cast<GridMedia*>(primitives.at(i)->material->medium)) {
						glm::vec3 res = gridMedia->lbvh->traverseTreeUntil(ray, tMax);

						//If intersected any non-empty voxel
						if (res.x <= res.y) {
							tempIntersec.primitive = primitives.at(i);
							tempIntersec.tNear = res.x;
							tempIntersec.tFar = res.y;
							tempIntersec.hitPoint = ray.getPointAt(res.x);
							tMax = tempIntersec.tNear;
							hit = tempIntersec;
							ditIntersect = true;
						}

						//A model can only have one Grid Media
						continue;
					}


				bool localIntersec = primitives.at(i)->intersect(ray, tempIntersec);
				//If we are inside a volume
				if (localIntersec && tempIntersec.tNear != tempIntersec.tFar && tempIntersec.tNear < 0 && tempIntersec.tFar > 0) {
					tempIntersec.tNear = 0;
					tempIntersec.hitPoint = ray.o;
					tMax = tempIntersec.tNear;
					hit = tempIntersec;
					ditIntersect = true;
				}

				if (localIntersec && tempIntersec.tNear > tMin && tempIntersec.tNear < tMax) {
					tMax = tempIntersec.tNear;
					hit = tempIntersec;
					ditIntersect = true;
				}
			}

			for (int i = 0; i < lights.size(); i++) {
				//If infinite area light, ignore intersection
				if (!isBlack(lights.at(i)->material->light->Le()))
					continue;

				bool localIntersec = lights.at(i)->intersect(ray, tempIntersec);
				if (localIntersec && tempIntersec.tNear > tMin && tempIntersec.tNear < tMax) {
					tMax = tempIntersec.tNear;
					hit = tempIntersec;
					ditIntersect = true;
				}
			}

			return ditIntersect;
		}

		AABB *getAABB();
		/*
			Centers this AABB model at origin in OCS
		*/
		void centralize();
		//TODO: add or generate primitive, to also guard putting the primitive in the worng list, i.e. ones with light MUST go to lights
		Primitive* getRandomLightPrimitive();

		void processScene(const aiScene* scene, std::string path, std::string materialName) {
			Material* mat = nullptr;

			if(materialName.compare("") != 0)
				mat = ResourceManager::getSelf()->getMaterial(genStringID(materialName));
			if(mat != nullptr)
				materials.push_back(mat);

			for (int i = 0; i < scene->mNumMeshes; i++) {
				vertexDataLength += scene->mMeshes[i]->mNumVertices * stride;
				faceVertexIndicesLength += scene->mMeshes[i]->mNumFaces * 3;
			}

			vertexData = new float[vertexDataLength];
			faceVertexIndices = new int[faceVertexIndicesLength];// 3 vertices (TRIANGLE)

			vertexLayout.init();
			vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			vertexLayout.add(VertexAttrib::Normal, VertexAttribType::Float, 3);
			vertexLayout.add(VertexAttrib::Tangent, VertexAttribType::Float, 3);
			vertexLayout.add(VertexAttrib::TexCoord0, VertexAttribType::Float, 2);
			vertexLayout.end();

			processNode(scene->mRootNode, scene, path, mat, materialName);
			boundingBox = new AABB(&aabbMin[0], &aabbMax[0]);

			//loadVerAttrib(vertexData, vertexDataLength, strideLayout, layoutSize);
		}

		//START POINT
		void processNode(aiNode *node, const aiScene *scene, std::string path, Material *m, std::string materialName) {
			relativePath = path;

			int *startIndexVertex = new int[node->mNumMeshes];
			int *startIndexIndices = new int[node->mNumMeshes];
			int stride = 11;//TODO enable multiple size strides (i.e. STRIDE per MESH)

			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				startIndexVertex[i] = (i < 1) ? 0 : (startIndexVertex[i - 1] + scene->mMeshes[node->mMeshes[i]]->mNumVertices * stride);
				startIndexIndices[i] = (i < 1) ? 0 : (startIndexIndices[i - 1] + scene->mMeshes[node->mMeshes[i]]->mNumFaces * 3);
			}

			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
				meshes.push_back(processMesh(mesh, scene, startIndexVertex[i], startIndexIndices[i], m, materialName));
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++)
				processNode(node->mChildren[i], scene, path, m, materialName);

		}

		Mesh processMesh(aiMesh *mesh, const aiScene *scene, unsigned int vertexStartIndex, unsigned int indicesStartIndex, Material *m, std::string materialName) {
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				glm::vec3 v(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
				aabbMin = glm::min(aabbMin, v);
				aabbMax = glm::max(aabbMax, v);

				this->vertexData[vertexStartIndex + i * stride] = mesh->mVertices[i].x;
				this->vertexData[vertexStartIndex + i * stride + 1] = mesh->mVertices[i].y;
				this->vertexData[vertexStartIndex + i * stride + 2] = mesh->mVertices[i].z;

				// normals
				this->vertexData[vertexStartIndex + i * stride + 3] = mesh->mNormals[i].x;
				this->vertexData[vertexStartIndex + i * stride + 4] = mesh->mNormals[i].y;
				this->vertexData[vertexStartIndex + i * stride + 5] = mesh->mNormals[i].z;

				//tangent
				this->vertexData[vertexStartIndex + i * stride + 6] = mesh->mTangents[i].x;
				this->vertexData[vertexStartIndex + i * stride + 7] = mesh->mTangents[i].y;
				this->vertexData[vertexStartIndex + i * stride + 8] = mesh->mTangents[i].z;

				//texture coordinates (UV)
				//TODO should also verify if other componenets are present
				if (mesh->mTextureCoords[0]) {
					vertexData[vertexStartIndex + i * stride + 9] = mesh->mTextureCoords[0][i].x;
					vertexData[vertexStartIndex + i * stride + 10] = mesh->mTextureCoords[0][i].y;
				}
			}

			//Since we are setting a constraint of dealing always with triangles, aiFace->mNumIndices will always be 3 (vertices)
			int vertexIndicesCurrentIndex = 0;
			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];

				float *vertexAddress[3];

				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					float index = face.mIndices[j];
					this->faceVertexIndices[indicesStartIndex + vertexIndicesCurrentIndex++] = index;
					vertexAddress[j] = &(this->vertexData[vertexStartIndex + face.mIndices[j] * stride]);
				}

				//glm::vec3 v1(*(vertexAddress[0]), *(vertexAddress[0] + 1), *(vertexAddress[0] + 2));
				//glm::vec3 v2(*(vertexAddress[1]), *(vertexAddress[1] + 1), *(vertexAddress[1] + 2));
				//glm::vec3 v3(*(vertexAddress[2]), *(vertexAddress[2] + 1), *(vertexAddress[2] + 2));

				primitives.push_back(new Triangle(vertexAddress[0], vertexAddress[1], vertexAddress[2], m, vertexAddress[0] + 3));
			}

			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			std::vector<TextureInfo> textures;
			// 1. diffuse maps
			std::vector<TextureInfo> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "material.diffuse", scene);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			// 2. specular maps
			std::vector<TextureInfo> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "material.specular", scene);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			// 3. normal maps
			std::vector<TextureInfo> normalMaps = loadMaterialTextures(material, aiTextureType_DISPLACEMENT, "material.normal", scene);
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			// 4. height maps
			std::vector<TextureInfo> heightMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_height", scene);
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

			//Material specified via .json scene descriptor
			if (m != nullptr) {
				std::vector<TextureInfo> texs = loadMaterialTextures(m, materialName);
				textures.insert(textures.end(), texs.begin(), texs.end());
			}

			Mesh thismesh;
			thismesh.strideLength = stride;
			thismesh.vertexDataPointer = &this->vertexData[vertexStartIndex];
			thismesh.vertexDataPointerLength = mesh->mNumVertices * stride;
			//thismesh.vertexIndicesPointer = &this->faceVertexIndices[indicesStartIndex];
			thismesh.vertexIndicesPointer = new int[mesh->mNumFaces * 3];
			for (int i = 0; i < mesh->mNumFaces; i++) {
				thismesh.vertexIndicesPointer[i * 3] = mesh->mFaces[i].mIndices[0];
				thismesh.vertexIndicesPointer[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
				thismesh.vertexIndicesPointer[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
			}
			thismesh.vertexIndicesPointerLength = mesh->mNumFaces * 3;// each face is made of 3 vertices (triangle), we force it using triangulate on ASSIMP
			thismesh.textures = textures;
			thismesh.vertexLayout = vertexLayout;
			//thismesh.init();
			return thismesh;
		}

		std::vector<TextureInfo> loadMaterialTextures(Material *m, std::string materialName) {
			std::vector<TextureInfo> loadingTextures;

			for (Texture* t : m->textures) {

				TextureInfo ti;
				if (t->textureName == ALBEDO) {
					ti.texID = genStringID(materialName + ".albedo");
					ti.bindName = "material.diffuse";
				}
				else if (t->textureName == METALLIC) {
					continue;
				}
				loadingTextures.push_back(ti);
			}

			return loadingTextures;
		}

		std::vector<TextureInfo> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, const aiScene *scene) {
			std::vector<TextureInfo> loadingTextures;

			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
				aiString texture_file;
				mat->Get(AI_MATKEY_TEXTURE(type, i), texture_file);

				aiString str;
				mat->GetTexture(type, i, &str);
				std::string string = str.C_Str();
				string = string.substr(string.find_last_of("\\") + 1);
				StringID texID;

				if (const aiTexture *texture = scene->GetEmbeddedTexture(texture_file.C_Str())) {
					/*Texture *tex;
					std::cout << typeName << std::endl;
					std::cout << texture->achFormatHint << std::endl;
					std::cout << texture->mFilename.C_Str() << std::endl;
					std::cout << texture->mHeight << std::endl;
					std::cout << texture->mWidth << std::endl;
					t2d.generateWithData(texture);
					ResourceManager::getSelf()->setTexture2D(string, t2d);*/
				} else {
					texID = ResourceManager::getSelf()->loadTexture(string, relativePath + string);
					//std::cout << string << std::endl;
				}

				//TODO: not best solution, but boost is not working.
				//boost::filesystem::path p(string);
				//std::cout << p.filename().string() << std::endl;

				TextureInfo ti;
				ti.texID = texID;
				ti.bindName = typeName;
				loadingTextures.push_back(ti);
			}
			return loadingTextures;
		}
	};
}