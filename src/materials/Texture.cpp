#include "materials/Texture.h" 

namespace narvalengine {
	Texture::Texture() {

	}

	Texture::Texture(int width, int height, TextureLayout texFormat, int samplerFlags, MemoryBuffer memBuffer) {
		this->width = width;
		this->height = height;
		this->nPixels = width * height;
		this->texFormat = texFormat;
		this->samplerFlags = samplerFlags;

		mem.data = new uint8_t[memBuffer.size];
		mem.size = memBuffer.size;
		memCopy(&mem, memBuffer.data, memBuffer.size);
	}

	Texture::Texture(int width, int height, int depth, TextureLayout texFormat, int samplerFlags, MemoryBuffer memBuffer) {
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->nPixels = width * height * depth;
		this->texFormat = texFormat;
		this->samplerFlags = samplerFlags;

		mem.data = new uint8_t[memBuffer.size];
		mem.size = memBuffer.size;
		memCopy(&mem, memBuffer.data, memBuffer.size);
	}

	glm::ivec3 Texture::getResolution() {
		return glm::ivec3(width, height, depth);
	}

	glm::vec4 Texture::sampleAtIndex(uint32_t index) {
		glm::vec4 res = glm::vec4(0, 0, 0, 0);

		if (texFormat == R32F || texFormat == RG32F || texFormat == RGB32F || texFormat == RGBA32F) {
			float* p = (float*)mem.data;
			int pos1d = index;

			switch (texFormat) {
			case R32F:
				p = p + index;
				res.x = p[0];
				break;
			case RG32F:
				p = p + (index*2);
				res.x = p[0];
				res.y = p[1];
				break;
			case RGB32F:
				p = p + (index*3);
				res.x = p[0];
				res.y = p[1];
				res.z = p[2];
				break;
			case RGBA32F:
				p = p + (index*4);
				res.x = p[0];
				res.y = p[1];
				res.z = p[2];
				res.w = p[3];
				break;
			}
		}
		else if (texFormat == RGBA8) {
			uint8_t* p = (uint8_t*)mem.data;
			int pos1d = index;

			switch (texFormat) {
			case RGBA8:
				p = p + (index*4);
				res.x = p[0] / 255.0f;
				res.y = p[1] / 255.0f;
				res.z = p[2] / 255.0f;
				res.w = p[3] / 255.0f;
				break;
			}
		}
		else
			DLOG(WARNING) << "No sampler for TextureLayout " << magic_enum::enum_name(texFormat) << " was found. " << toString(res) << " will be returned.";

		return res;
	}
	void Texture::wrapTextureCoordinates(float &u, float &v, float &w) {
		if (samplerFlags & NE_TEX_SAMPLER_U_CLAMP)
			u = glm::clamp(u, 0.0f, 1.0f);
		else if (samplerFlags & NE_TEX_SAMPLER_U_MIRROR)
			u = glm::clamp(glm::abs(u - (int)u), 0.0f, 1.0f);
		else
			u = glm::clamp(u, 0.0f, 1.0f); //Default: clamp.

		if (samplerFlags & NE_TEX_SAMPLER_V_CLAMP)
			v = glm::clamp(v, 0.0f, 1.0f);
		else if (samplerFlags & NE_TEX_SAMPLER_V_MIRROR)
			v = glm::clamp(glm::abs(v - (int)v), 0.0f, 1.0f);
		else
			v = glm::clamp(v, 0.0f, 1.0f); //Default: clamp.

		if (samplerFlags & NE_TEX_SAMPLER_W_CLAMP)
			w = glm::clamp(w, 0.0f, 1.0f);
		else if (samplerFlags & NE_TEX_SAMPLER_W_MIRROR)
			w = glm::clamp(glm::abs(w - (int)w), 0.0f, 1.0f);
		else
			w = glm::clamp(w, 0.0f, 1.0f); //Default: clamp.
	}

	glm::vec4 Texture::sample(float u, float v, float w) {
		wrapTextureCoordinates(u, v, w);
		glm::vec4 res = glm::vec4(0, 0, 0, 0);
		glm::ivec3 pos;

		pos.x = u * this->width;
		pos.y = v * this->height;
		pos.z = w * this->depth;

		//image ranges from [0, width -1], so we must treat the border case where we reach the boundary.
		if (pos.x > 0 && pos.x == this->width)
			pos.x--;
		if (pos.y > 0 && pos.y == this->height)
			pos.y--;
		if (pos.z > 0 && pos.z == this->depth)
			pos.z--;
		
		return sampleAtIndex(to1D(this->width, this->height, pos.x, pos.y, pos.z));
	}
}