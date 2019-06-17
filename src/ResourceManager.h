#pragma once
#include "defines.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Model.h"
#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include "stb_image.h"

class ResourceManager {
private:
	ResourceManager();
	static ResourceManager* self;
	std::map<std::string, Shader> shaders;
	std::map<std::string, Model> models;
	std::map<std::string, Texture2D> textures2D;
	
public:
	~ResourceManager();
	static ResourceManager *getSelf();


	void addModel(std::string name, Model model) {

		if (models.count(name) > 0)
			return;

		models.insert({name, model});
	}

	Model getModel(std::string name) {
		return models.at(name);
	}

	Texture2D getTexture2D(std::string name) {
		return textures2D.at(name);
	}

	Texture2D loadTexture2D(std::string name, std::string path) {
		if (textures2D.count(name) > 0)
			return textures2D.at(name);

		int width, height, channels;
		unsigned char *data = stbi_load( (RESOURCES_DIR+path).c_str(), &width, &height, &channels, 0);
		Texture2D tex;
		tex.generateWithData(width, height, channels, data);
		textures2D.insert({ name, tex });
		stbi_image_free(data);

		return textures2D.at(name);
	}

	Shader getShader(std::string name){
		return shaders.at(name);
	}

	Shader loadShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath,
		std::string geometryShaderPath) {

		if (shaders.count(name) > 0)
			return shaders.at(name);

		shaders.insert({ name, loadShaderFromFile(vertexShaderPath, fragmentShaderPath, geometryShaderPath) });

		return shaders.at(name);
	}

	Shader loadShaderFromFile(std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath) {
		std::string  vertexCode = "", fragmentCode = "", geometryCode = "";
		std::string buffer;
		std::ifstream vc(RESOURCES_DIR + vertexShaderPath);

		if (!vc) {
			std::cerr << "Couldn't read vertex shader file." << std::endl;
			exit(1);
		}

		while (vc) {
			getline(vc, buffer);
			vertexCode = vertexCode + buffer + "\n";
		}

		std::ifstream fc(RESOURCES_DIR + fragmentShaderPath);

		if (!fc) {
			std::cerr << "Couldn't read fragment shader file." << std::endl;
			exit(1);
		}

		while (fc) {
			getline(fc, buffer);
			fragmentCode = fragmentCode + buffer + "\n";
		}

		Shader* shader = new Shader;
		shader->compile(vertexCode, fragmentCode, (geometryCode.empty()) ? "" : geometryCode);
		return *shader;
	}

};
