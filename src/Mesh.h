#pragma once
#include <glad/glad.h>
#include <vector>
#include "ResourceManager.h"
#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct TextureInfo {
	std::string name;
	std::string bindName;
};

class Mesh{
public:
	GLuint VAO, VBO, EBO;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<TextureInfo> textures;

	Mesh();
	~Mesh();

	void init() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
			&indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

		glBindVertexArray(0);
	}

	void render(std::string shaderName){
		for (unsigned int i = 0; i < textures.size(); i++){
			glActiveTexture(GL_TEXTURE0 + i);
			std::string name = textures[i].bindName;

			ResourceManager::getSelf()->getShader(shaderName).setInteger(name,i);
			ResourceManager::getSelf()->getTexture2D(textures[i].name).bind();
		}

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glActiveTexture(GL_TEXTURE0);
	}
};

