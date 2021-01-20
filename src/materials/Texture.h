#pragma once
#include <glad/glad.h>
#include "core/RendererAPI.h"
#include "utils/Utils.h"
#include <glm/glm.hpp>

namespace narvalengine {
	inline static int getNumberOfChannels(TextureLayout texFormat) {
		if (texFormat == R32I || texFormat == R32F || texFormat == R8) {
			return 1;
		} else if (texFormat == RG32I || texFormat == RG32F || texFormat == RG8) {
			return 2;
		} else if (texFormat == RGB32I || texFormat == RGB32F || texFormat == RGB8) {
			return 3;
		}else if (texFormat == RGBA32I || texFormat == RGBA32F || texFormat == RGBA8) {
			return 4;
		}else {
			return 0;
		}
	};

	class Texture {
	public:
		int width = 0, height = 0, depth = 0;
		TextureLayout texFormat;
		TextureName textureName;
		TextureChannelFormat textureChannelFormat;
		int samplerFlags;
		MemoryBuffer mem;

		Texture() {};

		Texture(int width, int height, TextureLayout texFormat, int samplerFlags, MemoryBuffer memBuffer) {
			this->width = width;
			this->height = height;
			this->texFormat = texFormat;
			this->samplerFlags = samplerFlags;
			
			mem.data = new uint8_t[memBuffer.size];
			mem.size = memBuffer.size;
			memCopy(&mem, memBuffer.data, memBuffer.size);
		}

		Texture(int width, int height, int depth, TextureLayout texFormat, int samplerFlags, MemoryBuffer memBuffer) {
			this->width = width;
			this->height = height;
			this->depth = depth;
			this->texFormat = texFormat;
			this->samplerFlags = samplerFlags;

			mem.data = new uint8_t[memBuffer.size];
			mem.size = memBuffer.size;
			memCopy(&mem, memBuffer.data, memBuffer.size);
		}

		glm::ivec3 getResolution() {
			return glm::ivec3(width, height, depth);
		}
		
		glm::vec4 sample(int x, int y) {
			glm::vec4 res = glm::vec4(0, 0, 0, 0);
			if (texFormat == R32F || texFormat == RG32F || texFormat == RGB32F || texFormat == RGBA32F) {
				float* p = (float*)mem.data;

				switch (texFormat) {
					case R32F:
						res.x = p[0]; break;
					case RG32F:
						res.x = p[0]; 
						res.y = p[1];
						break;
					case RGB32F:
						res.x = p[0];
						res.y = p[1];
						res.z = p[2];
						break;
					case RGBA32F:
						res.x = p[0];
						res.y = p[1];
						res.z = p[2];
						res.a = p[3];
						break;
				}
			}

			return res;
		}
	};
}