#pragma once
#include <glad/glad.h>
#include "Texture2D.h"

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

	void clear() {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void attachTexture(int id) {
		bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
		unbind();
	}
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

