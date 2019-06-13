#pragma once
#include <glad/glad.h>
#include "VAO.h"

class Model {
public:
	Model();
	~Model();

	GLfloat *vertAttrib;
	int vertAttribSize;
	int stride;
	int *format;
	int formatSize;
	int numberOfVertices;
	VAO vao;

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

private:
};