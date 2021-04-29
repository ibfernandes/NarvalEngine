#pragma once
#include "materials/Texture.h"

namespace narvalengine {
	class Light;
	class Medium;
	class BSDF;

	class Material {
	public:
		TextureName textureTypes;
		//std::vector<Texture*> textures;
		Texture *textures[TextureName::TextureNameCount];
		Medium *medium;
		Light *light;
		BSDF *bsdf;

		//TODO should verify if tex2D is seted as this textureType?
		void addTexture(TextureName textureType, Texture *tex2D) {
			textureTypes = TextureName(textureType | textureType);
			textures[ctz(textureType)] = tex2D;
			//textures.push_back(tex2D);
		}

		bool containsMaterialName(TextureName type) {
			return textureTypes & type;
		}

		/*
			Sample specific channels with semantic meaning of TextureName
		*/
		glm::vec4 sampleMaterial(TextureName textureType, float x, float y) {
			//assert(containsMaterialName(textureType));
			int idx = ctz(textureType);

			if (textures[idx] != nullptr)
				return textures[idx]->sample(x, y);
			else
				return glm::vec4(0, 0, 0, 1);

			/*for (Texture *tex : textures) {
				if (tex->textureName & textureType)
					return tex->sample(x, y);
			}*/
		}
	};
}