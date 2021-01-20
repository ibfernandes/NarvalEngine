#include "core/Renderer.h"

namespace narvalengine {
	/*
	Renderer::Renderer() {
	}


	Renderer::~Renderer() {
	}

	void Renderer::render(Model *model) {
		model->vao.bindVAO();
		glDrawArrays(GL_TRIANGLES, 0, model->numberOfVertices);
		glBindVertexArray(0);
	}

	void Renderer::renderLine(glm::vec3 origin, glm::vec3 end) {

		GLfloat vertices[] = {
			origin.x, origin.y, origin.z, end.x, end.y, end.z
		};
		int format[] = { 3 };

		vao.generate(vertices, 6, 3, format, 1);

		vao.bindVAO();
		glDrawArrays(GL_LINES, 0, 6);
		glBindVertexArray(0);
	}*/
}
