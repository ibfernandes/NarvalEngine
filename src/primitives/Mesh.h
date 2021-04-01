#pragma once
#include <glad/glad.h>
#include <vector>
#include "core/ResourceManager.h"
#include "utils/StringID.h"
#include <glm/glm.hpp>

namespace narvalengine {
	struct TextureInfo {
		StringID texID;
		std::string bindName;
	};

	class Mesh {
	public:
		//GLuint VAO, VBO, EBO;
		//Both are pointers to the Model
		float *vertexDataPointer;
		int vertexDataPointerLength;
		int *vertexIndicesPointer;
		int vertexIndicesPointerLength;
		std::vector<TextureInfo> textures;
		int strideLength;
		VertexLayout vertexLayout;

		Mesh();
		~Mesh();

		/*void init() {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

			glBufferData(GL_ARRAY_BUFFER, vertexDataPointerLength * sizeof(float), vertexDataPointer, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndicesPointerLength * sizeof(int),
				vertexIndicesPointer, GL_STATIC_DRAW);

			// vertex positions
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideLength * sizeof(float), (void*)0);
			// vertex normals
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, strideLength * sizeof(float), (void*)(3 * sizeof(float)));
			// tangent coords
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, strideLength * sizeof(float), (void*)(6 * sizeof(float)));
			// vertex texture coords
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, strideLength * sizeof(float), (void*)(9 * sizeof(float)));

			glBindVertexArray(0);
		}

		void bindTextures(std::string shaderName) {
			for (unsigned int i = 0; i < textures.size(); i++) {
				glActiveTexture(GL_TEXTURE0 + i);
				std::string name = textures[i].bindName;

				ResourceManager::getSelf()->getShader(shaderName).setInteger(name, i);
				ResourceManager::getSelf()->getTexture(textures[i].name).bind();
			}
		}

		void renderVertices() {
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, vertexIndicesPointerLength, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		void render(std::string shaderName) {
			bindTextures(shaderName);

			renderVertices();

			glActiveTexture(GL_TEXTURE0);
		}*/
	};
}