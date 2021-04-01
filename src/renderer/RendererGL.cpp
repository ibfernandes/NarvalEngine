#include "RendererGL.h"

namespace narvalengine {
	RendererGL *rendererGL;

	void FrameBufferGL::create(Attachment *attachments, int length) {
		glGenFramebuffers(1, &id);
		memcpy(this->attachments, attachments, length * sizeof(Attachment));
		attachmentsLength = length;

		int colorIndex = 0;
		bind();
		for (int i = 0; i < attachmentsLength; i++) {
			GLenum attachmentType = GL_COLOR_ATTACHMENT0 + colorIndex;
			TextureGL tex = rendererGL->textures[attachments[i].texh.id];
			TextureLayout texLayout = tex.texLayout;
			this->width = tex.width;
			this->height = tex.height;

			//TODO: include stencil only
			if (isDepth(texLayout)) {
				if (texLayout == TextureLayout::D24S8)
					attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
				else if (texLayout == TextureLayout::D24)
					attachmentType = GL_DEPTH_ATTACHMENT;
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, tex.id, 0);
		}
		unbind();
	}

	void RendererGL::init() {
		glEnable(GL_DEPTH_TEST);
		glGenVertexArrays(1, &vao);
		rendererGL = this;
	}
}