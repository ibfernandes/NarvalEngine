#pragma once
#include "defines.h"
#include <glad/glad.h>
#include "VAO.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "ResourceManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <boost/filesystem.hpp>
#include <assimp/pbrmaterial.h>

class Model {
public:
	Model();
	~Model();
	std::vector<Mesh> meshes;

	GLfloat *vertAttrib;
	std::string relativePath;
	int vertAttribSize;
	int stride;
	int *format;
	int formatSize;
	int numberOfVertices;
	VAO vao;

	void processNode(aiNode *node, const aiScene *scene, std::string path){
		relativePath = path;
		// process all the node's meshes (if any)
		//if (scene->HasMaterials())
		//	std::cout << relativePath << " has materials" << std::endl;
		//else
		//	std::cout << relativePath << " does not have materials" << std::endl;

		for (unsigned int i = 0; i < node->mNumMeshes; i++){
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		// then do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
			processNode(node->mChildren[i], scene, path);
	}

	Mesh processMesh(aiMesh *mesh, const aiScene *scene){
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<TextureInfo> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++){
			Vertex vertex;
			glm::vec3 vector; 
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			// normals
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.normal = vector;

			//tangent
			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.tangent = vector;

			// texture coordinates
			if (mesh->mTextureCoords[0]){
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.texCoords = vec;
			}
			else
				vertex.texCoords = glm::vec2(0.0f, 0.0f);
			
			vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++){
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		
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

		Mesh m;
		m.vertices = vertices;
		m.indices = indices;
		m.textures = textures;
		m.init();
		return m;
	}

	std::vector<TextureInfo> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, const aiScene *scene){
		std::vector<TextureInfo> loadingTextures;
		
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++){
			aiString texture_file;
			mat->Get(AI_MATKEY_TEXTURE(type, i), texture_file);
			
			aiString str;
			mat->GetTexture(type, i, &str);
			std::string string = str.C_Str();
			string = string.substr(string.find_last_of("\\") + 1);

			if (const aiTexture *texture = scene->GetEmbeddedTexture(texture_file.C_Str())) {
				Texture2D t2d;
				std::cout << typeName << std::endl;
				std::cout << texture->achFormatHint << std::endl;
				std::cout << texture->mFilename.C_Str() << std::endl;
				std::cout << texture->mHeight << std::endl;
				std::cout << texture->mWidth << std::endl;
				t2d.generateWithData(texture);
				ResourceManager::getSelf()->setTexture2D(string, t2d);
			}
			else {
				ResourceManager::getSelf()->loadTexture2D(string, relativePath + string);
				//std::cout << string << std::endl;
			}

			//TODO: not best solution, but boost is not working.
			//boost::filesystem::path p(string);
			//std::cout << p.filename().string() << std::endl;

			TextureInfo ti;
			ti.name = string;
			ti.bindName = typeName;
			loadingTextures.push_back(ti);
		}
		return loadingTextures;
	}

	void loadVerAttrib(GLfloat *vertAttrib, int vertAttribSize, int *format, int formatSize) {
		this->vertAttrib = vertAttrib;
		this->vertAttribSize = vertAttribSize;

		int stride = 0;
		for (int i = 0; i < formatSize; i++) 
			stride += format[i];
		
		this->stride = stride;
		this->numberOfVertices = vertAttribSize/stride;
		this->format = format;
		this->formatSize = formatSize;

		vao.generate(vertAttrib, vertAttribSize, stride, format, formatSize);
	}
};