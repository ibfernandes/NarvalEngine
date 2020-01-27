#pragma once
#include "Model.h"
#include "VAO.h"
class Renderer
{
public:
	Renderer();
	~Renderer();	
	VAO vao;

	void render(Model model) {
		model.vao.bindVAO();
		glDrawArrays(GL_TRIANGLES, 0, model.numberOfVertices);
		glBindVertexArray(0);
	}

	void renderLine(glm::vec3 origin, glm::vec3 end) {

		GLfloat vertices[] = {
			origin.x, origin.y, origin.z, end.x, end.y, end.z
		};
		int format[] = { 3 };

		vao.generate(vertices, 6, 3, format, 1);

		vao.bindVAO();
		glDrawArrays(GL_LINES, 0, 6);
		glBindVertexArray(0);
	}
};

