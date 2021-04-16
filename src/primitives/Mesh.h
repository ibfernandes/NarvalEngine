#pragma once
#include <glad/glad.h>
#include <vector>
#include "core/ResourceManager.h"
#include "utils/StringID.h"
#include <glm/glm.hpp>

namespace narvalengine {
	struct TextureInfo {
		StringID texID;
		TextureName texName;
		std::string bindName;
		std::string rmTexName;
	};

	class Mesh {
	public:
		float *vertexDataPointer = nullptr;
		int vertexDataPointerLength;
		int* vertexIndicesPointer = nullptr;
		int vertexIndicesPointerLength;
		//std::vector<TextureInfo> textures;
		//TextureInfo textures[TextureName::Count];
		Material *material = nullptr;
		int modelMaterialIndex = -1;
		int strideLength; //TODO should be substituted by vertexLayout
		VertexLayout vertexLayout;
		//TODO should have a material too?

		Mesh();
		~Mesh();
	};
}