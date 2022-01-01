#pragma once
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace narvalengine {
	class Shader {
	private:
		int id = -1;

	public:
		/**
		 * Vertex shader code in plain text.
		 */
		std::string vertexShader;
		/**
		 * Fragment shader code in plain text.
		 */
		std::string fragmentShader;
		Shader();
		~Shader();

		void addSourceCode(std::string vertexShader, std::string fragmentShader);
	};
}