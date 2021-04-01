#pragma once
#include "defines.h"
#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include "stb_image.h"
#define __TBB_NO_IMPLICIT_LINKAGE 1
#define __TBBMALLOC_NO_IMPLICIT_LINKAGE 1
#include <openvdb/openvdb.h>
#include <openvdb/tools/Dense.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "core/Shader.h"
#include "materials/Texture.h"
#include "utils/StringID.h"

namespace narvalengine {
	class Model;
	class Material;

	class ResourceManager {
	private:
		ResourceManager();
		static ResourceManager* self;
		std::unordered_map<StringID, Shader*> shaders;
		std::unordered_map<StringID, Model*> models;
		std::unordered_map<StringID, Material*> materials;
		std::unordered_map<StringID, Texture*> textures;

	public:
		~ResourceManager();
		static ResourceManager *getSelf();

		StringID setModel(std::string name, Model *model);
		StringID replaceModel(std::string name, Model *model);
		StringID loadModel(std::string name, std::string path, std::string fileName);
		StringID loadModel(std::string name, std::string path, std::string fileName, std::string materialName);
		Model* getModel(std::string name);
		Model* getModel(StringID id);

		StringID setMaterial(std::string name, Material *material);
		StringID replaceMaterial(std::string name, Material *material);
		Material* getMaterial(std::string name);
		Material* getMaterial(StringID id);

		StringID setTexture(std::string name, Texture* t);
		StringID replaceTexture(std::string name, Texture* t);
		StringID loadVDBasTexture(std::string name, std::string path);
		StringID loadVolasTexture(std::string name, std::string path);
		StringID loadTexture(std::string name, std::string path);
		Texture* getTexture(std::string name);
		Texture* getTexture(StringID id);

		StringID loadShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath);
		Shader* getShader(std::string name);
		Shader* getShader(StringID id);
	};
}