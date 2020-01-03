#include "FBO.h"



FBO::FBO(){
	glGenFramebuffers(1, &id);


	if (!glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		// didn't work
	}
		
}


FBO::~FBO(){
	glDeleteFramebuffers(1, &id);
}
