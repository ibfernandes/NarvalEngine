#pragma once
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
private:
	int id;

public:
	Shader();
	~Shader();

	Shader* use() {
		glUseProgram(id);
		return this;
	};

	int getId() {
		return id;
	}

	/**
	* Compile all vertex, fragment and geometry source code into a linked program.
	* Geometry is optional.
	**/
	void compile(std::string vertexSource, std::string fragmentSource, std::string geometrySource) {
		GLuint  vertexShader = 0, geometryShader = 0, fragmentShader;
		const char *c_str = 0;

		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &(c_str = vertexSource.c_str()), NULL);
		glCompileShader(vertexShader);
		checkCompileErrors(vertexShader, "VERTEX");

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &(c_str = fragmentSource.c_str()), NULL);
		glCompileShader(fragmentShader);
		checkCompileErrors(fragmentShader, "FRAGMENT");

		if (!geometrySource.empty()) {
			geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometryShader, 1, &(c_str = geometrySource.c_str()), NULL);
			glCompileShader(geometryShader);
			checkCompileErrors(fragmentShader, "GEOMETRY");
		}

		id = glCreateProgram();
		glAttachShader(id, vertexShader);
		if (!geometrySource.empty())
			glAttachShader(id, geometryShader);
		glAttachShader(id, fragmentShader);
		glLinkProgram(id);
		checkCompileErrors(id, "PROGRAM");

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		if (!geometrySource.empty())
			glDeleteShader(geometryShader);
	}

	/**
	 * Checks and print compiling ERRORS.
	 **/
	void checkCompileErrors(int object, std::string type) {
		int success;
		GLchar infoLog[512];

		if (type != "PROGRAM") {
			glGetShaderiv(object, GL_COMPILE_STATUS, &success);

			if (success == 0) {
				glGetShaderInfoLog(object, 512, NULL, infoLog);
				std::cout << infoLog << std::endl;
			}

		}
		else {
			glGetProgramiv(object, GL_LINK_STATUS, &success);

			if (success == 0) {
				glGetProgramInfoLog(object, 512, NULL, infoLog);
				std::cout << infoLog << std::endl;
			}

		}
	}

	void setFloat(std::string name, float value) {
		glUniform1f(glGetUniformLocation(id, name.c_str()), value);
	}

	void setInteger(std::string name, int value) {
		glUniform1i(glGetUniformLocation(id, name.c_str()), value);
	}

	void setVec2(std::string name, float x, float y) {
		glUniform2f(glGetUniformLocation(id, name.c_str()), x, y);
	}

	void setVec3(std::string name, float x, float y, float z) {
		glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
	}

	void setMat4(std::string name, glm::mat4 mat) {
		if (glGetUniformLocation(id, name.c_str()) == -1) {
			std::cout << "ERROR: uniform " + name + " not found";
			exit(1);
		}

		glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
	}
};