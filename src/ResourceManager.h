#pragma once
#include "defines.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Texture3D.h"
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
class Model;

class ResourceManager {
private:
	ResourceManager();
	static ResourceManager* self;
	std::map<std::string, Shader> shaders;
	std::map<std::string, Model> models;
	std::map<std::string, Texture2D> textures2D;
	std::map<std::string, Texture3D> textures3D;
	
public:
	~ResourceManager();
	static ResourceManager *getSelf();

	void addModel(std::string name, Model model);
	Model loadModel(std::string name, std::string path, std::string fileName);
	Model getModel(std::string name);
	Texture3D getTexture3D(std::string name);
	Texture3D loadVDBasTexture3D(std::string name, std::string path, int resolution);
	Texture2D getTexture2D(std::string name);
	void setTexture2D(std::string name, Texture2D t);
	Texture2D loadTexture2D(std::string name, std::string path);
	Shader getShader(std::string name);
	Shader loadShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath,
		std::string geometryShaderPath);
	Shader loadShaderFromFile(std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath);
};
