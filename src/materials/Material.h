#pragma once
#include "materials/Texture.h"

namespace narvalengine {
	class Light;
	class Medium;
	class BSDF;

	class Material {
	public:
		TextureName textureTypes;
		Texture *textures[TextureName::TextureNameCount];
		Medium *medium;
		Light *light;
		BSDF *bsdf;

		bool hasMedium() {
			if (medium)
				return true;
			else
				return false;
		}

		bool hasLight() {
			if (light)
				return true;
			else
				return false;
		}

		bool hasBSDF() {
			if (bsdf)
				return true;
			else
				return false;
		}

		void addTexture(TextureName textureType, Texture *tex2D) {
			if (tex2D->textureName != textureType)
				DLOG(WARNING) << "Texture names don't match. It was expected " << magic_enum::enum_name(textureType) << " but got tex2D->" << magic_enum::enum_name(tex2D->textureName);
 			
			textureTypes = TextureName(textureType | textureType);
			textures[ctz(textureType)] = tex2D;
		}

		Texture *getTexture(TextureName textureType) {
			return textures[ctz(textureType)];
		}

		bool hasTexture(TextureName type) {
			return textureTypes & type;
		}

		/**
		 * Sample specific channels with semantic meaning of {@code textureType}.
		 * 
		 * @param textureType
		 * @param u texture coordinate.
		 * @param v texture coordinate.
		 * @return sampled color.
		 */
		glm::vec4 sampleMaterial(TextureName textureType, float u, float v) {
			int idx = ctz(textureType);

			if (textures[idx] != nullptr)
				return textures[idx]->sample(u, v);
			else
				return glm::vec4(0, 0, 0, 1);
		}
	};
}