#pragma once
#include <glad/glad.h>
#include "Texture2D.h"
#include <glm/glm.hpp>

class FBO
{
public:
	GLuint id;

	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	void unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);   
	}

	void clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void clear(glm::vec4 color) {
		clear(color.x, color.y, color.z, color.a);
	}

	void clear() {
		clear(0, 0, 0, 0);
	}

	void attachTextureColorAndDepthStencil(int colorTex, int depthTex) {
		bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR: Framebuffer in not complete!";
		unbind();
	}

	void attachTexture(int id) {
		bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
		unbind();
	}

	//TODO: change name to "depth" only
	void attachDepthAndStencil(int id) {
		bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, id, 0);
		unbind();
	}

	void attachTexture(Texture2D texture) {
		attachTexture(texture.id);
	}

	FBO();
	~FBO();
};

