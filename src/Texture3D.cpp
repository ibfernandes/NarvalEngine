#include "Texture3D.h"

Texture3D::Texture3D() {
}
Texture3D::Texture3D(int width, int height, int depth){
	this->width = width;
	this->height = height;
	this->depth = depth;
}


Texture3D::~Texture3D()
{
}
