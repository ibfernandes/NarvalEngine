#pragma once
#include "Model.h"
class Renderer
{
public:
	Renderer();
	~Renderer();

	void render(Model model) {
		model.vao.bindVAO();
		glDrawArrays(GL_TRIANGLES, 0, model.numberOfVertices);
		glBindVertexArray(0);
	}
};

