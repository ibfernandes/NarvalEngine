#pragma once
#include <glad/glad.h>
#include <stdio.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

class Texture1D
{
public:
	Texture1D();
	~Texture1D();
	GLuint id;

	//TODO: be aware of texture maxsize
	void generateWithData(int size, int *data) {
		this->size = size;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_1D, id);

		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32I, size, 0, GL_RED_INTEGER, GL_INT, data);

		glBindTexture(GL_TEXTURE_1D, 0);
	}
	
	void bind() {
		glBindTexture(GL_TEXTURE_1D, id);
	}

private:
	int size;
};
