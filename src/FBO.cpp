#include "FBO.h"



FBO::FBO(){
	glGenFramebuffers(1, &id);
}


FBO::~FBO(){
	glDeleteFramebuffers(1, &id);
}
