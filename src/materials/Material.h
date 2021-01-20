#pragma once
#include "materials/Texture.h"

namespace narvalengine {
	class Light;
	class Medium;
	class BSDF;

	class Material {
	public:
		TextureName textureTypes;
		std::vector<Texture*> textures;
		Medium *medium;
		Light *light;
		BSDF *bsdf;

		void addTexture(TextureName textureType, Texture *tex2D) {
			textureTypes = TextureName(textureTypes | textureType);
			textures.push_back(tex2D);
		}

		bool containsMaterialName(TextureName type) {
			return textureTypes & type;
		}

		/*
			Sample specific channels with semantic meaning of TextureName
		*/
		glm::vec4 sampleMaterial(TextureName textureType, float x, float y) {
			//assert(containsMaterialName(textureType));

			for (Texture *tex : textures) {
				if (tex->textureName & textureType)
					return tex->sample(x, y);
			}
		}
	};
}