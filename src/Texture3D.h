#pragma once
#include <glad/glad.h>
#include <stdio.h>
#include <glm/glm.hpp>
#include <algorithm>

class Texture3D{
private:
	int width, height, depth, channels;

public:
	Texture3D();
	Texture3D(int width, int height, int depth);
	~Texture3D();
	GLuint glId = 0;
	float *floatData;
	int *intData;

	void loadToMemory(int width, int height, int depth, int nmrChannels, float *data) {
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->channels = nmrChannels;
		this->floatData = new float[width * height * depth * nmrChannels];
		std::copy(data, data + width * height * depth, this->floatData);
	}

	void loadToMemory(int width, int height, int depth, int nmrChannels, int *data) {
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->channels = nmrChannels;
		this->intData = new int[width * height * depth * nmrChannels];
		std::copy(data, data + width * height * depth, this->intData);
	}

	void loadToOpenGL(int width, int height, int depth, int internalFormat, int format, int type, int wrap, int filter, float *data) {
		glGenTextures(1, &(this->glId));
		glBindTexture(GL_TEXTURE_3D, glId);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);

		glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0, format, type, data);

		glBindTexture(GL_TEXTURE_3D, 0);
	}

	void loadToOpenGL(int width, int height, int depth, int internalFormat, int format, int type, int wrap, int filter, int *data) {
		glGenTextures(1, &glId);
		glBindTexture(GL_TEXTURE_3D, glId);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);

		glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0, format, type, data);

		glBindTexture(GL_TEXTURE_3D, 0);
	}

	void loadToOpenGL(int width, int height, int depth, int numberOfChannels, int *data) {
		switch (numberOfChannels) {
			case 1:
				loadToOpenGL(width, height, depth, GL_R32I, GL_RED_INTEGER, GL_INT, GL_CLAMP_TO_BORDER, GL_NEAREST, data); break;
			case 3:
				loadToOpenGL(width, height, depth, GL_RGB32I, GL_RGB_INTEGER, GL_INT, GL_CLAMP_TO_BORDER, GL_NEAREST, data); break;
			case 4:
				loadToOpenGL(width, height, depth, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, GL_CLAMP_TO_BORDER, GL_NEAREST, data); break;
		}
	}

	void loadToOpenGL(int width, int height, int depth, int numberOfChannels, float *data) {
		switch (numberOfChannels) {
			case 1:
				loadToOpenGL(width, height, depth, GL_R8, GL_RED, GL_FLOAT, GL_CLAMP_TO_BORDER, GL_NEAREST, data); break;
			case 3:
				loadToOpenGL(width, height, depth, GL_RGB8, GL_RGB, GL_FLOAT, GL_CLAMP_TO_BORDER, GL_NEAREST, data); break;
			case 4:
				loadToOpenGL(width, height, depth, GL_RGBA8, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_BORDER, GL_NEAREST, data); break;
		}
	}

	void loadToOpenGL() {
		loadToOpenGL(this->width, this->height, this->depth, this->channels, this->floatData);
	}

	void bind() {
		glBindTexture(GL_TEXTURE_3D, glId);
	}

	glm::vec3 getResolution() {
		return glm::vec3(width, height, depth);
	}
};

