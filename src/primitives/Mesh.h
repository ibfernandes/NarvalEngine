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
		uint32_t* vertexIndicesPointer = nullptr;
		int vertexIndicesPointerLength;
		Material *material = nullptr;
		int modelMaterialIndex = -1;
		int strideLength;
		VertexLayout vertexLayout;

		Mesh();
		~Mesh();
	};
}