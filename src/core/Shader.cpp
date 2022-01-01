#include "core/Shader.h"

namespace narvalengine {
	Shader::Shader() {
	}

	Shader::~Shader() {
	}

	void Shader::addSourceCode(std::string vertexShader, std::string fragmentShader) {
		this->vertexShader = vertexShader;
		this->fragmentShader = fragmentShader;
	}
}
