#include "renderer/RendererGL.h"

namespace narvalengine {
	RendererGL *rendererGL;

	void APIENTRY
		MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {

		if (type == GL_DEBUG_TYPE_ERROR) 
			LOG(FATAL) << "OpenGL API error. "  << std::endl <<
			"Message: " << message << std::endl << 
			"Type: 0x" << std::hex << type << std::endl <<
			"Severity: 0x" << std::hex << severity;
	}

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
		glEnable(GL_TEXTURE_2D);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(MessageCallback, 0);

		glEnable(GL_DEPTH_TEST);
		glGenVertexArrays(1, &vao);
		rendererGL = this;
	}
}