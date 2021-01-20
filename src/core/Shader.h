#pragma once
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

//TODO: compile from a middleware language and then convert to GLSL, HLSL etc
namespace narvalengine {
	class Shader {
	private:
		int id;

	public:
		std::string vertexShader;
		std::string fragmentShader;
		Shader();
		~Shader();

		//TODO: define a middle-language to later on compile to GLSL and/or HLSL as needed
		void addSourceCode(std::string vertexShader, std::string fragmentShader) {
			this->vertexShader = vertexShader;
			this->fragmentShader = fragmentShader;
		}
	};
}