#include "defines.h"
#include "Shader.h"
#include <string>
#include <iostream>
#include <map>
#include <fstream>

class ResourceManager {
private:
	ResourceManager();
	static ResourceManager* self;
	std::map<std::string, Shader> shaders;
	
public:
	~ResourceManager();
	static ResourceManager *getSelf();

	void test() {

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
