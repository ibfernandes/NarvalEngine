#pragma once
#include <glad/glad.h>

class VAO {
public:
	GLuint id;
	GLuint vboID;
	VAO();
	~VAO();

	void generate(GLfloat *vertAttrib, int vertAttribSize, int stride, int *format, int formatSize) {
		glGenVertexArrays(1, &id);
		glGenBuffers(1, &vboID);

		glBindVertexArray(id);

		glBindBuffer(GL_ARRAY_BUFFER, vboID);
		glBufferData(GL_ARRAY_BUFFER, vertAttribSize * sizeof(GLfloat), vertAttrib, GL_STATIC_DRAW);

		int strideSum = 0;
		for (int i = 0; i < formatSize; i++) {
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, format[i], GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (GLvoid*) (strideSum * sizeof(GLfloat)));
			strideSum += format[i];
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void bindVAO() {
		glBindVertexArray(id);
	}
};

