#pragma once
#include <glad/glad.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <algorithm>

class Texture3D
{
public:
	Texture3D();
	Texture3D(int width, int height, int depth);
	~Texture3D();
	GLuint id;
	float *data;

	void generateWithData(int width, int height, int depth, int nmrChannels, int *data) {
		//this->data = new float[width * height * depth];
		//std::copy(data, data + width * height * depth, this->data);
		this->width = width;
		this->height = height;
		this->depth = depth;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_3D, id);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
		if (nmrChannels == 1)
			glTexImage3D(GL_TEXTURE_3D, 0, GL_R32I, width, height, depth, 0, GL_RED_INTEGER, GL_INT, data);
		if(nmrChannels == 3)
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32I, width, height, depth, 0, GL_RGB_INTEGER, GL_INT, data);
		if(nmrChannels == 4)
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32I, width, height, depth, 0, GL_RGBA_INTEGER, GL_INT, data);

		glBindTexture(GL_TEXTURE_3D, 0);
	}

	void generateWithData(int width, int height, int depth, int nmrChannels, float *data) {
		this->data = new float[width * height * depth];
		std::copy(data, data + width * height * depth, this->data);
		this->width = width;
		this->height = height;
		this->depth = depth;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_3D, id);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		if (nmrChannels == 1)
			glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, width, height, depth, 0, GL_RED, GL_FLOAT, data);
		if (nmrChannels == 3)
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
		if (nmrChannels == 4)
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_FLOAT, data);

		glBindTexture(GL_TEXTURE_3D, 0);
	}

	void bind() {
		glBindTexture(GL_TEXTURE_3D, id);
	}

	glm::vec3 getResolution() {
		return glm::vec3(width, height, depth);
	}

private:
	int width, height, depth;
};

